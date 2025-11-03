#include <corona/python/python_bridge.h>

#include <optional>

namespace Corona::PythonBridge {
namespace {
std::function<void(const std::string&)> g_sender;
std::mutex g_mtx;
}  // namespace

void set_sender(std::function<void(const std::string&)> sender) {
    std::lock_guard lk(g_mtx);
    g_sender = std::move(sender);
}

void clear_sender() {
    std::lock_guard lk(g_mtx);
    g_sender = nullptr;
}

void send(const std::string& message) {
    std::function<void(const std::string&)> fn;
    {
        std::lock_guard lk(g_mtx);
        fn = g_sender;
    }
    if (fn) {
        fn(message);
    }
}

}  // namespace Corona::PythonBridge
