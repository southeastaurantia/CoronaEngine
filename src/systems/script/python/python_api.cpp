#define PY_SSIZE_T_CLEAN
#include <corona/script/python/python_api.h>
#include <nanobind/stl/string.h>
#include <windows.h>

#include <iostream>
#include <ranges>
#include <regex>
#include <set>
#include <unordered_map>

#include "python_error_handler.cpp"
#include "python_path_config.cpp"

extern "C" PyObject* PyInit_CoronaEngine();

namespace Corona::Script::Python {

const std::string codePath = PathCfg::engine_root();

PythonAPI::PythonAPI() {
    logger = Kernel::create_logger();
    // TODO: 添加写入文件的 sink
    // logger->add_sink(Corona::Kernel::create_file_sink());
}

PythonAPI::~PythonAPI() {
    if (Py_IsInitialized()) {
        {
            nanobind::gil_scoped_acquire guard;
            pModule.reset();
            pFunc.reset();
            messageFunc.reset();
        }
        Py_FinalizeEx();
    }
    PyConfig_Clear(&config);
}

int64_t PythonAPI::nowMsec() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

std::string PythonAPI::wstr2str(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string out(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), out.data(), len, nullptr, nullptr);
    return out;
}

bool PythonAPI::ensureInitialized() {
    if (Py_IsInitialized()) {
        return true;
    }

    // 注册 nanobind 导出的 CoronaEngine 模块
    PyImport_AppendInittab("CoronaEngine", &PyInit_CoronaEngine);

    PyConfig_InitPythonConfig(&config);
    PyConfig_SetBytesString(&config, &config.home, CORONA_PYTHON_HOME_DIR);
    PyConfig_SetBytesString(&config, &config.pythonpath_env, CORONA_PYTHON_HOME_DIR);
    config.module_search_paths_set = 1;

    {
        std::string runtimePath = PathCfg::runtime_backend_abs();
        PyWideStringList_Append(&config.module_search_paths, str2wstr(runtimePath).c_str());
        PyWideStringList_Append(&config.module_search_paths, str2wstr(CORONA_PYTHON_MODULE_DLL_DIR).c_str());
        PyWideStringList_Append(&config.module_search_paths, str2wstr(CORONA_PYTHON_MODULE_LIB_DIR).c_str());
        PyWideStringList_Append(&config.module_search_paths, str2wstr(PathCfg::site_packages_dir()).c_str());
    }

    Py_InitializeFromConfig(&config);

    if (!Py_IsInitialized()) {
        std::cerr << "[Python Error] Python failed to initialize. Diagnostics:" << std::endl;
        try {
            auto print_wlist = [&](const char* title, const PyWideStringList& list) {
                std::cerr << "  " << title << " (" << list.length << "):" << std::endl;
                for (Py_ssize_t i = 0; i < list.length; ++i) {
                    std::wstring ws = list.items[i] ? std::wstring(list.items[i]) : L"";
                    std::cerr << "    - " << wstr2str(ws) << std::endl;
                }
            };
            std::cerr << "  home: " << wstr2str(config.home) << std::endl;
            std::cerr << "  pythonpath_env: " << wstr2str(config.pythonpath_env) << std::endl;
            print_wlist("module_search_paths", config.module_search_paths);
        } catch (...) {
            // swallow
        }
        return false;
    }

    {
        nanobind::gil_scoped_acquire gil;
        try {
            nanobind::module_ main_mod = nanobind::module_::import_("cpp_client");

            nanobind::object init_func = nanobind::getattr(main_mod, "initialize");
            init_func();

            nanobind::object run_attr = nanobind::getattr(main_mod, "run");
            nanobind::object putq_attr = nanobind::getattr(main_mod, "put_queue");

            if (!nanobind::callable::check_(run_attr)) {
                std::cerr << "[Hotfix][API] 'run' attribute is not callable" << std::endl;
                return false;
            }

            pModule = std::move(main_mod);
            pFunc = std::move(run_attr);
            messageFunc = std::move(putq_attr);
        } catch (const nanobind::python_error& e) {
            log_python_error(e);
            pModule.reset();
            pFunc.reset();
            messageFunc.reset();
            return false;
        }
    }
    return true;
}

