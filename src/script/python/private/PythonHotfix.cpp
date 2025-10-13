#include "PythonHotfix.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <queue>
#include <regex>
#include <set>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

// 静态工具函数实现
int64_t PythonHotfix::GetCurrentTimeMsec() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool PythonHotfix::EndsWith(const std::string& str, const std::string& suffix) {
    if (str.size() < suffix.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

void PythonHotfix::NormalizeModuleName(std::string& path_like) {
    if (path_like.empty()) return;
    std::ranges::replace(path_like, '\\', '.');
    std::ranges::replace(path_like, '/', '.');
    static const std::regex pattern(R"(([^\.]*?)\.py$)");
    std::smatch match;
    if (std::regex_search(path_like, match, pattern) && match.size() > 1) {
        path_like = match[1].str();
    } else {
        size_t lastDot = path_like.find_last_of('.');
        if (lastDot != std::string::npos) path_like = path_like.substr(lastDot + 1); else path_like.clear();
    }
}

void PythonHotfix::TraverseDirectory(const std::filesystem::path& directory,
                                     std::queue<std::unordered_set<std::string>>& message_que,
                                     int64_t check_time_ms) {
    std::unordered_set<std::string> local_set;
    if (!std::filesystem::exists(directory)) return;

    auto ensure_init = [](const std::filesystem::path& dir){
        auto initp = dir / "__init__.py";
        if (!std::filesystem::exists(initp)) {
            std::ofstream f(initp.string(), std::ios::app);
        }
    };
    ensure_init(directory);

    static const std::set<std::string> kSkip = {"__pycache__", "__init__.py", ".pyc", "StaticComponents.py"};

    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        const auto& p = entry.path();
        if (entry.is_directory()) { ensure_init(p); continue; }
        if (!entry.is_regular_file()) continue;

        const std::string file_name = p.filename().string();
        if (!EndsWith(file_name, ".py")) continue;

        if (std::ranges::any_of(kSkip, [&](const std::string& s){ return EndsWith(file_name, s); })) continue;

        std::error_code ec;
        auto ftime = std::filesystem::last_write_time(p, ec);
        if (ec) continue;
        auto syst = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
        int64_t mod_ms = std::chrono::duration_cast<std::chrono::milliseconds>(syst.time_since_epoch()).count();
        if (check_time_ms - mod_ms > kFileRecentWindowMs) continue;

        std::string mod_name = p.string();
        NormalizeModuleName(mod_name);
        if (!mod_name.empty()) local_set.insert(mod_name);
    }

    if (!local_set.empty()) message_que.push(std::move(local_set));
}

void PythonHotfix::CheckPythonFileDependence() {
    if (packageSet.empty()) return;
    PyObject* sys_modules = PyImport_GetModuleDict();
    if (!sys_modules) { PyErr_Clear(); return; }
    PyObject *key = nullptr, *val = nullptr; Py_ssize_t pos = 0;

    while (PyDict_Next(sys_modules, &pos, &key, &val)) {
        if (!PyModule_Check(val)) continue;
        const char* mod_name = PyModule_GetName(val);
        if (!mod_name) continue;
        PyObject* mod_dict = PyModule_GetDict(val);
        if (!mod_dict) continue;
        PyObject *ik = nullptr, *iv = nullptr; Py_ssize_t ipos = 0;
        while (PyDict_Next(mod_dict, &ipos, &ik, &iv)) {
            if (PyModule_Check(iv)) {
                if (const char* imported = PyModule_GetName(iv)) dependencyGraph[imported].insert(mod_name);
            }
        }
    }

    std::unordered_set<std::string> visited;
    std::queue<std::string> bfs;
    dependencyVec.clear();
    for (const auto& m : packageSet) {
        if (visited.insert(m).second) { bfs.push(m); dependencyVec.push_back(m); }
    }
    while (!bfs.empty()) {
        auto cur = bfs.front(); bfs.pop();
        auto it = dependencyGraph.find(cur);
        if (it == dependencyGraph.end()) continue;
        for (const auto& dep : it->second) {
            if (visited.insert(dep).second) { bfs.push(dep); dependencyVec.push_back(dep); }
        }
    }
    PyErr_Clear();
}

bool PythonHotfix::ReloadPythonFile() {
    PyGILState_STATE gstate = PyGILState_Ensure();

    CheckPythonFileDependence();
    bool reload = !packageSet.empty();
    for (int i = static_cast<int>(dependencyVec.size()) - 1; i >= 0; --i) {
        const std::string& name = dependencyVec[i];
        PyObject* name_obj = PyUnicode_FromString(name.c_str());
        if (!name_obj) { PyErr_Clear(); continue; }
        PyObject* old_mod = PyImport_GetModule(name_obj); // borrowed
        Py_DECREF(name_obj);
        if (!old_mod) { PyErr_Clear(); continue; }
        PyObject* new_mod = PyImport_ReloadModule(old_mod);
        if (!new_mod) {
            if (PyErr_Occurred()) PyErr_Print();
        } else {
            Py_DECREF(new_mod); // discard new reference
        }
    }
    packageSet.clear();
    dependencyGraph.clear();
    dependencyVec.clear();

    PyGILState_Release(gstate);
    return reload;
}

