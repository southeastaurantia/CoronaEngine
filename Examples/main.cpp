#include "Examples1.hpp"
#include "Examples2.hpp"
#include "Examples3.hpp"
#include "Examples4.hpp"

#define PY_SSIZE_T_CLEAN
#include <Python.h>



std::wstring str2wstr(const std::string& str) {
    if (str.empty()) return {};
    int wlen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
    if (wlen <= 0 ) return {};
    std::wstring w(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), w.data(), wlen);
    return w;
}


int main()
{
    // Examples1();
    // Examples2();
    // Examples3();
    // Examples4();
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    PyConfig_SetBytesString(&config, &config.home, CORONA_PYTHON_HOME_DIR);
    PyConfig_SetBytesString(&config, &config.pythonpath_env, CORONA_PYTHON_HOME_DIR);
    config.module_search_paths_set = 1;

    PyWideStringList_Append(&config.module_search_paths, str2wstr(CORONA_PYTHON_MODULE_DLL_DIR).c_str());
    PyWideStringList_Append(&config.module_search_paths, str2wstr(CORONA_PYTHON_MODULE_LIB_DIR).c_str());
    PyWideStringList_Append(&config.module_search_paths, str2wstr(std::string(CORONA_PYTHON_MODULE_LIB_DIR) + "/site-packages").c_str());

    Py_InitializeFromConfig(&config);

    std::cerr << "Py_IsInitialized=" << Py_IsInitialized() << std::endl;
    std::cerr << "tstate=" << PyThreadState_Get() << std::endl;

    // 创建一个大小为 3 的元组
    PyObject *tuple = PyTuple_New(1);
    Py_DECREF(tuple);

    // 关闭 Python 解释器
    Py_Finalize();

    return 0;

    return 0;
}