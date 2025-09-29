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
    static void checkPythonScriptChange();
    void checkReleaseScriptChange();

  private:
    static const std::string codePath;

    PythonHotfix hotfixManger;
    std::shared_mutex queMtx;

    int64_t lastHotReloadTime = 0; // ms
    bool hasHotReload = false;

    PyObject *pModule = nullptr;
    PyObject *pFunc = nullptr;
    PyObject *messageFunc = nullptr;

    std::string hotreloadPath; // 无需冗余初始化
    std::vector<std::string> moduleList;
    std::vector<std::string> callableList;

    PyConfig config{}; // 值初始化

    bool ensureInitialized();
    bool performHotReload();
    void invokeEntry(bool isReload) const;
    void sendMessage(const std::string &message) const;
    static int64_t nowMsec();
    static std::wstring str2wstr(const std::string &str);
    static void copyModifiedFiles(const std::filesystem::path& sourceDir,
                           const std::filesystem::path& destDir,
                           int64_t checkTimeMs);
};