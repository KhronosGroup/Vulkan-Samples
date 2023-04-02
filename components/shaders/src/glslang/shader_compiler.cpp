#include "components/glslang/shader_compiler.hpp"

#include "default_resource_limits.h"

namespace components
{
namespace glslang
{
namespace detail
{
inline EShLanguage find_shader_language(VkShaderStageFlagBits stage)
{
	switch (stage)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:
			return EShLangVertex;

		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
			return EShLangTessControl;

		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
			return EShLangTessEvaluation;

		case VK_SHADER_STAGE_GEOMETRY_BIT:
			return EShLangGeometry;

		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return EShLangFragment;

		case VK_SHADER_STAGE_COMPUTE_BIT:
			return EShLangCompute;

		case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
			return EShLangRayGen;

		case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
			return EShLangAnyHit;

		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
			return EShLangClosestHit;

		case VK_SHADER_STAGE_MISS_BIT_KHR:
			return EShLangMiss;

		case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
			return EShLangIntersect;

		case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
			return EShLangCallable;

		case VK_SHADER_STAGE_MESH_BIT_EXT:
			return EShLangMesh;

		case VK_SHADER_STAGE_TASK_BIT_EXT:
			return EShLangTask;

		default:
			return EShLangVertex;
	}
}
}        // namespace detail

GlslangShaderCompiler::GlslangShaderCompiler(LogCallback &&log_callback) :
    log_callback(std::move(log_callback))
{
}

std::vector<uint32_t> GlslangShaderCompiler::compile_spirv(const shaders::CompilerConfig &config, const std::vector<uint8_t> &shader_source) const
{
	if (shader_source.size() == 0)
	{
		throw std::runtime_error{"Shader source is empty"};
	}

	if (!config.is_valid())
	{
		throw std::runtime_error{"Invalid compiler config"};
	}

	if (!glslang::InitializeProcess())
	{
		throw std::runtime_error{"Failed to initialize glslang"};
	}

	EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);

	EShLanguage language = detail::find_shader_language(config.stage);
	std::string source   = std::string(shader_source.begin(), shader_source.end());

	const char *file_name_list[1]  = {""};
	const char *shader_source_data = reinterpret_cast<const char *>(source.data());

	glslang::TShader shader(language);
	shader.setStringsWithLengthsAndNames(&shader_source_data, nullptr, file_name_list, 1);
	shader.setEntryPoint(config.entry_point.c_str());
	shader.setSourceEntryPoint(config.entry_point.c_str());
	shader.setEnvTarget(target_language, target_language_version);
	shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);

	std::string preamble;
	for (const auto &define : config.defines)
	{
		preamble += "#define " + define.first + " " + define.second + "\n";
	}
	shader.setPreamble(preamble.c_str());

	if (!shader.parse(GetDefaultResourceLimits(), 110, false, messages))
	{
		throw std::runtime_error{"Failed to parse shader source:\n\t" + std::string{shader.getInfoLog()}};
	}

	// Add shader to new program object.
	glslang::TProgram program;
	program.addShader(&shader);

	// Link program.
	if (!program.link(messages))
	{
		throw std::runtime_error{"Failed to link shader program:\n\t" + std::string{program.getInfoLog()}};
	}

	glslang::TIntermediate *intermediate = program.getIntermediate(language);

	// Translate to SPIRV.
	if (!intermediate)
	{
		throw std::runtime_error{"Failed to translate shader to SPIR-V"};
	}

	spv::SpvBuildLogger logger;

	std::vector<uint32_t> spirv;
	glslang::GlslangToSpv(*intermediate, spirv, &logger);

	if (log_callback)
	{
		log_callback(logger.getAllMessages());
	}

	glslang::FinalizeProcess();

	return spirv;
}

}        // namespace glslang
}        // namespace components