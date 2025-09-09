#include "Core/SafeDataCache.h"

#include <Core/IO/ResMgr.h>
#include <Core/Logger.h>
#include <chrono>
#include <iostream>

class Texture final : public Corona::Resource
{
  public:
    int id;
    int width;
    int height;

    ~Texture()
    {
        // 此处调用会有问题，Logger对象优先于Texture销毁
        // LOG_DEBUG("Texture {} destroyed", get_rid());
    }
};

class TextureLoader final : public Corona::ResourceLoader<Texture>
{
  public:
    // 实现新的 load 接口
    bool load(const std::string &path, Handle &handle) override
    {
        // 直接修改传入的 handle 指向的对象
        handle->width = 10;
        handle->height = 20;
        return true;
    }
};

int main()
{
    // LOG_INFO是耗时操作

    Corona::ResMgr<Texture>::register_loader<TextureLoader>();

    auto player_texture = Corona::ResMgr<Texture>::load("Player.png");
    auto player2_texture = Corona::ResMgr<Texture>::load("Player.png");
    auto player3_texture = Corona::ResMgr<Texture>::load("Player3.png");

    LOG_INFO("Player texture {}:{}", player_texture->width, player_texture->height);
    LOG_INFO("Player2 texture {}:{}", player2_texture->width, player2_texture->height);
    LOG_INFO("Player3 texture {}:{}", player3_texture->width, player3_texture->height);

    Corona::SafeDataCache<Texture> texture_caches;
    std::unordered_set<uint64_t> keys2{}, keys3{}, keys4{};

    for (int i = 0; i < 100000; ++i)
    {
        int val = i + 1;
        auto texture = std::make_shared<Texture>();
        texture->id = i;
        texture->width = val;
        texture->height = val * 2;
        texture_caches.insert(i, texture);
        keys2.insert(i);
        keys3.insert(i);
        keys4.insert(i);
    }

    std::atomic<uint64_t> total_spend_time = 0;

    constexpr bool used_safe_loop_foreach = false;

    auto worker2 = std::thread([&] {
        const auto now = std::chrono::high_resolution_clock::now();
        if constexpr (!used_safe_loop_foreach)
        {
            for (auto const &id : keys2)
            {
                texture_caches.modify(id, [&](const std::shared_ptr<Texture> &texture) {
                    for (int i = 0; i < 10000; ++i)
                    {
                        texture->width = 555;
                        texture->height = 555;
                    }
                });
            }
        }
        else
        {
            texture_caches.safe_loop_foreach(keys2, [&](const std::shared_ptr<Texture> &texture) {
                for (int i = 0; i < 10000; ++i)
                {
                    texture->width = 555;
                    texture->height = 555;
                }
            });
        }

        const auto end = std::chrono::high_resolution_clock::now();
        const auto spend = std::chrono::duration_cast<std::chrono::microseconds>(end - now).count();
        total_spend_time.fetch_add(spend);
        LOG_INFO("Worker2 finish: {}ms, use safe loop foreach: {}", spend / 1000.0f, used_safe_loop_foreach ? "true" : "false");
    });

    auto worker3 = std::thread([&] {
        const auto now = std::chrono::high_resolution_clock::now();
        if constexpr (!used_safe_loop_foreach)
        {
            for (auto const &id : keys3)
            {
                texture_caches.modify(id, [&](const std::shared_ptr<Texture> &texture) {
                    for (int i = 0; i < 9000; ++i)
                    {
                        texture->width = 666;
                        texture->height = 666;
                    }
                });
            }
        }
        else
        {
            texture_caches.safe_loop_foreach(keys3, [&](const std::shared_ptr<Texture> &texture) {
                for (int i = 0; i < 9000; ++i)
                {
                    texture->width = 666;
                    texture->height = 666;
                }
            });
        }
        const auto end = std::chrono::high_resolution_clock::now();
        const auto spend = std::chrono::duration_cast<std::chrono::microseconds>(end - now).count();
        total_spend_time.fetch_add(spend);
        LOG_INFO("Worker3 finish: {}ms, use safe loop foreach: {}", spend / 1000.0f, used_safe_loop_foreach ? "true" : "false");
    });

    auto worker4 = std::thread([&] {
        const auto now = std::chrono::high_resolution_clock::now();
        if constexpr (!used_safe_loop_foreach)
        {
            for (auto const &id : keys4)
            {
                texture_caches.modify(id, [&](const std::shared_ptr<Texture> &texture) {
                    for (int i = 0; i < 8000; ++i)
                    {
                        texture->width = 777;
                        texture->height = 777;
                    }
                });
            }
        }
        else
        {
            texture_caches.safe_loop_foreach(keys3, [&](const std::shared_ptr<Texture> &texture) {
                for (int i = 0; i < 8000; ++i)
                {
                    texture->width = 777;
                    texture->height = 777;
                }
            });
        }
        const auto end = std::chrono::high_resolution_clock::now();
        const auto spend = std::chrono::duration_cast<std::chrono::microseconds>(end - now).count();
        total_spend_time.fetch_add(spend);
        LOG_INFO("Worker4 finish: {}ms, use safe loop foreach: {}", spend / 1000.0f, used_safe_loop_foreach ? "true" : "false");
    });

    auto texture = texture_caches.get(5);
    // texture->width = 5 // not allowed, used texture_caches.modify
    LOG_INFO("=======Texture get id 5, {}:{}", texture->width, texture->height);

    worker2.join();
    worker3.join();
    worker4.join();

    LOG_INFO("=======Texture get id 5, {}:{}", texture->width, texture->height);

    std::cin.get();

    LOG_INFO("Size = {}, Total spend: {}us", texture_caches.size(), total_spend_time.load());

    return 0;
}