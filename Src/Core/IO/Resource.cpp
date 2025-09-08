//
// Created by 47226 on 2025/9/8.
//

#include "Resource.h"

std::atomic<Corona::Resource::RID> Corona::Resource::id_counter = 0;

namespace Corona
{
    Resource::RID Resource::get_rid() const
    {
        return rid;
    }
    Resource::Status Resource::get_status() const
    {
        return status.load();
    }
    void Resource::set_status(const Status value)
    {
        if (this->status.load() != value)
        {
            this->status.store(value);
        }
    }
    Resource::RID Resource::get_next_id()
    {
        return id_counter.fetch_add(1);
    }
} // namespace Corona