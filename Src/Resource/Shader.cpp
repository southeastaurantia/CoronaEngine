//
// Created by 25473 on 25-9-11.
//

#include "Shader.h"

#include <fstream>

namespace Corona {

    bool ShaderLoader::load(const std::string &path, const Handle &handle)
    {
        if (!handle)
        {
            return false;
        }

        handle->vertCode = readStringFile(path+"/shaders/test.vert.glsl");
        handle->fragCode = readStringFile(path+"/shaders/test.frag.glsl");
        handle->computeCode = readStringFile(path+"/shaders/test.comp.glsl");

        return true;
    }

    std::string ShaderLoader::readStringFile(const std::string_view &path)
    {
        std::ifstream file(path.data());
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open the file.");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        file.close();
        return buffer.str();
    }
} // Corona