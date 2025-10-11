#include "PythonHotfix.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>
#include <regex>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <ranges>

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
    for (const auto& kv : packageSet) {
        const std::string& mod = kv.first;
        if (visited.find(mod) == visited.end()) {
            visited.insert(mod);
            bfs.push(mod);
            dependencyVec.push_back(mod);
        }
    }
    while (!bfs.empty()) {
        auto cur = bfs.front(); bfs.pop();
        auto it = dependencyGraph.find(cur);
        if (it == dependencyGraph.end()) continue;
        for (const auto& dep : it->second) {
            if (visited.find(dep) == visited.end()) {
                visited.insert(dep);
                bfs.push(dep);
                dependencyVec.push_back(dep);
            }
        }
    }
    PyErr_Clear();
}

bool PythonHotfix::ReloadPythonFile() {
    PyGILState_STATE gstate = PyGILState_Ensure();

    CheckPythonFileDependence();
    bool reload = !packageSet.empty();

    // Log dependency reload order for visibility
    if (reload) {
        std::cout << "[Hotfix][Reload] dependency order (before reverse): ";
        for (size_t i = 0; i < dependencyVec.size(); ++i) {
            if (i) std::cout << " -> ";
            std::cout << dependencyVec[i];
        }
        std::cout << std::endl;
    }

    for (int i = static_cast<int>(dependencyVec.size()) - 1; i >= 0; --i) {
        const std::string& name = dependencyVec[i];
        std::cout << "[Hotfix][Reload] begin reload idx=" << (dependencyVec.size() - 1 - i)
                  << "/" << dependencyVec.size() << ": " << name << std::endl;

        // Use sys.modules directly to avoid allocating temporary Unicode objects
        PyObject* sys_modules = PyImport_GetModuleDict();
        PyObject* old_mod = sys_modules ? PyDict_GetItemString(sys_modules, name.c_str()) : nullptr; // borrowed
        bool owned_old = false;
        if (!old_mod) {
            // As a fallback, import the module to get a handle (owned)
            old_mod = PyImport_ImportModule(name.c_str()); // new ref
            if (!old_mod) {
                std::cout << "[Hotfix][Reload] skip (module not found and import failed): " << name << std::endl;
                if (PyErr_Occurred()) PyErr_Clear();
                continue;
            }
            owned_old = true;
        }

        // Sanity
        if (!PyModule_Check(old_mod)) {
            std::cout << "[Hotfix][Reload] skip (not a module): " << name << " ptr=" << old_mod << std::endl;
            if (owned_old) { /* leak-safe: skip DECREF */ }
            continue;
        }

        // Cross-check pointer in sys.modules
        if (sys_modules) {
            PyObject* smod = PyDict_GetItemString(sys_modules, name.c_str()); // borrowed
            std::cout << "[Hotfix][Reload] sys.modules['" << name << "']=" << smod
                      << (smod == old_mod ? " (same)" : " (different)") << std::endl;
        }

        PyObject* new_mod = PyImport_ReloadModule(old_mod);
        if (!new_mod) {
            std::cout << "[Hotfix][Reload] reload failed: " << name << std::endl;
            if (PyErr_Occurred()) PyErr_Print();
            // leak-safe: if we owned 'old_mod', skip DECREF on failure to avoid risky paths
            continue; // keep trying other modules
        }

        std::cout << "[Hotfix][Reload] reload ok: name=" << name
                  << " new_mod=" << new_mod
                  << (new_mod == old_mod ? " (same ptr)" : " (new ptr)") << std::endl;

        // Log current sys.modules pointer for this name after reload
        if (sys_modules) {
            PyObject* smod2 = PyDict_GetItemString(sys_modules, name.c_str()); // borrowed
            std::cout << "[Hotfix][Reload] post sys.modules['" << name << "']=" << smod2
                      << (smod2 == new_mod ? " (matches new_mod)" : (smod2 == old_mod ? " (matches old_mod)" : " (different)"))
                      << std::endl;
        }

        // leak-safe: skip DECREF(new_mod) and any owned_old DECREF to avoid potential DECREF crash paths
        // if (owned_old) { Py_DECREF(old_mod); }
        // Py_DECREF(new_mod);
    }
    packageSet.clear();
    dependencyGraph.clear();
    dependencyVec.clear();

    PyGILState_Release(gstate);
    return reload;
}
