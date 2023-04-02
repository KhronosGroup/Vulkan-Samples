#pragma once

#include <string>

#include "shader_descriptor.hpp"

namespace shader_compiler
{
void generate_shader_header(const ShaderDescriptor &descriptor, const std::string &output_file);
}        // namespace shader_compiler