#define PY_SSIZE_T_CLEAN
#include "PythonAPI.h"
#include <windows.h>

#include <Log.h>
#include <regex>
#include <iostream>
#include <set>
#include <ranges>

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

PythonAPI::PythonAPI() = default;

PythonAPI::~PythonAPI()
{
    if (Py_IsInitialized()) {
        GILGuard guard;
        Py_XDECREF(messageFunc);
        Py_XDECREF(pFunc);
        Py_XDECREF(pModule);
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

    if (!hotfixManger.ReloadPythonFile()) {
        return false;
    }

    GILGuard gil;
    PyObjPtr newMod(PyImport_ReloadModule(pModule));
    if (!newMod.get()) { if (PyErr_Occurred()) { PyErr_Print(); } return false; }

    PyObjPtr newFunc(PyObject_GetAttrString(newMod.get(), "run"));
    if (!newFunc.get() || !PyCallable_Check(newFunc.get())) {
        if (PyErr_Occurred()) { PyErr_Print(); }
        return false;
    }

    Py_XDECREF(pModule);
    Py_XDECREF(pFunc);
    Py_XDECREF(messageFunc);
    pModule = newMod.release();
    pFunc   = newFunc.release();
    messageFunc = PyObject_GetAttrString(pModule, "put_queue");

    lastHotReloadTime = currentTime;
    hasHotReload = true;
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

    // sendMessage("hello world");

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
    copyModifiedFiles(sourcePath, runtimePath, checkTime);
}

void PythonAPI::checkReleaseScriptChange() {
    static int64_t lastCheckTime = 0;
    int64_t currentTime = PythonHotfix::GetCurrentTimeMsec();
    if (currentTime - lastCheckTime < 100) { return; }
    lastCheckTime = currentTime;

    std::queue<std::unordered_set<std::string>> messageQue;
    const std::string runtimePath = PathCfg::RuntimeBackendAbs();
    PythonHotfix::TraverseDirectory(runtimePath, messageQue, currentTime);

    if (!messageQue.empty()) {
        std::unique_lock lk(queMtx);
        const auto& mods = messageQue.front();
        for (const auto& mod : mods) {
            if (!hotfixManger.packageSet.contains(mod)) {
                hotfixManger.packageSet.emplace(mod, currentTime);
            }
        }
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
            if (checkTimeMs - modifyMs <= PythonHotfix::kFileRecentWindowMs) {
                auto relativePath = std::filesystem::relative(filePath, sourceDir);
                auto destFilePath = destDir / relativePath;
                std::filesystem::create_directories(destFilePath.parent_path());
                std::filesystem::copy_file(filePath, destFilePath,
                    std::filesystem::copy_options::overwrite_existing);
            }
        } catch (const std::exception& e) {
            std::cerr << "File copy error: " << e.what() << std::endl;
        }
    }
}