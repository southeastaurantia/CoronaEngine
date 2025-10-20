#pragma once

#include <corona/script/EngineScripts.h>
#include <corona/script/PythonHotfix.h>

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
    // Leak-safe hot-reload toggle (runtime). When enabled, we skip DECREF on reused
    // objects to avoid crashes at the cost of small leaks during development.
    void setLeakSafeReload(bool enabled);
    [[nodiscard]] bool isLeakSafeReload() const;

   private:
    static const std::string codePath;

    PythonHotfix hotfixManger;
    mutable std::shared_mutex queMtx;

    int64_t lastHotReloadTime = 0;  // ms
    bool hasHotReload = false;

    // Controls DECREF behavior on hot reload; default comes from env CORONA_PY_LEAKSAFE (1/0),
    // falling back to true if unset.
    bool leakSafeMainReload_ = true;

    PyObject* pModule = nullptr;
    PyObject* pFunc = nullptr;
    PyObject* messageFunc = nullptr;

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