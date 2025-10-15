#pragma once

#include <functional>
#include <mutex>
#include <string>

namespace Corona::PythonBridge {

// Register a sender that will run on the main thread and deliver messages to Python.
void set_sender(std::function<void(const std::string&)> sender);

// Clear the sender (e.g., during shutdown).
void clear_sender();

// Invoke the sender if present; safe to call from any thread, but preferred on main thread.
void send(const std::string& message);

} // namespace Corona::PythonBridge

