#define PY_SSIZE_T_CLEAN
#include <corona/script/PythonAPI.h>
#include <windows.h>
#include <cstdlib>

#include <corona_logger.h>
#include <regex>
#include <iostream>
#include <set>
#include <ranges>
#include <unordered_map>

// Helper: safe DECREF with diagnostics for API layer
static inline void APISafeDecRef(PyObject* obj, const char* tag) {
    if (!obj) { std::cout << "[Hotfix][API] SafeDecRef skip null: " << tag << std::endl; return; }
    Py_ssize_t rc = Py_REFCNT(obj);
    int64_t rc64 = static_cast<int64_t>(rc);
    std::cout << "[Hotfix][API] SafeDecRef attempt '" << tag << "' ptr=" << obj << " refcnt=" << rc64 << std::endl;
    if (rc64 <= 0 || rc64 > 1000000000LL) {
        std::cout << "[Hotfix][API] SafeDecRef suspicious refcnt, skip: " << tag << std::endl; return;
    }
    Py_DECREF(obj);
}

namespace CE::Python::Internal {
    struct PyObjPtr {
        PyObject* p = nullptr;
        PyObjPtr() = default;
        explicit PyObjPtr(PyObject* obj): p(obj) {}
        ~PyObjPtr(){ Py_XDECREF(p); }
        [[nodiscard]] PyObject* get() const { return p; }
        PyObject* release() { PyObject* t = p; p = nullptr; return t; }
        PyObjPtr(const PyObjPtr&) = delete;
        PyObjPtr& operator=(const PyObjPtr&) = delete;
        PyObjPtr(PyObjPtr&& o) noexcept { p = o.p; o.p = nullptr; }
        PyObjPtr& operator=(PyObjPtr&& o) noexcept {
            if (this != &o) { Py_XDECREF(p); p = o.p; o.p = nullptr; }
            return *this;
        }
    };
    struct GILGuard {
        PyGILState_STATE state;
        GILGuard(){ state = PyGILState_Ensure(); }
        ~GILGuard(){ PyGILState_Release(state); }
    };
} // namespace CE::Python::Internal

using CE::Python::Internal::PyObjPtr;
using CE::Python::Internal::GILGuard;

// Unified path configuration
namespace PathCfg {
    inline std::string Normalize(std::string s) {
        std::ranges::replace(s, '\\', '/');
        return s;
    }