bool PythonAPI::performHotReload() {
    int64_t currentTime = PythonHotfix::GetCurrentTimeMsec();  // ms
    constexpr int64_t kHotReloadIntervalMs = 100;              // 100ms
    if (currentTime - lastHotReloadTime <= kHotReloadIntervalMs || hotfixManger.packageSet.empty()) {
        return false;
    }

    std::cout << "[Hotfix] performHotReload triggered. packageSet.size=" << hotfixManger.packageSet.size() << std::endl;

    bool reloadedDeps = hotfixManger.ReloadPythonFile();
    if (!reloadedDeps) {
        std::cout << "[Hotfix] hotfixManger.ReloadPythonFile returned false" << std::endl;
        return false;
    }

    nanobind::gil_scoped_acquire gil;
    std::cout << "[Hotfix] reloading 'main' module (via importlib.reload)" << std::endl;

    try {
        nanobind::module_ importlib = nanobind::module_::import_("importlib");
        nanobind::object reload_func = nanobind::getattr(importlib, "reload");

        nanobind::module_ mod = nanobind::module_::import_("main");
        (void)reload_func(mod);

        nanobind::object newFunc = nanobind::getattr(mod, "run");
        if (!nanobind::callable::check_(newFunc)) {
            std::cout << "[Hotfix][API] new run attr invalid" << std::endl;
            return false;
        }
        nanobind::object newMsg = nanobind::getattr(mod, "put_queue");

        pModule = std::move(mod);
        pFunc = std::move(newFunc);
        messageFunc = std::move(newMsg);
    } catch (const nanobind::python_error& e) {
        std::cout << "[Hotfix][API] reload(main) failed (python_error)" << std::endl;
        log_python_error(e);
        return false;
    }

    lastHotReloadTime = currentTime;
    hasHotReload = true;
    std::cout << "[Hotfix] performHotReload finished successfully" << std::endl;
    return true;
}

void PythonAPI::invokeEntry(bool isReload) const {
    if (!pFunc.is_valid()) {
        return;
    }
    nanobind::gil_scoped_acquire gil;

    try {
        (void)pFunc(isReload ? 1 : 0);
    } catch (const nanobind::python_error& e) {
        log_python_error(e);
    }
}

void PythonAPI::sendMessage(const std::string& message) const {
    if (!messageFunc.is_valid()) {
        return;
    }
    nanobind::gil_scoped_acquire gil;

    try {
        (void)messageFunc(message.c_str());
    } catch (const nanobind::python_error& e) {
        log_python_error(e);
    }
}

void PythonAPI::runPythonScript() {
    if (!ensureInitialized()) {
        std::cerr << "Python init failed." << std::endl;
        return;
    }

    bool reloaded = false;
    {
        std::unique_lock lk(queMtx);
        reloaded = performHotReload();
        if (!reloaded && !hotfixManger.packageSet.empty()) {
            hasHotReload = false;
        }
    }

    invokeEntry(reloaded);
}

void PythonAPI::checkPythonScriptChange() {
    const std::string& sourcePath = PathCfg::editor_backend_abs();
    const std::string runtimePath = PathCfg::runtime_backend_abs();
    int64_t checkTime = PythonHotfix::GetCurrentTimeMsec();
    std::cout << "[Hotfix] checkPythonScriptChange: src=" << sourcePath
              << ", dst=" << runtimePath
              << ", t=" << checkTime << std::endl;
    copyModifiedFiles(sourcePath, runtimePath, checkTime);
}

