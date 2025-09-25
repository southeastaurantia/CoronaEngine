#include "TextResource.h"

#include <Log.h>

#include <algorithm>
#include <fstream>

namespace Corona
{
    static bool endsWith(const std::string &s, const std::string &suf)
    {
        if (s.size() < suf.size())
            return false;
        return std::equal(suf.rbegin(), suf.rend(), s.rbegin());
    }

    bool TextResourceLoader::supports(const ResourceId &id) const
    {
        for (const auto &e : exts_)
        {
            if (endsWith(id.path, e))
                return true;
        }
        return false;
    }

    std::shared_ptr<IResource> TextResourceLoader::load(const ResourceId &id)
    {
        std::ifstream ifs(id.path, std::ios::in | std::ios::binary);
        if (!ifs.is_open())
        {
            CE_LOG_ERROR("TextResource open failed: {}", id.path);
            return nullptr;
        }
        std::string content;
        ifs.seekg(0, std::ios::end);
        content.resize(static_cast<size_t>(ifs.tellg()));
        ifs.seekg(0, std::ios::beg);
        ifs.read(content.data(), static_cast<std::streamsize>(content.size()));
        auto res = std::make_shared<TextResource>();
        res->text = std::move(content);
        return res;
    }
} // namespace Corona
