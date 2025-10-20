#pragma once

#include <corona/script/EngineScripts.h>
#include <corona/script/PythonHotfix.h>

#include <chrono>
#include <filesystem>
#include <shared_mutex>
#include <string>

#include <nanobind/nanobind.h>
#include "PythonHotfix.h"
#include "EngineScripts.h"

struct PythonAPI
{
    PythonAPI();

    ~PythonAPI();

    void runPythonScript();
    static void checkPythonScriptChange();
    void checkReleaseScriptChange();
    void sendMessage(const std::string &message) const;

   private:
    static const std::string codePath;

    PythonHotfix hotfixManger;
    mutable std::shared_mutex queMtx;

    int64_t lastHotReloadTime = 0;  // ms
    bool hasHotReload = false;

    nanobind::object pModule;       // module 'main'
    nanobind::object pFunc;         // callable 'run'
    nanobind::object messageFunc;   // callable 'put_queue'

    std::vector<std::string> moduleList;
    std::vector<std::string> callableList;

    PyConfig config{};  // 值初始化

    bool ensureInitialized();
    bool performHotReload();
    void invokeEntry(bool isReload) const;
    static int64_t nowMsec();
    static std::wstring str2wstr(const std::string& str);
    static void copyModifiedFiles(const std::filesystem::path& sourceDir,
                                  const std::filesystem::path& destDir,
                                  int64_t checkTimeMs);
};