void PythonAPI::checkReleaseScriptChange() {
    static int64_t lastCheckTime = 0;
    static std::unordered_map<std::string, int64_t> lastProcessedMtime;  // mod -> last mtime processed

    int64_t currentTime = PythonHotfix::GetCurrentTimeMsec();
    if (currentTime - lastCheckTime < 100) {
        return;
    }
    lastCheckTime = currentTime;

    std::queue<std::unordered_set<std::string>> messageQue;
    const std::string runtimePathStr = Corona::Script::Python::PathCfg::runtime_backend_abs();
    const std::filesystem::path runtimePath(runtimePathStr);
    PythonHotfix::TraverseDirectory(runtimePathStr, messageQue, currentTime);

    if (messageQue.empty()) {
        return;
    }

    std::unique_lock lk(queMtx);
    const auto& mods = messageQue.front();
    std::cout << "[Hotfix] detected modified modules (" << mods.size() << ") from runtime scan: ";
    bool first = true;
    for (const auto& m : mods) {
        if (!first) std::cout << ", ";
        std::cout << m;
        first = false;
    }
    std::cout << std::endl;

    auto modToPath = [&](const std::string& mod) {
        std::string rel = mod;  // replace '.' with '/'
        std::ranges::replace(rel, '.', '/');
        return runtimePath / (rel + ".py");
    };

    for (const auto& mod : mods) {
        int64_t fileMtimeMs = 0;
        std::error_code ec;
        const auto filePath = modToPath(mod);
        auto ftime = std::filesystem::last_write_time(filePath, ec);
        if (!ec) {
            auto sysTime = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
            fileMtimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(sysTime.time_since_epoch()).count();
        }

        auto it = lastProcessedMtime.find(mod);
        if (it != lastProcessedMtime.end() && fileMtimeMs > 0 && fileMtimeMs <= it->second) {
            std::cout << "[Hotfix] skip duplicate module in window: '" << mod << "' mtime=" << fileMtimeMs
                      << " lastProcessed=" << it->second << std::endl;
            continue;
        }

        if (!hotfixManger.packageSet.contains(mod)) {
            hotfixManger.packageSet.emplace(mod, currentTime);
            std::cout << "[Hotfix] packageSet.emplace: '" << mod << "' @" << currentTime << std::endl;
        } else {
            std::cout << "[Hotfix] packageSet already contains: '" << mod << "' (skip)" << std::endl;
        }
        if (fileMtimeMs > 0) {
            lastProcessedMtime[mod] = fileMtimeMs;
        }
    }
}

std::wstring PythonAPI::str2wstr(const std::string& str) {
    if (str.empty()) {
        return {};
    }
    int wlen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
    if (wlen <= 0) {
        return {};
    }
    std::wstring w(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), w.data(), wlen);
    return w;
}

void PythonAPI::copyModifiedFiles(const std::filesystem::path& sourceDir,
                                  const std::filesystem::path& destDir,
                                  int64_t checkTimeMs) {
    static const std::set<std::string> skip = {
        "__pycache__", "__init__.py", ".pyc", "StaticComponents.py"};
    static std::unordered_map<std::string, int64_t> lastCopiedMtime;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto& filePath = entry.path();
        std::string fileName = filePath.filename().string();
        if (!PythonHotfix::EndsWith(fileName, ".py")) {
            continue;
        }
        bool skipFile = std::ranges::any_of(skip, [&](const std::string& s) {
            return PythonHotfix::EndsWith(fileName, s);
        });
        if (skipFile) {
            continue;
        }

        try {
            auto ftime = std::filesystem::last_write_time(filePath);
            auto sysTime = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
            int64_t modifyMs = std::chrono::duration_cast<std::chrono::milliseconds>(sysTime.time_since_epoch()).count();

            auto srcKey = filePath.string();
            auto it = lastCopiedMtime.find(srcKey);
            bool newerThanLastCopy = (it == lastCopiedMtime.end()) || (modifyMs > it->second);
            if (checkTimeMs - modifyMs <= PythonHotfix::kFileRecentWindowMs && newerThanLastCopy) {
                auto relativePath = std::filesystem::relative(filePath, sourceDir);
                auto destFilePath = destDir / relativePath;
                std::filesystem::create_directories(destFilePath.parent_path());
                std::filesystem::copy_file(filePath, destFilePath,
                                           std::filesystem::copy_options::overwrite_existing);
                std::filesystem::last_write_time(destFilePath, ftime);

                lastCopiedMtime[srcKey] = modifyMs;

                std::string modName = destFilePath.string();
                PythonHotfix::NormalizeModuleName(modName);
                std::cout << "[Hotfix] copied recent file: " << filePath.string()
                          << " -> " << destFilePath.string()
                          << ", module='" << modName << "' src_mtime=" << modifyMs << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "File copy error: " << e.what() << std::endl;
        }
    }
}
}  // namespace Corona::Script::Python