    inline const std::string& EngineRoot() {
        static std::string root = []{
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
        static const std::string abs = []{ return Normalize(EngineRoot() + "/" + EditorBackendRel()); }();
        return abs;
    }

    inline std::string RuntimeBackendAbs() {
        auto p = std::filesystem::current_path() / "CabbageEditor" / "Backend";
        return Normalize(p.string());
    }

    inline std::string SitePackagesDir() {
        return Normalize(std::string(CORONA_PYTHON_MODULE_LIB_DIR) + "/site-packages");
    }
} // namespace PathCfg

const std::string PythonAPI::codePath = PathCfg::EngineRoot();

PyObject *PyInit_CoronaEngineEmbedded()
{
    PyMethodDef CoronaEngineMethods[] = {{nullptr, nullptr, 0, nullptr}};

    if (PyType_Ready(&EngineScripts::ActorScripts::PyActorType) < 0) { return nullptr; }
    if (PyType_Ready(&EngineScripts::SceneScripts::PySceneType) < 0) { return nullptr; }

    static PyModuleDef module{};
    module.m_base = PyModuleDef_HEAD_INIT;
    module.m_name = "CoronaEngine";
    module.m_methods = CoronaEngineMethods;
    module.m_size = -1;

    auto m = PyModule_Create(&module);
    if (!m) { return nullptr; }

    Py_INCREF(&EngineScripts::ActorScripts::PyActorType);
    if (PyModule_AddObject(m, "Actor", reinterpret_cast<PyObject *>(&EngineScripts::ActorScripts::PyActorType)) < 0) {
        Py_DECREF(&EngineScripts::ActorScripts::PyActorType);
        Py_DECREF(m);
        return nullptr;
    }

    Py_INCREF(&EngineScripts::SceneScripts::PySceneType);
    if (PyModule_AddObject(m, "Scene", reinterpret_cast<PyObject *>(&EngineScripts::SceneScripts::PySceneType)) < 0) {
        Py_DECREF(&EngineScripts::SceneScripts::PySceneType);
        Py_DECREF(m);
        return nullptr;
    }

    return m;
}

PythonAPI::PythonAPI() {
    // Initialize leak-safe from environment variable: CORONA_PY_LEAKSAFE
    // Accepts: 1/true/on/yes to enable; 0/false/off/no to disable; default true if unset.
    if (const char* env = std::getenv("CORONA_PY_LEAKSAFE")) {
        std::string v = env; std::ranges::transform(v, v.begin(), ::tolower);
        if (v == "1" || v == "true" || v == "on" || v == "yes") leakSafeMainReload_ = true;
        else if (v == "0" || v == "false" || v == "off" || v == "no") leakSafeMainReload_ = false;
    }
    std::cout << "[Hotfix][API] leak-safe default=" << (leakSafeMainReload_ ? "ON" : "OFF")
              << " (CORONA_PY_LEAKSAFE env)" << std::endl;
}

void PythonAPI::setLeakSafeReload(bool enabled) {
    if (leakSafeMainReload_ == enabled) return;
    leakSafeMainReload_ = enabled;
    std::cout << "[Hotfix][API] leak-safe toggled to " << (enabled ? "ON" : "OFF") << std::endl;
}

bool PythonAPI::isLeakSafeReload() const { return leakSafeMainReload_; }

PythonAPI::~PythonAPI()
{
    if (Py_IsInitialized()) {
        // 清理 PyObject 引用需在持有 GIL 的短作用域内完成
        {
            GILGuard guard;
            if (!leakSafeMainReload_) {
                Py_XDECREF(messageFunc);
                Py_XDECREF(pFunc);
                Py_XDECREF(pModule);
            } else {
                std::cout << "[Hotfix][API] leak-safe: skip DECREF in destructor" << std::endl;
            }
            // 防止悬垂
            messageFunc = nullptr;
            pFunc = nullptr;
            pModule = nullptr;
        }
        // 释放 GIL 之后再 Finalize，避免在 finalizing 状态下释放 GIL 触发致命错误
        Py_FinalizeEx();
    }
    PyConfig_Clear(&config);
}

int64_t PythonAPI::nowMsec()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool PythonAPI::ensureInitialized()
{
    if (Py_IsInitialized()) { return true; }

    PyImport_AppendInittab("CoronaEngine", &PyInit_CoronaEngineEmbedded);

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
        if (PyErr_Occurred()) { PyErr_Print(); }
        return false;
    }

    {
        GILGuard gil;
        pModule = PyImport_ImportModule("main");
        if (!pModule) { if (PyErr_Occurred()) { PyErr_Print(); } return false; }
        pFunc = PyObject_GetAttrString(pModule, "run");
        messageFunc = PyObject_GetAttrString(pModule, "put_queue");
        if (!pFunc || !messageFunc || !PyCallable_Check(pFunc)) {
            if (PyErr_Occurred()) { PyErr_Print(); }
            Py_XDECREF(pFunc); pFunc = nullptr;
            Py_XDECREF(messageFunc); messageFunc = nullptr;
            Py_XDECREF(pModule); pModule = nullptr;
            return false;
        }
    }
    return true;
}

