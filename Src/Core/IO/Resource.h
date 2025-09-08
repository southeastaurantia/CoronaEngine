//
// Created by 47226 on 2025/9/8.
//

#ifndef CORONAENGINE_RESOURCE_H
#define CORONAENGINE_RESOURCE_H
#include <memory>

namespace Corona
{
    class Resource
    {
      public:
        enum class Status
        {
            OK,
            FAILED,
            LOADING,
            NOT_FOUND
        };

        using RID = uint64_t;

        ~Resource() = default;

        RID get_rid() const;
        Status get_status() const;
        void set_status(Status value);

      private:
        const RID rid{get_next_id()};
        std::atomic<Status> status{Status::NOT_FOUND};

        static RID get_next_id();
        static std::atomic<RID> id_counter;
    };

} // namespace Corona

#endif // CORONAENGINE_RESOURCE_H
