#pragma once

#include <Python.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/script/python/python_hotfix.h>
#include <nanobind/nanobind.h>

#include <chrono>
#include <filesystem>
#include <shared_mutex>
#include <string>

struct PythonAPI {
    PythonAPI();

    ~PythonAPI();

    void runPythonScript();
    static void checkPythonScriptChange();
    void checkReleaseScriptChange();
    void sendMessage(const std::string& message) const;

   private:
    static const std::string codePath;

    PythonHotfix hotfixManger;
    mutable std::shared_mutex queMtx;

    int64_t lastHotReloadTime = 0;  // ms
    bool hasHotReload = false;

    nanobind::object pModule;      // module 'main'
    nanobind::object pFunc;        // callable 'run'
    nanobind::object messageFunc;  // callable 'put_queue'

    std::vector<std::string> moduleList;
    std::vector<std::string> callableList;

    PyConfig config{};  // 值初始化

    std::unique_ptr<Corona::Kernel::ILogger> logger;

    bool ensureInitialized();
    bool performHotReload();
    void invokeEntry(bool isReload) const;
    static int64_t nowMsec();
    static std::wstring str2wstr(const std::string& str);
    static std::string wstr2str(const std::wstring& wstr);
    static void copyModifiedFiles(const std::filesystem::path& sourceDir,
                                  const std::filesystem::path& destDir,
                                  int64_t checkTimeMs);
};