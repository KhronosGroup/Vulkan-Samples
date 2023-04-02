#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <SPIRV/GLSL.std.450.h>
#include <SPIRV/GlslangToSpv.h>
#include <glslang/OSDependent/osinclude.h>
VKBP_ENABLE_WARNINGS()

namespace components
{
const TBuiltInResource *GetDefaultResourceLimits();
}
