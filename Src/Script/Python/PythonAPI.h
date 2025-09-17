#pragma once

#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <thread>

#include "PythonHotfix.h"

#include "EngineScripts.h"

#include <Python.h>

struct PythonAPI
{
    PythonAPI();

    ~PythonAPI();

    void runPythonScript();

    void checkPythonScriptChange();
    void checkReleaseScriptChange();

    std::wstring str2wstr(const std::string &str);

  private:
    
    static const std::string codePath;
    
    PythonHotfix hotfixManger;

    std::shared_mutex queMtx;
    long long lastHotReloadTime = 0;
    bool hasHotReload = false;

    PyObject *pModule = nullptr;
    PyObject *pFunc = nullptr;

    std::string hotreloadPath = "";

    std::vector<std::string> moduleList;
    std::vector<std::string> callableList;

    PyConfig config;

    void copyModifiedFiles(const std::filesystem::path &sourceDir, const std::filesystem::path &destDir, long long checkTime);
};