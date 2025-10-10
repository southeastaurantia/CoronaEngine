#pragma once

namespace Corona::Concurrent {

void initialize() noexcept;
void finalize() noexcept;

namespace detail {

class RuntimeAutoInit {
   public:
    RuntimeAutoInit() noexcept { initialize(); }
    ~RuntimeAutoInit() noexcept { finalize(); }
};

inline void ensure_runtime_initialized() noexcept {
    static RuntimeAutoInit guard{};
    (void)guard;
}

}  // namespace detail

}  // namespace Corona::Concurrent
