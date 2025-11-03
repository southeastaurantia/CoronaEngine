#pragma once
#include <corona/kernel/utils/storage.h>

namespace Corona {

class SharedDataHub {
    struct DemoData {
        int value;
    };

   public:
    static SharedDataHub& instance() {
        static SharedDataHub instance;
        return instance;
    }

    ~SharedDataHub() = default;

   private:
    SharedDataHub() = default;
    SharedDataHub(const SharedDataHub&) = delete;
    SharedDataHub& operator=(const SharedDataHub&) = delete;
    SharedDataHub(SharedDataHub&&) = delete;
    SharedDataHub& operator=(SharedDataHub&&) = delete;

   public:
    using DemoDataStorage = Kernel::Utils::Storage<DemoData, 128>;
    DemoDataStorage& demo_data_storage() { return demo_data_storage_; }
    const DemoDataStorage& demo_data_storage() const { return demo_data_storage_; }

   private:
    DemoDataStorage demo_data_storage_;
};

}  // namespace Corona