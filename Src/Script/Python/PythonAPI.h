#pragma once

#include <chrono>
#include <shared_mutex>
#include <filesystem>
#include <string>


#include "PythonHotfix.h"
#include "EngineScripts.h"

struct PythonAPI
{
    PythonAPI();

    ~PythonAPI();

    void runPythonScript();
    void checkPythonScriptChange();
    void checkReleaseScriptChange();

  private:
    
    static const std::string codePath;
    
    PythonHotfix hotfixManger;
    std::shared_mutex queMtx;

    long long lastHotReloadTime = 0;
    bool hasHotReload = false;

    PyObject *pModule = nullptr;
    PyObject *pFunc = nullptr;
    PyObject *messageFunc = nullptr;

    std::string hotreloadPath = "";
    std::vector<std::string> moduleList;
    std::vector<std::string> callableList;

    PyConfig config;

    bool ensureInitialized();
    bool performHotReload();
    void invokeEntry(bool isReload);
    void sendMessage(const std::string &message);
    static long long nowMsec();
    std::wstring str2wstr(const std::string &str);
    void copyModifiedFiles(const std::filesystem::path& sourceDir,
                           const std::filesystem::path& destDir,
                           long long checkTime);
};