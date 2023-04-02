#include "header_generator.hpp"

#include <components/common/logging.hpp>

#include <fstream>
#include <iostream>
#include <string>

namespace shader_compiler
{
using namespace components;

namespace detail
{
std::string snake_to_pascal(const std::string &snake)
{
	std::string pascal;

	for (size_t i = 0; i < snake.size(); i++)
	{
		if (i == 0)
		{
			pascal += toupper(snake[0]);
			continue;
		}

		if (snake[i] == '_')
		{
			pascal += toupper(snake[i + 1]);
			i++;
			continue;
		}

		pascal += snake[i];
	}

	return pascal;
}
}        // namespace detail

void generate_raw_variant_pointers(const ShaderDescriptor &descriptor, std::ofstream &output_stream)
{
	output_stream << "// clang-format off" << std::endl;

	for (const auto &variant_pair : descriptor.variants)
	{
		auto &hash    = variant_pair.first;
		auto &variant = variant_pair.second;

		output_stream << "\tconstexpr uint32_t " << detail::snake_to_pascal(descriptor.name) << "__" << hash << "[] = [" << std::endl;

		for (size_t i = 0; i < variant.spirv.size(); i++)
		{
			if (i % 50 == 0)
			{
				if (i != 0)
				{
					output_stream << std::endl;
				}

				output_stream << "\t\t";
				output_stream << variant.spirv[i];

				if (i != variant.spirv.size() - 1)
				{
					output_stream << ",";
				}
			}
			else
			{
				output_stream << variant.spirv[i];

				if (i != variant.spirv.size() - 1)
				{
					output_stream << ",";
				}
			}
		}

		output_stream << "];" << std::endl;
		output_stream << std::endl;

		output_stream << "\tconstexpr size_t " << detail::snake_to_pascal(descriptor.name) << "__" << hash << "_size = " << variant.spirv.size() << ";" << std::endl;
		output_stream << std::endl;
	}

	output_stream << "// clang-format on" << std::endl;
	output_stream << std::endl;
}

void generate_resource_definitions(const ShaderDescriptor &descriptor, std::ofstream &output_stream)
{
	auto structs = descriptor.resources.structs_in_topological_order();

	for (const auto &type : structs)
	{
		output_stream << "\t\tstruct " << detail::snake_to_pascal(type.type_name) << std::endl;
		output_stream << "\t\t{" << std::endl;

		for (const auto &member : type.members)
		{
			switch (member->type)
			{
				case shaders::ShaderRawDataType::Array: {
					auto array_member = std::static_pointer_cast<shaders::ArrayShaderData>(member);

					output_stream << "\t\t\t"
					              << "std::array<" << array_member->type_name << ", " << array_member->element_count << "> " << array_member->struct_member_name << ";" << std::endl;
					continue;
				}

				case shaders::ShaderRawDataType::Vector: {
					auto vector_member = std::static_pointer_cast<shaders::VectorShaderData>(member);

					output_stream << "\t\t\t"
					              << "std::array<" << vector_member->type_name << ", " << vector_member->component_count << "> " << vector_member->struct_member_name << ";" << std::endl;
					continue;
				}

				case shaders::ShaderRawDataType::Matrix: {
					auto matrix_member = std::static_pointer_cast<shaders::MatrixShaderData>(member);

					output_stream << "\t\t\t"
					              << "std::array<std::array<" << matrix_member->type_name << ", " << matrix_member->columns << ">, " << matrix_member->rows << "> " << matrix_member->struct_member_name << ";" << std::endl;
					continue;
				}

				case shaders::ShaderRawDataType::Bool:
				case shaders::ShaderRawDataType::Int:
				case shaders::ShaderRawDataType::Float: {
					auto numeric_member = std::static_pointer_cast<shaders::NumericShaderData>(member);

					output_stream << "\t\t\t" << numeric_member->type_name << " " << numeric_member->struct_member_name << ";" << std::endl;
					continue;
				}

				case shaders::ShaderRawDataType::Struct: {
					auto struct_member = std::static_pointer_cast<shaders::StructShaderData>(member);
					output_stream << "\t\t\t" << detail::snake_to_pascal(struct_member->type_name) << " " << struct_member->struct_member_name << ";" << std::endl;
					continue;
				}

				case shaders::ShaderRawDataType::Unknown: {
					LOGW("Unknown shader data type encountered! - reflector not yet supported for this type!");
					continue;
				}

				default: {
					output_stream << "\t\t\t"
					              << "UnknownType " << (member->struct_member_name.empty() ? "unknown" : member->struct_member_name) << ";" << std::endl;
				}
			}
		}

		output_stream << "\t\t};" << std::endl;
		output_stream << std::endl;
	}
}

void generate_interface_bindings(const ShaderDescriptor &descriptor, std::ofstream &output_stream)
{
}

void generate_class_definition(const ShaderDescriptor &descriptor, std::ofstream &output_stream)
{
	output_stream << "class " << detail::snake_to_pascal(descriptor.name) << std::endl;
	output_stream << "{" << std::endl;
	output_stream << "\tpublic:" << std::endl;

	output_stream << "\t\t"
	              << "using UnknownType = uint32_t; // used to represent poorly reflected types" << std::endl;
	output_stream << std::endl;

	generate_resource_definitions(descriptor, output_stream);
	generate_interface_bindings(descriptor, output_stream);

	output_stream << "\t\tstd::vector<uint32_t> get_variant(const shaders::CompilerConfig &variant) const" << std::endl;
	output_stream << "\t\t{" << std::endl;

	output_stream << "\t\t\tswitch (variant.hash())" << std::endl;
	output_stream << "\t\t\t{" << std::endl;

	for (const auto &variant_pair : descriptor.variants)
	{
		auto &hash    = variant_pair.first;
		auto &variant = variant_pair.second;

		output_stream << "\t\t\t\tcase " << hash << ":" << std::endl;
		output_stream << "\t\t\t\t\treturn std::vector<uint32_t>(" << detail::snake_to_pascal(descriptor.name) << "__" << hash << ", " << detail::snake_to_pascal(descriptor.name) << "__" << hash << " + " << detail::snake_to_pascal(descriptor.name) << "__" << hash << "_size);" << std::endl;
		output_stream << "\t\t\t\t\tbreak;" << std::endl;
	}

	output_stream << "\t\t\t\tdefault:" << std::endl;
	output_stream << "\t\t\t\t\tthrow std::runtime_error(\"Invalid shader variant\");" << std::endl;
	output_stream << "\t\t\t}" << std::endl;

	output_stream << "};" << std::endl;
}

void generate_shader_header(const ShaderDescriptor &descriptor, const std::string &output_file)
{
	std::ofstream output_stream(output_file);

	if (!output_stream.is_open())
	{
		throw std::runtime_error("Could not open output file for writing");
	}

	output_stream << "#pragma once" << std::endl;
	output_stream << std::endl;
	output_stream << "/* DO NOT EDIT */" << std::endl;
	output_stream << "/* This file was generated by the shader compiler */" << std::endl;
	output_stream << std::endl;

	output_stream << "#include <cstdint>" << std::endl;
	output_stream << "#include <vector>" << std::endl;
	output_stream << "#include <components/shaders/shader_resources.hpp>" << std::endl;
	output_stream << std::endl;

	output_stream << "namespace shaders" << std::endl;
	output_stream << "{" << std::endl;
	output_stream << std::endl;

	generate_raw_variant_pointers(descriptor, output_stream);
	generate_class_definition(descriptor, output_stream);

	output_stream << std::endl;
	output_stream << "}        // namespace shaders" << std::endl;

	output_stream.close();
}
}        // namespace shader_compiler