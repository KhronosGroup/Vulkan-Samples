#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inColor;
layout(location = 0) out vec4 rt0;

void main()
{
    rt0 = inColor;
}