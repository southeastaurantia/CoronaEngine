#include <Core/IO/ResMgr.h>
#include <Core/Logger.h>
#include <iostream>

class Texture final : public Corona::Resource
{
  public:
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
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        return true;
    }
};

int main()
{

    Corona::ResMgr<Texture>::register_loader<TextureLoader>();

    auto player_texture = Corona::ResMgr<Texture>::load("Player.png");
    auto player2_texture = Corona::ResMgr<Texture>::load("Player.png");
    auto player3_texture = Corona::ResMgr<Texture>::load("Player3.png");

    LOG_INFO("Player texture {}:{}", player_texture->width, player_texture->height);
    LOG_INFO("Player2 texture {}:{}", player2_texture->width, player2_texture->height);
    LOG_INFO("Player3 texture {}:{}", player3_texture->width, player3_texture->height);

    std::cin.get();

    return 0;
}