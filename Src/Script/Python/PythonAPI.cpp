#define PY_SSIZE_T_CLEAN
#include "PythonAPI.h"
#include <Log.h>

namespace CE::Python::Internal {
    struct PyObjPtr {
        PyObject* p = nullptr;
        PyObjPtr() = default;
        explicit PyObjPtr(PyObject* obj): p(obj) {}
        ~PyObjPtr(){ Py_XDECREF(p); }
        PyObject* get() const { return p; }
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
}

using CE::Python::Internal::PyObjPtr;
using CE::Python::Internal::GILGuard;

const std::string PythonAPI::codePath =
[] {
    std::string resultPath = "";
    std::string runtimePath = std::filesystem::current_path().string();
    std::regex pattern(R"((.*)CoronaEngine\b)");
    std::smatch matches;
    if (std::regex_search(runtimePath, matches, pattern))
    {
        if (matches.size() > 1)
        {
            resultPath = matches[1].str() + "CoronaEngine";
        }
        else
        {
            throw std::runtime_error("Failed to resolve source path.");
        }
    }
    std::ranges::replace(resultPath, '\\', '/');
    return resultPath;
    }();


PyObject *PyInit_CoronaEngineEmbedded()
{
    PyMethodDef CoronaEngineMethods[] = {{nullptr, nullptr, 0, nullptr}};

    if (PyType_Ready(&EngineScripts::ActorScripts::PyActorType) < 0) return nullptr;
    if (PyType_Ready(&EngineScripts::SceneScripts::PySceneType) < 0) return nullptr;

    static PyModuleDef module{};
    module.m_base = PyModuleDef_HEAD_INIT;
    module.m_name = "CoronaEngine";
    module.m_methods = CoronaEngineMethods;
    module.m_size = -1;

    auto m = PyModule_Create(&module);
    if (!m) return nullptr;

    Py_INCREF(&EngineScripts::ActorScripts::PyActorType);
    if (PyModule_AddObject(m, "Actor", reinterpret_cast<PyObject *>(&EngineScripts::ActorScripts::PyActorType)) < 0)
    {
        Py_DECREF(&EngineScripts::ActorScripts::PyActorType);
        Py_DECREF(m);
        return nullptr;
    }

    Py_INCREF(&EngineScripts::SceneScripts::PySceneType);
    if (PyModule_AddObject(m, "Scene", reinterpret_cast<PyObject *>(&EngineScripts::SceneScripts::PySceneType)) < 0)
    {
        Py_DECREF(&EngineScripts::SceneScripts::PySceneType);
        Py_DECREF(m);
        return nullptr;
    }

    return m;
}


PythonAPI::PythonAPI()
{
    hotreloadPath = PythonAPI::codePath + "/Editor/CoronaEditor/Backend";
    std::ranges::replace(hotreloadPath, '\\', '/');
}

PythonAPI::~PythonAPI()
{
    if (Py_IsInitialized())
    {
        GILGuard guard;
        Py_XDECREF(pFunc);
        Py_XDECREF(pModule);
        Py_FinalizeEx();
    }
    PyConfig_Clear(&config);
}

long long PythonAPI::nowMsec()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool PythonAPI::ensureInitialized()
{
    if (Py_IsInitialized()) return true;

    PyImport_AppendInittab("CoronaEngine", &PyInit_CoronaEngineEmbedded);

    PyConfig_InitPythonConfig(&config);
    PyConfig_SetBytesString(&config, &config.home, CORONA_PYTHON_HOME_DIR);
    PyConfig_SetBytesString(&config, &config.pythonpath_env, CORONA_PYTHON_HOME_DIR);
    config.module_search_paths_set = 1;

    std::string runtimePath = codePath + "/Editor/CoronaEditor/Backend";
    std::ranges::replace(runtimePath, '\\', '/');
    PyWideStringList_Append(&config.module_search_paths, str2wstr(runtimePath).c_str());
    PyWideStringList_Append(&config.module_search_paths, str2wstr(CORONA_PYTHON_MODULE_DLL_DIR).c_str());
    PyWideStringList_Append(&config.module_search_paths, str2wstr(CORONA_PYTHON_MODULE_LIB_DIR).c_str());
    PyWideStringList_Append(&config.module_search_paths, str2wstr(std::string(CORONA_PYTHON_MODULE_LIB_DIR) + "/site-packages").c_str());

    Py_InitializeFromConfig(&config);

    if (!Py_IsInitialized()) {
        if (PyErr_Occurred()) PyErr_Print();
        return false;
    }

    {
        GILGuard gil;
        pModule = PyImport_ImportModule("main");
        if (!pModule) { if (PyErr_Occurred()) PyErr_Print(); return false; }
        pFunc = PyObject_GetAttrString(pModule, "run");
        if (!pFunc || !PyCallable_Check(pFunc)) {
            if (PyErr_Occurred()) PyErr_Print();
            Py_XDECREF(pFunc); pFunc = nullptr;
            return false;
        }
    }
    return true;
}

bool PythonAPI::performHotReload() {
    long long currentTime = hotfixManger.GetCurrentTimeMsec();
    if (currentTime - lastHotReloadTime <= 100 || hotfixManger.packageSet.empty())
        return false;

    if (!hotfixManger.ReloadPythonFile())
        return false;

    GILGuard gil;
    PyObjPtr newMod(PyImport_ReloadModule(pModule));
    if (!newMod.get()) { if (PyErr_Occurred()) PyErr_Print(); return false; }

    PyObjPtr newFunc(PyObject_GetAttrString(newMod.get(), "run"));
    if (!newFunc.get() || !PyCallable_Check(newFunc.get())) {
        if (PyErr_Occurred()) PyErr_Print();
        return false;
    }

    Py_XDECREF(pModule);
    Py_XDECREF(pFunc);
    pModule = newMod.release();
    pFunc   = newFunc.release();

    lastHotReloadTime = currentTime;
    hasHotReload = true;
    return true;
}

void PythonAPI::invokeEntry(bool isReload) {
    if (!pFunc) return;
    GILGuard gil;

    PyObjPtr result(PyObject_CallFunction(pFunc, "i", isReload ? 1 : 0));
    if (!result.get() && PyErr_Occurred()) {
        PyErr_Print();
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
        if (!reloaded && !hotfixManger.packageSet.empty())
            hasHotReload = false;
    }

    invokeEntry(reloaded);
}

void PythonAPI::checkPythonScriptChange()
{
#if CABBAGE_ENGINE_DEBUG
    std::string runtimePath = "./Resource/CabbageEditorBackend";
    long long checkTime = hotfixManger.GetCurrentTimeMsec();
    copyModifiedFiles(hotreloadPath, runtimePath, checkTime);
#endif
}

void PythonAPI::checkReleaseScriptChange() {
    static long long lastCheckTime = 0;
    long long currentTime = hotfixManger.GetCurrentTimeMsec();
    if (currentTime - lastCheckTime < 100) return;
    lastCheckTime = currentTime;

    std::queue<std::unordered_set<std::string>> messageQue;
    std::string runtimePath = codePath + "/Editor/CoronaEditor/Backend";
    std::ranges::replace(runtimePath, '\\', '/');
    hotfixManger.TraverseDirectory(runtimePath, messageQue, currentTime);

    if (!messageQue.empty()) {
        std::unique_lock lk(queMtx);
        hotfixManger.packageSet = std::move(messageQue.front());
    }
}

std::wstring PythonAPI::str2wstr(const std::string& str) {
    if (str.empty()) return {};
    int wlen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
    if (wlen <= 0 ) return {};
    std::wstring w(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), w.data(), wlen);
    return w;
}


void PythonAPI::copyModifiedFiles(const std::filesystem::path& sourceDir,
                                  const std::filesystem::path& destDir,
                                  long long checkTime)
{
    static const std::set<std::string> skip = {
        "__pycache__", "__init__.py", ".pyc", "StaticComponents.py"
    };

    for (auto const& entry : std::filesystem::recursive_directory_iterator(sourceDir)) {
        if (!entry.is_regular_file()) continue;
        const auto& filePath = entry.path();
        std::string fileName = filePath.filename().string();
        if (!hotfixManger.endsWith(fileName, ".py")) continue;
        bool skipFile = std::any_of(skip.begin(), skip.end(), [&](const std::string& s){
            return hotfixManger.endsWith(fileName, s);
        });
        if (skipFile) continue;

        try {
            long long modifyTime = std::chrono::system_clock::to_time_t(
                std::chrono::clock_cast<std::chrono::system_clock>(
                    std::filesystem::last_write_time(filePath)));
            if (checkTime - modifyTime <= 1) {
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