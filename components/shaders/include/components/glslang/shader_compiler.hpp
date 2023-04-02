#pragma once

#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <glslang/Public/ShaderLang.h>
VKBP_ENABLE_WARNINGS()

#include <components/shaders/compiler.hpp>

namespace components
{
namespace glslang
{
using namespace ::glslang;

using LogCallback = std::function<void(const std::string &)>;

class GlslangShaderCompiler : public shaders::ShaderCompiler
{
  public:
	GlslangShaderCompiler(LogCallback &&log_callback = nullptr);
	~GlslangShaderCompiler() = default;

	// Compile shader source code to SPIR-V
	std::vector<uint32_t> compile_spirv(const shaders::CompilerConfig &config, const std::vector<uint8_t> &shader_source) const override;

	EShTargetLanguage        target_language         = EShTargetLanguage::EShTargetSpv;
	EShTargetLanguageVersion target_language_version = EShTargetLanguageVersion::EShTargetSpv_1_5;

  private:
	LogCallback log_callback;
};
}        // namespace glslang
}        // namespace components