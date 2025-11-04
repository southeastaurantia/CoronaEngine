#define PY_SSIZE_T_CLEAN
#include <corona/python/python_api.h>
#include <windows.h>

#include <entt/signal/sigh.hpp>
#include <iostream>
#include <ranges>
#include <regex>
#include <set>
#include <unordered_map>

namespace {
inline void log_nanobind_python_error(const nanobind::python_error& e, const char* where = nullptr) {
    try {
        nanobind::gil_scoped_acquire gil;
        nanobind::module_ traceback = nanobind::module_::import_("traceback");
        nanobind::object format_exception = nanobind::getattr(traceback, "format_exception");
        auto formatted = format_exception(e.type(), e.value(), e.traceback());
        nanobind::str sep("");
        auto joined = nanobind::getattr(sep, "join")(formatted);
        auto out = nanobind::cast<std::string>(joined);
        if (where && *where) {
            std::cerr << "[Python Error][" << where << "] " << out << std::endl;
        } else {
            std::cerr << "[Python Error] " << out << std::endl;
        }
    } catch (const std::exception&) {
        // 回退：至少打印 e.what()
        if (where && *where) {
            std::cerr << "[Python Error][" << where << "] " << e.what() << std::endl;
        } else {
            std::cerr << "[Python Error] " << e.what() << std::endl;
        }
    }
}

inline void log_current_pyerr(const char* where = nullptr) {
    if (!PyErr_Occurred()) return;
    PyObject* ptype = nullptr;
    PyObject* pvalue = nullptr;
    PyObject* ptb = nullptr;
    PyErr_Fetch(&ptype, &pvalue, &ptb);
    PyErr_NormalizeException(&ptype, &pvalue, &ptb);
    PyObject* tb = PyImport_ImportModule("traceback");
    if (tb) {
        PyObject* fmt = PyObject_GetAttrString(tb, "format_exception");
        if (fmt && PyCallable_Check(fmt)) {
            PyObject* args = PyTuple_Pack(3, ptype ? ptype : Py_None, pvalue ? pvalue : Py_None, ptb ? ptb : Py_None);
            PyObject* formatted = PyObject_CallObject(fmt, args);
            if (formatted) {
                PyObject* sep = PyUnicode_FromString("");
                if (sep) {
                    PyObject* joined = PyUnicode_Join(sep, formatted);
                    if (joined) {
                        const char* s = PyUnicode_AsUTF8(joined);
                        if (s) {
                            if (where && *where) {
                                std::cerr << "[Python Error][" << where << "] " << s << std::endl;
                            } else {
                                std::cerr << "[Python Error] " << s << std::endl;
                            }
                        }
                        Py_DECREF(joined);
                    }
                    Py_DECREF(sep);
                }
                Py_DECREF(formatted);
            }
            Py_DECREF(args);
        }
        Py_XDECREF(fmt);
        Py_DECREF(tb);
    }
    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptb);
}
}  // namespace

extern "C" PyObject* PyInit_CoronaEngine();

namespace PathCfg {
inline std::string Normalize(std::string s) {
    std::ranges::replace(s, '\\', '/');
    return s;
}

inline const std::string& EngineRoot() {
    static std::string root = [] {
        std::string resultPath;
        std::string runtimePath = std::filesystem::current_path().string();
        std::regex pattern(R"((.*)CoronaEngine\b)");
        std::smatch matches;
        if (std::regex_search(runtimePath, matches, pattern)) {
            if (matches.size() > 1) {
                resultPath = matches[1].str() + "CoronaEngine";
            } else {
                throw std::runtime_error("Failed to resolve source path.");
            }
        }
        return Normalize(resultPath);
    }();
    return root;
}

inline const std::string& EditorBackendRel() {
    static const std::string rel = "Editor/CabbageEditor/Backend";
    return rel;
}

inline const std::string& EditorBackendAbs() {
    static const std::string abs = [] { return Normalize(EngineRoot() + "/" + EditorBackendRel()); }();
    return abs;
}

inline std::string RuntimeBackendAbs() {
    auto p = std::filesystem::current_path() / "CabbageEditor" / "Backend";
    return Normalize(p.string());
}

inline std::string SitePackagesDir() {
    return Normalize(std::string(CORONA_PYTHON_MODULE_LIB_DIR) + "/site-packages");
}
}  // namespace PathCfg

const std::string PythonAPI::codePath = PathCfg::EngineRoot();

PythonAPI::PythonAPI() {
    logger = Corona::Kernel::create_logger();
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
        std::string runtimePath = PathCfg::RuntimeBackendAbs();
        PyWideStringList_Append(&config.module_search_paths, str2wstr(runtimePath).c_str());
        PyWideStringList_Append(&config.module_search_paths, str2wstr(CORONA_PYTHON_MODULE_DLL_DIR).c_str());
        PyWideStringList_Append(&config.module_search_paths, str2wstr(CORONA_PYTHON_MODULE_LIB_DIR).c_str());
        PyWideStringList_Append(&config.module_search_paths, str2wstr(PathCfg::SitePackagesDir()).c_str());
    }

    Py_InitializeFromConfig(&config);

    if (!Py_IsInitialized()) {
        // 可能没有挂起的 PyErr，这里打印一个提示并尝试输出现有错误
        std::cerr << "[Python Init] Py_InitializeFromConfig failed" << std::endl;
        if (PyErr_Occurred()) {
            log_current_pyerr("Py_InitializeFromConfig");
        }
        return false;
    }

    {
        nanobind::gil_scoped_acquire gil;
        try {
            nanobind::module_ main_mod = nanobind::module_::import_("main");
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
            log_nanobind_python_error(e, "ensureInitialized/import main");
            pModule.reset();
            pFunc.reset();
            messageFunc.reset();
            return false;
        } catch (const std::exception& e) {
            std::cerr << "[Hotfix][API] ensureInitialized std::exception: " << e.what() << std::endl;
            if (PyErr_Occurred()) log_current_pyerr("ensureInitialized");
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
        log_nanobind_python_error(e, "performHotReload");
        return false;
    } catch (const std::exception& e) {
        std::cout << "[Hotfix][API] reload(main) failed (std::exception): " << e.what() << std::endl;
        if (PyErr_Occurred()) log_current_pyerr("performHotReload");
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
        log_nanobind_python_error(e, "invokeEntry");
    } catch (const std::exception& e) {
        std::cerr << "[Hotfix][API] invokeEntry std::exception: " << e.what() << std::endl;
        if (PyErr_Occurred()) log_current_pyerr("invokeEntry");
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
        log_nanobind_python_error(e, "sendMessage");
    } catch (const std::exception& e) {
        std::cerr << "[Hotfix][API] sendMessage std::exception: " << e.what() << std::endl;
        if (PyErr_Occurred()) log_current_pyerr("sendMessage");
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
    const std::string& sourcePath = PathCfg::EditorBackendAbs();
    const std::string runtimePath = PathCfg::RuntimeBackendAbs();
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
    const std::string runtimePathStr = PathCfg::RuntimeBackendAbs();
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
