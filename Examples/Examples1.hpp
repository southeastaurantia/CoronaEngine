#pragma once

#include <Core/Engine/Engine.h>
#include <Core/IO/Loaders/TextResource.h>
#include <Core/Log.h>

#include <filesystem>
#include <fstream>
#include <unordered_map>

// 示例：自定义资源与加载器
namespace demo
{
    struct MyConfigResource : public Corona::IResource
    {
        std::unordered_map<std::string, std::string> kv;
    };

    class MyConfigLoader : public Corona::IResourceLoader
    {
      public:
        bool supports(const Corona::ResourceId &id) const override
        {
            auto endsWith = [](const std::string &s, const std::string &suf) {
                return s.size() >= suf.size() && std::equal(suf.rbegin(), suf.rend(), s.rbegin());
            };
            if (id.type == "config")
                return true; // 按类型匹配
            if (endsWith(id.path, ".cfg"))
                return true; // 或按扩展名匹配
            return false;
        }

        std::shared_ptr<Corona::IResource> load(const Corona::ResourceId &id) override
        {
            std::ifstream ifs(id.path);
            if (!ifs.is_open())
            {
                CE_LOG_ERROR("MyConfig open failed: {}", id.path);
                return nullptr;
            }

            auto res = std::make_shared<MyConfigResource>();
            std::string line;
            while (std::getline(ifs, line))
            {
                if (line.empty() || line[0] == '#')
                    continue;
                auto pos = line.find('=');
                if (pos == std::string::npos)
                    continue;
                std::string key = line.substr(0, pos);
                std::string val = line.substr(pos + 1);
                res->kv.emplace(std::move(key), std::move(val));
            }
            return res;
        }
    };
} // namespace demo

inline void Examples1()
{
    // Init logger
    Corona::LogConfig cfg;
    cfg.enableConsole = true;
    cfg.enableFile = false;
    cfg.level = Corona::LogLevel::Debug;
    Corona::Engine::Instance().Init(cfg);

    CE_LOG_INFO("CoronaEngine ResourceManager demo start");

    namespace fs = std::filesystem;
    CE_LOG_INFO("Working dir: {}", fs::current_path().string());

    // Prefer an existing shader file in Examples/assets; fallback to a temp file in CWD
    fs::path shaderVert = fs::current_path() / "assets" / "shaders" / "test.vert.glsl";
    fs::path shaderFrag = fs::current_path() / "assets" / "shaders" / "test.frag.glsl";
    fs::path demoTxt = fs::current_path() / "demo_resource.txt";

    fs::path target = shaderVert;
    if (!fs::exists(target))
    {
        // Create a small demo text file if assets path isn't valid from current working dir
        std::ofstream ofs(demoTxt);
        ofs << "Hello from CoronaEngine ResourceManager!\n";
        ofs << "This is a demo text loaded via TextResourceLoader.";
        ofs.close();
        target = demoTxt;
        CE_LOG_WARN("Asset not found; created demo file: {}", target.string());
    }

    // 1) Sync load
    auto &io = Corona::Engine::Instance().Resources();
    Corona::ResourceId idText{"text", target.string()};
    auto txtRes = io.loadTyped<Corona::TextResource>(idText);
    if (txtRes)
    {
        CE_LOG_INFO("Sync loaded: {} ({} bytes)", idText.path, txtRes->text.size());
    }
    else
    {
        CE_LOG_ERROR("Sync load failed: {}", idText.path);
    }

    // 2) Async load (future)
    auto fut = io.loadAsync(idText);
    auto res2 = fut.get();
    CE_LOG_INFO("Async(future) loaded from cache: {} -> {}", idText.path, res2 ? "ok" : "fail");

    // 3) Async load (callback)
    io.loadAsync(idText, [](const Corona::ResourceId &rid, std::shared_ptr<Corona::IResource> r) {
        CE_LOG_INFO("Async(callback) loaded: {} -> {}", rid.path, r ? "ok" : "fail");
    });

    // 4) Preload a batch (if frag exists, otherwise preload the same file twice)
    Corona::ResourceId idFrag{"text", fs::exists(shaderFrag) ? shaderFrag.string() : target.string()};
    io.preload({idText, idFrag});
    io.wait(); // wait all async tasks

    // Optional: read back frag
    auto frag = io.loadTyped<Corona::TextResource>(idFrag);
    CE_LOG_INFO("Preloaded-read: {} -> {}", idFrag.path, frag ? "ok" : "fail");

    // 5) 自定义资源与加载器示例
    // 注册一次自定义加载器（线程安全，重复注册会追加在末尾）
    io.registerLoader(std::make_shared<demo::MyConfigLoader>());

    // 准备一个简单的 .cfg 文件
    auto cfgPath = fs::current_path() / "demo_config.cfg";
    {
        std::ofstream cfg(cfgPath, std::ios::out);
        cfg << "title=CoronaEngine Demo\n";
        cfg << "version=1.0\n";
        cfg << "mode=development\n";
    }

    Corona::ResourceId idCfg{"config", cfgPath.string()};
    auto cfgRes = io.loadTyped<demo::MyConfigResource>(idCfg);
    if (cfgRes)
    {
        CE_LOG_INFO("Custom cfg loaded: {} ({} entries)", idCfg.path, cfgRes->kv.size());
        if (auto it = cfgRes->kv.find("title"); it != cfgRes->kv.end())
            CE_LOG_INFO("cfg.title = {}", it->second);
    }
    else
    {
        CE_LOG_ERROR("Custom cfg load failed: {}", idCfg.path);
    }

    // Clean shutdown
    Corona::Engine::Instance().Shutdown();
}