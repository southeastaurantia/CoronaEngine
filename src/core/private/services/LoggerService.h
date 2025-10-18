#pragma once

#include <corona/interfaces/Services.h>

#include <corona_logger.h>

namespace Corona::Core {

class LoggerService final : public Interfaces::ILogger {
  public:
    LoggerService() = default;
    ~LoggerService() override = default;

    void log(Level level, std::string_view message) override;
};

} // namespace Corona::Core
