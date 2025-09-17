#pragma once

#include <chrono>
#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <mutex>
#include <queue>
#include <regex>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <Python.h>

struct PythonHotfix
{

    int pyFileCount = 0;

    void formatStr(std::string &inputStr)
    {
        std::replace(inputStr.begin(), inputStr.end(), '\\', '.');
        std::replace(inputStr.begin(), inputStr.end(), '/', '.');
    
        std::regex pattern("([^\\.]*?)\\.py$");
        std::smatch match;
    
        if (std::regex_search(inputStr, match, pattern) && match.size() > 1)
        {
            inputStr = match[1].str();
        }
        else
        {
            // 如果不是.py文件，尝试提取最后一部分
            size_t lastDot = inputStr.find_last_of('.');
            if (lastDot != std::string::npos)
            {
                inputStr = inputStr.substr(lastDot + 1);
            }
            else
            {
                inputStr = "";
            }
        }
    }

    bool endsWith(const std::string &str, const std::string &suffix)
    {
        if (str.length() < suffix.length())
            return false;
        return str.substr(str.length() - suffix.length()) == suffix;
    }

    void TraverseDirectory(const std::filesystem::path &directory,
                           std::queue<std::unordered_set<std::string>> &MessageQue, long long checkTime)
    {
        std::unordered_set<std::string> packageSet;
        // auto startTime = std::chrono::high_resolution_clock::now();
        // pyFileCount = 0;

        std::ofstream fileStream(directory);

        if (!std::filesystem::exists(directory.string() + "\\__init__.py"))
            std::ofstream file(directory.string() + "\\__init__.py");

        for (const auto &entry : std::filesystem::recursive_directory_iterator(directory))
        {
            const std::filesystem::path &filePath = entry.path();
            std::string fileName = filePath.filename().string();

            if (entry.is_directory())
            {
                std::string pathName = filePath.string();
                if (!std::filesystem::exists(pathName+"\\__init__.py"))
                    std::ofstream file(pathName + "\\__init__.py");
                continue;
            }

            if (!endsWith(fileName, ".py"))
                continue;

            bool shouldSkip = false;

            const std::set<std::string> skipSuffixes = {"__pycache__", "__init__.py", ".pyc", "StaticComponents.py"};
            for (const auto &suffix : skipSuffixes)
            {
                if (endsWith(fileName, suffix))
                {
                    shouldSkip = true;
                    break;
                }
            }
            if (shouldSkip)
            {
                continue;
            }
            else
            {
                // pyFileCount++;
                std::string filePathStr = filePath.string();
                long long modifyTime = std::chrono::system_clock::to_time_t(std::chrono::clock_cast<std::chrono::system_clock>(std::filesystem::last_write_time(filePathStr)));
                formatStr(filePathStr);
                if (filePathStr != "" && checkTime - modifyTime <= 1)
                {
                    packageSet.insert(filePathStr);
                }
            }
        }
        fileStream.close();

        if (packageSet.size() != 0)
        {
            MessageQue.push(packageSet);
        }
        // auto endTime = std::chrono::high_resolution_clock::now();
        // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        // std::cout << "Python Release TraverseDirectory took: " << duration << " ms." << std::endl;
        // std::cout << "Python Release file count: " << pyFileCount << std::endl;
    }

    time_t GetCurrentTimeMsec()
    {
        return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    }

    void CheckPythonFileDependence()
    {
        // auto DependenceStartTime = std::chrono::high_resolution_clock::now();

        if (packageSet.empty())
        {
            return;
        }
        // 获取所有已加载模块的字典
        PyObject* sysModules = PyImport_GetModuleDict();
        if (!sysModules)
        {
            PyErr_Clear();
            return;
        }
    
        // 构建完整的依赖关系图(包含所有模块的依赖关系)
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(sysModules, &pos, &key, &value))
        {
            if (PyModule_Check(value))
            {
                const char* moduleName = PyModule_GetName(value);
                if (!moduleName) continue;
                
                PyObject* moduleDict = PyModule_GetDict(value);
                if (!moduleDict) continue;
    
                PyObject *mkey, *mvalue;
                Py_ssize_t mpos = 0;
                while (PyDict_Next(moduleDict, &mpos, &mkey, &mvalue))
                {
                    if (PyModule_Check(mvalue))
                    {
                        const char* importedModuleName = PyModule_GetName(mvalue);
                        if (importedModuleName)
                        {
                            // 记录所有模块的依赖关系
                            dependencyGraph[importedModuleName].insert(moduleName);
                        }
                    }
                }
            }
        }
    
        // 递归查找所有相关依赖
        std::unordered_set<std::string> visited;
        std::queue<std::string> toVisit;
        dependencyVec.clear();
        
        // 初始队列包含所有修改过的模块
        for (const auto& module : packageSet)
        {
            toVisit.push(module);
            visited.insert(module);
            dependencyVec.push_back(module);
        }
    
        // 广度优先搜索所有依赖
        while (!toVisit.empty())
        {
            std::string current = toVisit.front();
            toVisit.pop();
    
            // 查找所有依赖当前模块的模块
            if (dependencyGraph.find(current) != dependencyGraph.end())
            {
                for (const auto& depender : dependencyGraph[current])
                {
                    if (visited.find(depender) == visited.end())
                    {
                        visited.insert(depender);
                        toVisit.push(depender);
                        dependencyVec.push_back(depender);
                    }
                }
            }
        }
    
        PyErr_Clear();


        // auto DependenceEndTime = std::chrono::high_resolution_clock::now();
        // auto DependenceDuration = std::chrono::duration_cast<std::chrono::milliseconds>(DependenceEndTime - DependenceStartTime).count();
        // std::cout << "CheckPythonFileDependence took: " << DependenceDuration << " ms." << std::endl;
        // std::cout << "CheckPythonFileDependence file count: " << dependencyVec.size() << std::endl;
    }

    bool ReloadPythonFile()
    {

        CheckPythonFileDependence();

        // auto startTime = std::chrono::high_resolution_clock::now();
        bool reloadSomething = !packageSet.empty();
    
        for (int i = static_cast<int>(dependencyVec.size()) - 1; i >= 0; --i)
        {
            std::string moduleName = dependencyVec[i];
    
            PyObject *moduleNameObj = PyUnicode_FromString(moduleName.c_str());
            PyObject *pModule = PyImport_GetModule(moduleNameObj);
            Py_DECREF(moduleNameObj); 

            if (pModule)
            {
                PyObject *newModule = PyImport_ReloadModule(pModule);
                if (!newModule)
                {
                    if (PyErr_Occurred())
                    {
                        PyErr_Print();
                    }
                }
                Py_DECREF(pModule);
            }
        }
    
        packageSet.clear();
        dependencyGraph.clear();

        // auto endTime = std::chrono::high_resolution_clock::now();
        // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        // std::cout << "ReloadPythonFile took: " << duration << " ms." << std::endl;
        return reloadSomething;
    }
    

    std::unordered_set<std::string> packageSet;
    std::unordered_map<std::string, std::unordered_set<std::string>> dependencyGraph;
    std::vector<std::string> dependencyVec;
};