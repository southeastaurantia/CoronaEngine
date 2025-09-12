//
// Created by 25473 on 25-9-11.
//

#pragma once
#include "Core/IO/ResourceLoader.h"
#include "Pipeline/RasterizerPipeline.h"
#include "Pipeline/ComputePipeline.h"

namespace Corona {

    class Shader final : public Resource {
        public:
            std::string vertCode;
            std::string fragCode;
            std::string computeCode;
    };

    class ShaderLoader final : public ResourceLoader<Shader>
    {
        public:
            bool load(const std::string &path, const Handle &handle) override;

        private:
            std::string readStringFile(const std::string_view &path);
    };

} // Corona
