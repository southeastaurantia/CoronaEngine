#pragma once

#include <corona/interfaces/Concurrency.h>
#include <corona/interfaces/ServiceLocator.h>

namespace Corona {
class EventBusHub;
class DataCacheHub;
} // namespace Corona

namespace Corona::Interfaces {

struct SystemContext {
    ServiceLocator& services;
    ICommandQueue* queue = nullptr;
    Corona::EventBusHub* events = nullptr;
    Corona::DataCacheHub* caches = nullptr;
};

} // namespace Corona::Interfaces
