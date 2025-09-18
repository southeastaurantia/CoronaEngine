#include <Python.h>

inline void Examples3()
{
    PyStatus status;
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    /* optional but recommended */
    status = PyConfig_SetBytesString(&config, &config.program_name, CORONA_PYTHON_EXE);

    CORONA_PYTHON_EXE;
    CORONA_PYTHON_MODULE_DLL_DIR;
    CORONA_PYTHON_MODULE_LIB_DIR;

    if (PyStatus_Exception(status))
    {
        PyConfig_Clear(&config);
        Py_ExitStatusException(status);
    }

    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status))
    {
        PyConfig_Clear(&config);
        Py_ExitStatusException(status);
    }
    PyConfig_Clear(&config);

    PyRun_SimpleString(R"(
import main
main.main()
)");
    if (Py_FinalizeEx() < 0)
    {
        exit(120);
    }
}