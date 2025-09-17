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

//#include"CabbageFramework/CabbageCommon/CabbageCommon.h"

const std::string PythonAPI::codePath =
[] {
    std::string resultPath = "";
    std::string runtimePath = std::filesystem::current_path().string();
    // std::replace(runtimePath.begin(), runtimePath.end(), '\\', '/');
    std::regex pattern(R"((.*)CabbageEngine\b)");
    std::smatch matches;
    if (std::regex_search(runtimePath, matches, pattern))
    {
        if (matches.size() > 1)
        {
            resultPath = matches[1].str() + "CabbageEngine";
        }
        else
        {
            throw std::runtime_error("Failed to resolve source path.");
        }
    }
    std::replace(resultPath.begin(), resultPath.end(), '\\', '/');
    return resultPath + "/SourceCode";
    }();


PyObject *PyInit_CabbageEngineEmbedded()
{
    PyMethodDef CabbageEngineMethods[] = {{NULL, NULL, 0, NULL}};

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
    module.m_name = "CabbageEngine";
    module.m_methods = CabbageEngineMethods;
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

    hotreloadPath = PythonAPI::codePath + "/CabbageEditor/CabbageEditorBackend";
    std::replace(hotreloadPath.begin(), hotreloadPath.end(), '\\', '/');
}

PythonAPI::~PythonAPI()
{
    Py_XDECREF(pModule);
    Py_XDECREF(pFunc);
    PyConfig_Clear(&config);
    Py_Finalize();
}

void PythonAPI::runPythonScript()
{
    if (!Py_IsInitialized())
    {
        std::string runtimePath = "./Resource/CabbageEditorBackend";
        std::string pythonPath = "./Resource/Python";

        PyImport_AppendInittab("CabbageEngine", &PyInit_CabbageEngineEmbedded);

        PyConfig_InitPythonConfig(&config);
        PyConfig_SetBytesString(&config, &config.home, pythonPath.c_str());
        PyConfig_SetBytesString(&config, &config.pythonpath_env, pythonPath.c_str());
        config.module_search_paths_set = 1;
        PyWideStringList_Append(&config.module_search_paths, str2wstr(runtimePath).c_str());
        PyWideStringList_Append(&config.module_search_paths, str2wstr(std::string(pythonPath + "/DLLs")).c_str());
        PyWideStringList_Append(&config.module_search_paths, str2wstr(std::string(pythonPath + "/Lib")).c_str());
        PyWideStringList_Append(&config.module_search_paths, str2wstr(std::string(pythonPath + "/Lib/site-packages")).c_str());

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
                PyObject *import_names = PyDict_Keys(global_dict);
                PyObject *import_values = PyDict_Values(global_dict);
                Py_ssize_t pos = 0;
                PyObject *key;

                while (PyDict_Next(global_dict, &pos, &key, nullptr))
                {
                    const char *name = PyUnicode_AsUTF8(key);
                    PyObject *value = PyDict_GetItem(global_dict, key);
                    if (PyModule_Check(value))
                    {
                        moduleList.push_back(name);
                    }
                    else
                    {
                        if (PyCallable_Check(value))
                        {
                            callableList.push_back(name);
                        }
                    }
                }
                Py_DECREF(import_names);
                Py_DECREF(import_values);
                Py_DECREF(global_dict);
                Py_DECREF(key);
            }
            else
            {
                if (PyErr_Occurred())
                {
                    PyErr_Print();
                }
                throw("Python: import error.");
            }
            Py_DECREF(pModule);
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
                    // auto startTime = std::chrono::high_resolution_clock::now();
                    isReload = true;
                    PyObject *new_module = PyImport_ReloadModule(pModule);
                    if (new_module)
                    {
                        Py_XDECREF(pModule);
                        Py_XDECREF(pFunc);
                        pModule = new_module;
                        pFunc = PyObject_GetAttrString(pModule, "run");
                        if (!pFunc)
                        {
                            if (PyErr_Occurred())
                            {
                                PyErr_Print();
                            }
                        }
                    }
                    else
                    {
                        if (PyErr_Occurred())
                        {
                            PyErr_Print();
                        }
                    }
                    //  auto endTime = std::chrono::high_resolution_clock::now();
                    //  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
                    //  std::cout << "Hot reload took " << duration << " ms." << std::endl;
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

            PyObject *pArg = PyTuple_New(1);
            PyTuple_SetItem(pArg, 0, PyBool_FromLong(isReload));
            PyObject *result = PyObject_CallObject(pFunc, pArg);
            if (!result)
            {
                if (PyErr_Occurred())
                {
                    PyErr_Print();
                }
            }
            Py_XDECREF(result);
            Py_DECREF(pArg);
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

void PythonAPI::checkReleaseScriptChange()
{
    std::queue<std::unordered_set<std::string>> messageQue;
    std::string runtimePath = "./Resource/CabbageEditorBackend";

    hotfixManger.TraverseDirectory(runtimePath, messageQue, hotfixManger.GetCurrentTimeMsec());

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