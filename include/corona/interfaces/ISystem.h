#pragma once

#include <corona/interfaces/SystemContext.h>

namespace Corona {

class ISystem {
   protected:
    explicit ISystem() = default;
    virtual ~ISystem() = default;

   public:
    virtual const char* name() = 0;  // name用于日志输出

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void configure(const Interfaces::SystemContext& context) {
        (void)context;
    }

   protected:
    virtual void tick() = 0;
};

}  // namespace Corona
