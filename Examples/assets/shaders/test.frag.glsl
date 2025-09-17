#version 460
#extension GL_EXT_nonuniform_qualifier : enable

layout(push_constant) uniform PushConsts
{    
    uint textureIndex;
    uint boneIndex;
    uint uniformBufferIndex;
    mat4 modelMatrix;
} pushConsts;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 viewProjMatrix;
} uniformBufferObjects[];


layout (set = 1, binding = 0) uniform sampler2D textures[];

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec2 fragMotionVector;

layout(location = 0) out vec3 gbufferPostion;
layout(location = 1) out vec3 gbufferNormal;
layout(location = 2) out vec4 gbufferBaseColor;
layout(location = 3) out vec2 gbufferMotionVector;

void main()
{
    gbufferBaseColor = vec4(vec3(texture(textures[pushConsts.textureIndex], fragTexCoord)),1.0f);
    gbufferNormal = fragNormal;
    gbufferPostion = fragPos;
    gbufferMotionVector = fragMotionVector;
}