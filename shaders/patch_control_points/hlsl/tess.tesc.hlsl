/* Copyright (c) 2024, Sascha Willems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
struct UBO
{
	float4x4 projection;
	float4x4 view;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

struct UBOTessellation
{
	float tessellationFactor;
};
[[vk::binding(1, 0)]]
ConstantBuffer<UBOTessellation> ubo_tessellation : register(b1);

struct VSOutput
{
	float4 Pos : SV_POSITION;
};

struct HSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]]	float3 Color : COLOR0;
};

struct ConstantsHSOutput
{
    float TessLevelOuter[4] : SV_TessFactor;
    float TessLevelInner[2] : SV_InsideTessFactor;
};

float getTessLevel(float4 p0, float4 p1)
{
	float       tessellationLevel = 0.0f;
	const float midDistance       = 1.6f;
	const float shortDistance     = 1.0f;

	// Calculate edge mid point
	float4 centerPoint = 0.5f * (p0 + p1);

	// Calculate vector from camera to mid point
	float4 vCam = mul(ubo.view, centerPoint);

	// Calculate vector for camera projection
	float4 vCamView = mul(ubo.projection, vCam);

	// Calculate the vector length
	float centerDistance = length(vCamView);

	// Adjusting the size of the tessellation depending on the length of the vector
	if (centerDistance >= midDistance)
	{
		tessellationLevel = 1.0f;
	}
	else if (centerDistance >= shortDistance && centerDistance < midDistance)
	{
		tessellationLevel = ubo_tessellation.tessellationFactor * 0.4f;
	}
	else
	{
		tessellationLevel = ubo_tessellation.tessellationFactor;
	}

	return tessellationLevel;
}

float3 getColor(float tessellationLevel)
{
	float3 outColor;
	if (tessellationLevel == 1.0f)
	{
		outColor = float3(1.0f, 0.0f, 0.0f);        // red color
	}
	else if (tessellationLevel == ubo_tessellation.tessellationFactor * 0.4f)
	{
		outColor = float3(0.0f, 0.0f, 1.0f);        // blue color
	}
	else if (tessellationLevel == ubo_tessellation.tessellationFactor)
	{
		outColor = float3(0.0f, 1.0f, 0.0f);        // green color
	}

	return outColor;
}

ConstantsHSOutput ConstantsHS(InputPatch<VSOutput, 3> patch)
{
    ConstantsHSOutput output = (ConstantsHSOutput)0;

	if (ubo_tessellation.tessellationFactor > 1.0)
	{
		output.TessLevelOuter[0] = getTessLevel(patch[2].Pos, patch[0].Pos);
		output.TessLevelOuter[1] = getTessLevel(patch[0].Pos, patch[1].Pos);
		output.TessLevelOuter[2] = getTessLevel(patch[1].Pos, patch[2].Pos);
		output.TessLevelInner[0] = lerp(output.TessLevelOuter[0], output.TessLevelOuter[2], 0.5);
	}
	else
	{
		output.TessLevelOuter[0] = 1;
		output.TessLevelOuter[1] = 1;
		output.TessLevelOuter[2] = 1;
		output.TessLevelInner[0] = 1;
	}

    return output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantsHS")]
[maxtessfactor(20.0)]
HSOutput main(InputPatch<VSOutput, 3> patch, uint InvocationID : SV_OutputControlPointID)
{
	HSOutput output = (HSOutput)0;
	output.Pos = patch[InvocationID].Pos;
	output.Color = getColor(getTessLevel(patch[2].Pos, patch[0].Pos)), 1.0;
	return output;
}
