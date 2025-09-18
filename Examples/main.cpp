#include "Examples1.hpp"
#include "Examples2.hpp"
#include "Examples3.hpp"
#include "Examples4.hpp"

#include <Script/Python/PythonAPI.h>

int main()
{
    // Examples1();
    // Examples2();
    // Examples3();
    // Examples4();


    PythonAPI pythonManager;
    std::thread([&]() {
        while (true)
        {
            pythonManager.checkPythonScriptChange();
            pythonManager.checkReleaseScriptChange();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }).detach();

    while (true)
    {
        pythonManager.runPythonScript();
    }


    return 0;
}