bool PythonAPI::performHotReload() {
    int64_t currentTime = PythonHotfix::GetCurrentTimeMsec(); // ms
    constexpr int64_t kHotReloadIntervalMs = 100; // 100ms
    if (currentTime - lastHotReloadTime <= kHotReloadIntervalMs || hotfixManger.packageSet.empty()) {
        return false;
    }

    std::cout << "[Hotfix] performHotReload triggered. packageSet.size=" << hotfixManger.packageSet.size() << std::endl;

    bool reloadedDeps = true;
    if (leakSafeMainReload_) {
        std::cout << "[Hotfix][API] leak-safe: skip ReloadPythonFile (submodules), will reload 'main' only" << std::endl;
        hotfixManger.packageSet.clear();
        hotfixManger.dependencyGraph.clear();
        hotfixManger.dependencyVec.clear();
    } else {
        reloadedDeps = hotfixManger.ReloadPythonFile();
        if (!reloadedDeps) {
            std::cout << "[Hotfix] hotfixManger.ReloadPythonFile returned false" << std::endl;
            return false;
        }
    }

    GILGuard gil;
    std::cout << "[Hotfix] reloading 'main' module (via importlib.reload)" << std::endl;

    PyObject* importlib = PyImport_ImportModule("importlib");
    PyObject* reload_func = importlib ? PyObject_GetAttrString(importlib, "reload") : nullptr;
    if (!reload_func) {
        std::cout << "[Hotfix][API] importlib.reload not available, fallback to PyImport_ReloadModule" << std::endl;
    }

    PyObject* sys_modules = PyImport_GetModuleDict();
    bool from_sys_modules = false;
    PyObject* mod = nullptr;
    if (sys_modules) {
        mod = PyDict_GetItemString(sys_modules, "main"); // borrowed
        if (mod) from_sys_modules = true;
    }
    if (!mod) {
        mod = PyImport_ImportModule("main"); // new ref
        from_sys_modules = false;
    }
    if (!mod || !PyModule_Check(mod)) {
        std::cout << "[Hotfix][API] failed to get 'main' module for reload" << std::endl;
        if (PyErr_Occurred()) PyErr_Print();
        if (reload_func && !leakSafeMainReload_) Py_XDECREF(reload_func);
        if (importlib && !leakSafeMainReload_) Py_XDECREF(importlib);
        return false;
    }

    std::cout << "[Hotfix][API] main module acquired ptr=" << mod
              << (from_sys_modules ? " (borrowed)" : " (owned)") << std::endl;

    PyObject* new_mod = nullptr;
    if (reload_func) {
        new_mod = PyObject_CallFunctionObjArgs(reload_func, mod, nullptr);
    } else {
        new_mod = PyImport_ReloadModule(mod);
    }
    if (!new_mod) {
        std::cout << "[Hotfix][API] reload(main) failed" << std::endl;
        if (PyErr_Occurred()) PyErr_Print();
        if (!from_sys_modules && !leakSafeMainReload_) { APISafeDecRef(mod, "main.mod(owned, reload fail)"); }
        if (reload_func && !leakSafeMainReload_) Py_XDECREF(reload_func);
        if (importlib && !leakSafeMainReload_) Py_XDECREF(importlib);
        return false;
    }

    std::cout << "[Hotfix][API] reload(main) ok: new_mod=" << new_mod << " old_mod=" << mod
              << (new_mod == mod ? " (same)" : " (different)") << std::endl;

    // In leak-safe mode, acquire a fresh owned reference for 'main' to avoid INCREF on borrowed
    if (leakSafeMainReload_) {
        PyObject* owned_main = PyImport_ImportModule("main"); // new ref
        if (owned_main && PyModule_Check(owned_main)) {
            std::cout << "[Hotfix][API] leak-safe: acquired owned 'main' via ImportModule ptr=" << owned_main << std::endl;
            mod = owned_main; // take ownership; no DECREF in leak-safe mode
            from_sys_modules = false;
        } else {
            if (owned_main) { Py_XDECREF(owned_main); }
            std::cout << "[Hotfix][API] leak-safe: failed to acquire owned 'main', fallback to borrowed" << std::endl;
        }
    }

    // After reload, re-fetch main from sys.modules to ensure up-to-date module object
    if (sys_modules && !leakSafeMainReload_) {
        PyObject* mod2 = PyDict_GetItemString(sys_modules, "main"); // borrowed
        if (mod2 && PyModule_Check(mod2)) {
            std::cout << "[Hotfix][API] refreshed 'main' from sys.modules ptr=" << mod2 << std::endl;
            mod = mod2;
            from_sys_modules = true;
        }
    }

    // After reload, get new attributes from the (reloaded) module object.
    PyObjPtr newFunc(PyObject_GetAttrString(mod, "run"));
    if (!newFunc.get() || !PyCallable_Check(newFunc.get())) {
        std::cout << "[Hotfix][API] new run attr invalid" << std::endl;
        if (PyErr_Occurred()) PyErr_Print();
        if (!from_sys_modules && !leakSafeMainReload_) {
            if (new_mod != mod) APISafeDecRef(new_mod, "main.new_mod(owned, attr fail)");
            APISafeDecRef(mod, "main.mod(owned, attr fail)");
        } else if (!leakSafeMainReload_) {
            if (new_mod != mod) APISafeDecRef(new_mod, "main.new_mod(borrowed, attr fail)");
        }
        if (reload_func && !leakSafeMainReload_) Py_XDECREF(reload_func);
        if (importlib && !leakSafeMainReload_) Py_XDECREF(importlib);
        return false;
    }

    PyObjPtr newMsg(PyObject_GetAttrString(mod, "put_queue"));
    if (!newMsg.get()) {
        std::cout << "[Hotfix][API] new put_queue missing" << std::endl;
        if (PyErr_Occurred()) PyErr_Print();
        if (!from_sys_modules && !leakSafeMainReload_) {
            if (new_mod != mod) APISafeDecRef(new_mod, "main.new_mod(owned, msg fail)");
            APISafeDecRef(mod, "main.mod(owned, msg fail)");
        } else if (!leakSafeMainReload_) {
            if (new_mod != mod) APISafeDecRef(new_mod, "main.new_mod(borrowed, msg fail)");
        }
        if (reload_func && !leakSafeMainReload_) Py_XDECREF(reload_func);
        if (importlib && !leakSafeMainReload_) Py_XDECREF(importlib);
        return false;
    }

    // Swap in new bindings; manage refs conservatively
    if (!leakSafeMainReload_) {
        APISafeDecRef(pModule, "old pModule");
        APISafeDecRef(pFunc, "old pFunc");
        APISafeDecRef(messageFunc, "old messageFunc");
    } else {
        std::cout << "[Hotfix][API] leak-safe: skip DECREF old pModule/pFunc/messageFunc" << std::endl;
    }

    if (!leakSafeMainReload_) {
        if (from_sys_modules) { Py_INCREF(mod); }
    } else {
        std::cout << "[Hotfix][API] leak-safe: skip INCREF main (borrowed)" << std::endl;
    }
    pModule = mod; // now owned if INCREFed (or borrowed if leak-safe)
    pFunc = newFunc.release();
    messageFunc = newMsg.release();

    if (!leakSafeMainReload_) {
        if (new_mod != mod) { APISafeDecRef(new_mod, "main.new_mod post-swap"); }
    } else {
        std::cout << "[Hotfix][API] leak-safe: skip DECREF new_mod/importlib" << std::endl;
    }

    if (!leakSafeMainReload_) {
        Py_XDECREF(reload_func);
        Py_XDECREF(importlib);
    } else {
        std::cout << "[Hotfix][API] leak-safe: skip XDECREF(importlib, reload_func)" << std::endl;
    }

    lastHotReloadTime = currentTime;
    hasHotReload = true;
    std::cout << "[Hotfix] performHotReload finished successfully" << std::endl;
    return true;
}

