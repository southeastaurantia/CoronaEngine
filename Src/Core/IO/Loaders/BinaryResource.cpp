#include "BinaryResource.h"
#include "Core/Log.h"

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

    bool BinaryResourceLoader::supports(const ResourceId &id) const
    {
        for (const auto &e : exts_)
        {
            if (endsWith(id.path, e))
                return true;
        }
        return false;
    }

    std::shared_ptr<IResource> BinaryResourceLoader::load(const ResourceId &id)
    {
        std::ifstream ifs(id.path, std::ios::binary);
        if (!ifs.is_open())
        {
            CE_LOG_ERROR("BinaryResource open failed: {}", id.path);
            return nullptr;
        }
        ifs.seekg(0, std::ios::end);
        auto size = static_cast<size_t>(ifs.tellg());
        ifs.seekg(0, std::ios::beg);
        std::vector<unsigned char> buf(size);
        ifs.read(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
        auto res = std::make_shared<BinaryResource>();
        res->data = std::move(buf);
        return res;
    }
} // namespace Corona
