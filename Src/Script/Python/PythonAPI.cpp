#define PY_SSIZE_T_CLEAN

#include <chrono>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <regex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>

#include "PythonAPI.h"

#include "Core/Log.h"

// #include"CabbageFramework/CabbageCommon/CabbageCommon.h"

const std::string PythonAPI::codePath =
[] {
    std::string resultPath = "";
    std::string runtimePath = std::filesystem::current_path().string();
    // std::replace(runtimePath.begin(), runtimePath.end(), '\\', '/');
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
    std::replace(resultPath.begin(), resultPath.end(), '\\', '/');
    return resultPath;
    }();


PyObject *PyInit_CoronaEngineEmbedded()
{
    PyMethodDef CoronaEngineMethods[] = {{NULL, NULL, 0, NULL}};

    if (PyType_Ready(&EngineScripts::ActorScripts::PyActorType) < 0)
    {
        std::cerr << "Py type ready error  - PyActorType" << std::endl;
        return nullptr;
    }
    if (PyType_Ready(&EngineScripts::SceneScripts::PySceneType) < 0)
    {
        std::cerr << "Py type ready error  - PySceneType" << std::endl;
        return nullptr;
    }

    static PyModuleDef module{};
    module.m_base = PyModuleDef_HEAD_INIT;
    module.m_name = "CoronaEngine";
    module.m_methods = CoronaEngineMethods;
    module.m_size = -1;
    module.m_doc = NULL;

    auto m = PyModule_Create(&module);

    Py_INCREF(&EngineScripts::ActorScripts::PyActorType);
    if (PyModule_AddObject(m, "Actor", (PyObject *)&EngineScripts::ActorScripts::PyActorType) < 0)
    {
        Py_DECREF(&EngineScripts::ActorScripts::PyActorType);
        Py_DECREF(m);
        return nullptr;
    }

    Py_INCREF(&EngineScripts::SceneScripts::PySceneType);
    if (PyModule_AddObject(m, "Scene", (PyObject *)&EngineScripts::SceneScripts::PySceneType) < 0)
    {
        Py_DECREF(&EngineScripts::SceneScripts::PySceneType);
        Py_DECREF(m);
        return nullptr;
    }

    return m;
}


PythonAPI::PythonAPI()
{
    PyConfig_InitPythonConfig(&config);

    hotreloadPath = PythonAPI::codePath + "/Editor/CoronaEditor/Backend";
    std::replace(hotreloadPath.begin(), hotreloadPath.end(), '\\', '/');
}

PythonAPI::~PythonAPI()
{
    if (Py_IsInitialized())
    {
        if (pFunc != nullptr) {
            Py_DECREF(pFunc);
            pFunc = nullptr;
        }
        if (pModule != nullptr) {
            Py_DECREF(pModule);
            pModule = nullptr;
        }

        Py_FinalizeEx();
    }

    // 只在最后清理一次配置
    PyConfig_Clear(&config);
}

