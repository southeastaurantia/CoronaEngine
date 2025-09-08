// //
// // Created by 47226 on 2025/9/6.
// //
//
// #ifndef CORONAENGINE_RESOURCE_HPP
// #define CORONAENGINE_RESOURCE_HPP
// #include <atomic>
// #include <memory>
// #include <string>
//
// namespace Corona
// {
//
//     class Resource
//     {
//       public:
//         template <typename ResourceType>
//             requires std::is_base_of_v<Resource, ResourceType> && std::default_initializable<ResourceType>
//         friend class ResourceManager;
//
//         enum class Status
//         {
//             NOT_FOUND, // 未加载
//             LOADING,   // 加载中
//             OK,        // 已加载
//             FAILED     // 加载失败
//         };
//
//         virtual ~Resource() = default;
//
//         const std::string &get_id() const;
//         Status get_status() const;
//
//       protected:
//         std::string res_id; // 资源ID, 路径即ID
//         std::atomic<Status> res_status{Status::NOT_FOUND};
//     };
//
// } // namespace CoronaEngine
//
// #endif // CORONAENGINE_RESOURCE_HPP