void PythonAPI::invokeEntry(bool isReload) const
{
    if (!pFunc) { return; }
    GILGuard gil;

    PyObjPtr result(PyObject_CallFunction(pFunc, "i", isReload ? 1 : 0));
    if (!result.get() && PyErr_Occurred()) {
        PyErr_Print();
    }
}

void PythonAPI::sendMessage(const std::string &message) const
{
    if (!messageFunc) { return; }
    GILGuard gil;

    PyObjPtr result(PyObject_CallFunction(messageFunc, "s", message.c_str()));
    if (!result.get() && PyErr_Occurred()) { PyErr_Print(); }
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

void PythonAPI::checkPythonScriptChange()
{
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
    static std::unordered_map<std::string, int64_t> lastProcessedMtime; // mod -> last mtime processed

    int64_t currentTime = PythonHotfix::GetCurrentTimeMsec();
    if (currentTime - lastCheckTime < 100) { return; }
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
    for (const auto& m : mods) { if (!first) std::cout << ", "; std::cout << m; first = false; }
    std::cout << std::endl;

    auto modToPath = [&](const std::string& mod){
        std::string rel = mod; // replace '.' with '/'
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
        if (fileMtimeMs > 0) { lastProcessedMtime[mod] = fileMtimeMs; }
    }
}

std::wstring PythonAPI::str2wstr(const std::string& str) {
    if (str.empty()) { return {}; }
    int wlen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
    if (wlen <= 0 ) { return {}; }
    std::wstring w(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), w.data(), wlen);
    return w;
}

void PythonAPI::copyModifiedFiles(const std::filesystem::path& sourceDir,
                                  const std::filesystem::path& destDir,
                                  int64_t checkTimeMs)
{
    static const std::set<std::string> skip = {
        "__pycache__", "__init__.py", ".pyc", "StaticComponents.py"
    };
    static std::unordered_map<std::string, int64_t> lastCopiedMtime;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir)) {
        if (!entry.is_regular_file()) { continue; }
        const auto& filePath = entry.path();
        std::string fileName = filePath.filename().string();
        if (!PythonHotfix::EndsWith(fileName, ".py")) { continue; }
        bool skipFile = std::ranges::any_of(skip, [&](const std::string& s){
            return PythonHotfix::EndsWith(fileName, s);
        });
        if (skipFile) { continue; }

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
