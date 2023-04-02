#include "shader_compiler.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <components/glslang/shader_compiler.hpp>
#include <components/spirv_reflect/shader_reflector.hpp>
#include <re2/re2.h>

#include "shader_descriptor.hpp"

#include <components/common/logging.hpp>
#include <components/common/strings.hpp>

namespace shader_compiler
{
using namespace components;
using namespace components::glslang;
using namespace components::spirv_reflect;

class IncludeResolver
{
  public:
	IncludeResolver(const std::vector<std::string> &search_paths) :
	    search_paths(search_paths)
	{}

	~IncludeResolver() = default;

	std::string resolve_include(const std::string &include_name) const
	{
		for (const auto &search_path : search_paths)
		{
			std::string full_path = search_path + "/" + include_name;
			if (std::filesystem::exists(full_path))
			{
				return full_path;
			}
		}

		throw std::runtime_error("Failed to resolve include: " + include_name);
	}

  private:
	std::vector<std::string> search_paths;
};

std::vector<std::vector<std::string>> generate_combinations(const std::vector<std::string> &input_set)
{
	std::vector<std::vector<std::string>> output_set;
	const std::size_t                     input_size       = input_set.size();
	const std::size_t                     max_combinations = 1 << input_size;        // equivalent to pow(2, input_size)

	for (std::size_t i = 0; i < max_combinations; ++i)
	{
		std::vector<std::string> current_combination;
		current_combination.reserve(input_size);        // reserve memory for maximum possible size

		for (std::size_t j = 0; j < input_size; ++j)
		{
			if (i & (1 << j))
			{
				current_combination.push_back(input_set[j]);
			}
		}

		output_set.push_back(current_combination);
	}

	return output_set;
}

std::string read_file(const std::string &file_name)
{
	std::ifstream file(file_name, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open shader file!");
	}

	size_t            file_size = (size_t) file.tellg();
	std::vector<char> buffer(file_size);
	file.seekg(0);
	file.read(buffer.data(), file_size);
	file.close();
	return {buffer.begin(), buffer.end()};
}

std::string expand_includes(const IncludeResolver &resolver, const std::string &contents)
{
	static RE2 include_regex("([ \\t]*#[ \\t]*include[ \\t]+\")([a-zA-Z0-9\\/\\._]+)(\"[ \\t]*)");
	assert(include_regex.ok());

	std::string expanded_contents = contents;

	std::string include_directive, matched_include, include_trail;
	while (RE2::PartialMatch(expanded_contents, include_regex, &include_directive, &matched_include, &include_trail))
	{
		auto   entire_include = include_directive + matched_include + include_trail;
		size_t pos            = expanded_contents.find(entire_include);
		size_t end            = pos + entire_include.size();        // keep the qoutes as they need to be removed

		std::string include_contents = read_file(resolver.resolve_include(matched_include));
		include_contents             = expand_includes(resolver, include_contents);

		expanded_contents.replace(pos, end - pos + 1, include_contents);

		pos += include_contents.size();
	}

	return expanded_contents;
}

ShaderVariant compile_and_reflect_shader_variant(const shaders::CompilerConfig &config, const std::vector<uint8_t> &contents)
{
	ShaderVariant shader;
	shader.hash = config.hash();

	GlslangShaderCompiler compiler;
	shader.spirv = compiler.compile_spirv(config, contents);

	SpirvReflectShaderReflector reflector;
	shader.resources = reflector.reflect_spirv(shader.spirv);

	return shader;
}

void compile_and_reflect_shader(ShaderDescriptor &descriptor)
{
	IncludeResolver resolver(descriptor.include_folders);

	auto define_combinations = generate_combinations(descriptor.variant_defines);
	auto contents            = expand_includes(resolver, read_file(descriptor.path));
	auto contents_bytes      = std::vector<uint8_t>(contents.begin(), contents.end());

	for (auto &define_combination : define_combinations)
	{
		// This is effectively a variant of the shader
		shaders::CompilerConfig config;
		config.stage = to_vk_shader_stage(descriptor.type);

		for (auto &define : descriptor.defines)
		{
			auto define_split = components::strings::split(define, "=");
			if (define_split.size() == 1)
				config.defines[define_split[0]] = "1";
			else if (define_split.size() == 2)
			{
				config.defines[define_split[0]] = define_split[1];
			}
			else
			{
				throw std::runtime_error("Invalid define: " + define);
			}
		}

		for (auto &define : define_combination)
		{
			auto define_split = components::strings::split(define, "=");
			if (define_split.size() == 1)
				config.defines[define_split[0]] = "1";
			else if (define_split.size() == 2)
			{
				config.defines[define_split[0]] = define_split[1];
			}
			else
			{
				throw std::runtime_error("Invalid define: " + define);
			}
		}

		config.entry_point = descriptor.entry_point;

		auto variant = compile_and_reflect_shader_variant(config, contents_bytes);

		descriptor.resources = shaders::ShaderResources::merge(descriptor.resources, variant.resources);

		descriptor.variants[config.hash()] = std::move(variant);
	}
}
}        // namespace shader_compiler