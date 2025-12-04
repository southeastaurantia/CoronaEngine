#version 460
#extension GL_EXT_nonuniform_qualifier : enable

layout(push_constant) uniform PushConsts
{    
    uint textureIndex;
    //uint boneIndex;
    uint uniformBufferIndex;
    mat4 modelMatrix;
} pushConsts;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 viewProjMatrix;
} uniformBufferObjects[];

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec2 fragMotionVector;


void main() 
{
    vec4 worldPos = pushConsts.modelMatrix * vec4(inPosition, 1.0f);
    fragPos = worldPos.xyz; 
    gl_Position = uniformBufferObjects[pushConsts.uniformBufferIndex].viewProjMatrix * worldPos;

    fragTexCoord = inTexCoord;
    fragNormal = inNormal;

    fragMotionVector = vec2(0.0f);
}