void PythonAPI::runPythonScript()
{
    if (!Py_IsInitialized())
    {
        std::string runtimePath = PythonAPI::codePath + "/Editor/CoronaEditor/Backend";
        std::replace(runtimePath.begin(), runtimePath.end(), '\\', '/');

        PyImport_AppendInittab("CoronaEngine", &PyInit_CoronaEngineEmbedded);

        PyConfig_InitPythonConfig(&config);
        PyConfig_SetBytesString(&config, &config.home, CORONA_PYTHON_HOME_DIR);
        PyConfig_SetBytesString(&config, &config.pythonpath_env, CORONA_PYTHON_HOME_DIR);
        config.module_search_paths_set = 1;
        PyWideStringList_Append(&config.module_search_paths, str2wstr(runtimePath).c_str());
        PyWideStringList_Append(&config.module_search_paths, str2wstr(CORONA_PYTHON_MODULE_DLL_DIR).c_str());
        PyWideStringList_Append(&config.module_search_paths, str2wstr(CORONA_PYTHON_MODULE_LIB_DIR).c_str());
        PyWideStringList_Append(&config.module_search_paths, str2wstr(std::string(CORONA_PYTHON_MODULE_LIB_DIR) + "/site-packages").c_str());

        Py_InitializeFromConfig(&config);

        if (Py_IsInitialized())
        {
            pModule = PyImport_ImportModule("main");
            if (PyErr_Occurred())
            {
                PyErr_Print();
            }
            if (pModule)
            {
                pFunc = PyObject_GetAttrString(pModule, "run");

                PyObject *global_dict = PyModule_GetDict(pModule);

                if (global_dict)
                {
                    Py_ssize_t pos = 0;
                    PyObject *key;
                    PyObject *value;

                    // 传入 &value，确保 value 被填充
                    while (PyDict_Next(global_dict, &pos, &key, &value))
                    {
                        if (!key || !value)
                            continue;
                        const char *name = PyUnicode_AsUTF8(key);
                        if (!name)
                            continue;
                        if (PyModule_Check(value))
                        {
                            moduleList.push_back(name);
                        }
                        else if (PyCallable_Check(value))
                        {
                            callableList.push_back(name);
                        }
                    }
                }
            }
            else
            {
                if (PyErr_Occurred())
                {
                    PyErr_Print();
                }
                throw("Python: import error.");
            }
        }
        else
        {
            if (PyErr_Occurred())
            {
                PyErr_Print();
            }
            throw("Python: init error.");
        }
    }

    if (pModule && pFunc)
    {
        try
        {
            bool isReload = false;
            queMtx.lock();
            long long currentTime = hotfixManger.GetCurrentTimeMsec();
            if (currentTime - lastHotReloadTime > 1 && !hotfixManger.packageSet.empty())
            {
                if (hotfixManger.ReloadPythonFile())
                {
                    isReload = true;
                    PyObject *new_module = PyImport_ReloadModule(pModule);
                    if (new_module)
                    {
                        PyObject *old_module = pModule;
                        PyObject *old_func = pFunc;

                        pModule = new_module;
                        pFunc = PyObject_GetAttrString(pModule, "run");

                        // 只有在获取新函数成功后才释放旧对象
                        if (pFunc) {
                            Py_XDECREF(old_module);
                            Py_XDECREF(old_func);
                        } else {
                            // 如果获取新函数失败，恢复旧状态
                            pModule = old_module;
                            pFunc = old_func;

                            if (PyErr_Occurred()) {
                                PyErr_Print();
                            }
                        }
                    }
                    else
                    {
                        if (PyErr_Occurred()) {
                            PyErr_Print();
                        }
                    }
                    lastHotReloadTime = currentTime;
                    hasHotReload = true;
                }
                else
                {
                    hasHotReload = false;
                }
            }
            else if (!hotfixManger.packageSet.empty())
            {
                hasHotReload = false;
            }
            queMtx.unlock();

            PyObject *pArg = nullptr;
            PyObject *result = nullptr;
            PyObject *pBool = nullptr;

            do {
                // 只在解释器初始化时创建Python对象
                if (!Py_IsInitialized()) {
                    break;
                }

                pArg = PyTuple_New(1);
                if (!pArg) {
                    break;
                }

                pBool = PyBool_FromLong(isReload);
                if (!pBool) {
                    // PyBool_FromLong失败时需要手动释放pBool
                    Py_DECREF(pBool);
                    pArg = nullptr;
                    break;
                }

                if (PyTuple_SetItem(pArg, 0, pBool) != 0) {
                    // PyTuple_SetItem失败时需要手动释放pBool
                    Py_DECREF(pBool);
                    Py_DECREF(pArg);
                    pBool = nullptr;
                    pArg = nullptr;
                    break;
                }
                pBool = nullptr;

                // 执行函数调用
                result = PyObject_CallObject(pFunc, pArg);

                // 标记pArg不再需要我们管理（已被函数使用）
                pArg = nullptr;

                if (!result) {
                    if (PyErr_Occurred()) {
                        PyErr_Print();
                    }
                }
            } while (false);

            // 安全释放所有资源
            if (Py_IsInitialized()) {
                if (pArg != nullptr) {
                    Py_XDECREF(pArg);
                    pArg = nullptr;
                }
                if (result != nullptr) {
                    Py_XDECREF(result);
                    result = nullptr;
                }
                // pBool此时应为nullptr，但为了安全仍进行检查
                if (pBool != nullptr) {
                    Py_XDECREF(pBool);
                    pBool = nullptr;
                }
            }
        }
        catch (...)
        {
            std::cout << "python error!" << std::endl;
        }
    }
}

void PythonAPI::checkPythonScriptChange()
{
#if CABBAGE_ENGINE_DEBUG
    std::string runtimePath = "./Resource/CabbageEditorBackend";
    long long checkTime = hotfixManger.GetCurrentTimeMsec();
    copyModifiedFiles(hotreloadPath, runtimePath, checkTime);
#endif
}
// bool PythonAPI::s_tzdbInit = false;
//
// void PythonAPI::Init() {
//     // 提前初始化时区数据库，避免在业务线程中首次触发
//     if (!s_tzdbInit) {
//         try {
//             // 触发时区数据库初始化但不实际使用结果
//             std::chrono::get_tzdb();
//             s_tzdbInit = true;
//         } catch (...) {
//             // 忽略初始化失败的异常
//         }
//     }
// }

