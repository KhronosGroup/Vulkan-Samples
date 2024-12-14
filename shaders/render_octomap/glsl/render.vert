#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 aPos;

// Instanced attributes
layout (location = 1) in vec3 instancePos;
layout (location = 2) in vec4 inColor;
layout (location = 3) in float instanceScale;

layout (binding = 0) uniform UBO
{
    mat4 projection;
    mat4 camera;
} ubo;

layout (location = 0) out vec4 outColor;

void main()
{
    outColor = inColor;
    vec4 locPos = vec4(aPos, 1.0);
    float eps = 0.00001;
    vec4 Pos = vec4((locPos.xyz * instanceScale) + instancePos, 1.0);
    Pos.x -= eps;
    Pos.y -= eps;
    Pos.z -= eps;
    gl_Position = ubo.projection * ubo.camera * Pos;
}