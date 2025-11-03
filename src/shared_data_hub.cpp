#include <corona/shared_data_hub.h>

namespace Corona {

SharedDataHub& SharedDataHub::instance() {
    static SharedDataHub instance;
    return instance;
}

SharedDataHub::DemoDataStorage& SharedDataHub::demo_data_storage() {
    return demo_data_storage_;
}

const SharedDataHub::DemoDataStorage& SharedDataHub::demo_data_storage() const {
    return demo_data_storage_;
}

}  // namespace Corona