//
// Created by 25473 on 25-9-11.
//

#ifndef SHADER_H
#define SHADER_H
#include "Core/IO/ResourceLoader.h"

namespace Corona {

    class Shader final : public Resource {
        public:

    };

    class ShaderLoader final : public ResourceLoader<Shader>
    {
        public:
            // bool load(const std::string &path, const Handle handle) override;
    };

} // Corona

#endif //SHADER_H
