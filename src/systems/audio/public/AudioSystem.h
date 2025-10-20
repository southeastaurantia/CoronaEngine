#pragma once

#include <corona/interfaces/Services.h>
#include <corona/interfaces/ThreadedSystem.h>

#include <cstdint>
#include <memory>

namespace Corona
{
    class AudioSystem final : public ThreadedSystem
    {
      public:
        AudioSystem();
        void configure(const Interfaces::SystemContext &context) override;

      protected:
        void onStart() override;
        void onTick() override;
        void onStop() override;

      private:
        void process_audio(std::uint64_t id);
        std::shared_ptr<Interfaces::IResourceService> resource_service_{};
        std::shared_ptr<Interfaces::ICommandScheduler> scheduler_{};
        Interfaces::ICommandScheduler::QueueHandle system_queue_handle_{};
    };
} // namespace Corona