void PythonAPI::checkReleaseScriptChange()
{
    static long long lastCheckTime = 0;
    long long currentTime = hotfixManger.GetCurrentTimeMsec();

    // 至少间隔100毫秒才进行一次检查（可根据实际情况调整）
    if (currentTime - lastCheckTime < 100) {
        return;
    }

    lastCheckTime = currentTime;

    std::queue<std::unordered_set<std::string>> messageQue;
    std::string runtimePath = PythonAPI::codePath + "/Editor/CoronaEditor/Backend";
    std::replace(runtimePath.begin(), runtimePath.end(), '\\', '/');

    hotfixManger.TraverseDirectory(runtimePath, messageQue, currentTime);

    queMtx.lock();
    if (!messageQue.empty())
    {
        hotfixManger.packageSet = messageQue.front();
        messageQue.pop();
    }
    queMtx.unlock();
}

std::wstring PythonAPI::str2wstr(const std::string &str)
{
    // 获取所需的宽字符数组大小
    size_t size_needed = mbstowcs(nullptr, str.c_str(), 0) + 1;

    // 分配足够的空间
    std::vector<wchar_t> buffer(size_needed);

    // 执行转换
    mbstowcs(&buffer[0], str.c_str(), size_needed);

    // 创建wstring并返回
    return std::wstring(buffer.begin(), buffer.end() - 1); // 减1是为了去掉结尾的null字符
}

void PythonAPI::copyModifiedFiles(const std::filesystem::path &sourceDir, const std::filesystem::path &destDir, long long checkTime)
{
    //  auto startTime = std::chrono::high_resolution_clock::now();
    //  int pyFileCount = 0;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(sourceDir))
    {
        const std::filesystem::path &filePath = entry.path();
        std::string fileName = filePath.filename().string();

        if (entry.is_directory())
            continue;
        if (!hotfixManger.endsWith(fileName, ".py"))
            continue;

        const std::set<std::string> skipSuffixes = {"__pycache__", "__init__.py", ".pyc", "StaticComponents.py"};
        bool shouldSkip = false;
        for (const auto &suffix : skipSuffixes)
        {
            if (hotfixManger.endsWith(fileName, suffix))
            {
                shouldSkip = true;
                break;
            }
        }
        if (shouldSkip)
        {
            continue;
        }
        //  pyFileCount++;
        try
        {
            if (std::filesystem::exists(filePath))
            {
                long long modifyTime = std::chrono::system_clock::to_time_t(std::chrono::clock_cast<std::chrono::system_clock>(std::filesystem::last_write_time(filePath)));
                if (checkTime - modifyTime <= 1)
                {
                    //  auto startCopyTime = std::chrono::high_resolution_clock::now();
                    std::filesystem::path relativePath = std::filesystem::relative(filePath, sourceDir);
                    std::filesystem::path destFilePath = destDir / relativePath;

                    std::filesystem::create_directories(destFilePath.parent_path());
                    std::filesystem::copy_file(filePath, destFilePath, std::filesystem::copy_options::overwrite_existing);
                    //  auto endCopyTime = std::chrono::high_resolution_clock::now();
                    //  auto CopyTime = std::chrono::duration_cast<std::chrono::milliseconds>(endCopyTime - startCopyTime).count();
                    //  std::cout << "Python SourceCode Copy took: " << CopyTime << " ms." << std::endl;
                }
            }
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            // Catch specific filesystem_error
            std::cerr << "Filesystem error occurred!" << std::endl;
            std::cerr << "What: " << e.what() << std::endl;       // General description
            std::cerr << "Path 1: " << e.path1() << std::endl;    // Path involved in the error
            std::cerr << "Error code: " << e.code() << std::endl; // System-specific error code
        }
        catch (const std::exception &e)
        {
            // Catch other standard exceptions (e.g., std::bad_alloc for memory issues)
            std::cerr << "General error occurred: " << e.what() << std::endl;
        }
    }
    //  auto endTime = std::chrono::high_resolution_clock::now();
    //  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    //  std::cout << "Python SourceCode TraverseDirectory took: " << duration << " ms." << std::endl;
    //  std::cout << "Python SourceCode file count: " << pyFileCount << std::endl;
}