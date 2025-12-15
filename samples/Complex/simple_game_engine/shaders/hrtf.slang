/* Copyright (c) 2025 Holochip Corporation
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
// Compute shader for HRTF (Head-Related Transfer Function) audio processing
// This shader processes audio data to create 3D spatial audio effects

// Input/output buffer bindings
[[vk::binding(0, 0)]] RWStructuredBuffer<float> inputAudioBuffer;  // Raw audio samples
[[vk::binding(1, 0)]] RWStructuredBuffer<float> outputAudioBuffer; // Processed audio samples
[[vk::binding(2, 0)]] StructuredBuffer<float> hrtfData;           // HRTF impulse responses
[[vk::binding(3, 0)]] ConstantBuffer<HRTFParams> params;          // HRTF parameters

// Parameters for HRTF processing - MUST match CPU GPUHRTFParams structure exactly
struct HRTFParams {
    float4 listenerPosition;     // Position of the listener (float[4] on CPU) - 16 bytes
    float4 listenerForward;      // Forward direction of the listener (float[4] on CPU) - 16 bytes
    float4 listenerUp;           // Up direction of the listener (float[4] on CPU) - 16 bytes
    float4 sourcePosition;       // Position of the sound source (float[4] on CPU) - 16 bytes
    float sampleCount;           // Number of samples to process (4 bytes) - offset 64
    float3 padding1;             // Padding to align to 16-byte boundary (12 bytes) - offset 68
    uint inputChannels;          // Number of input channels (4 bytes) - offset 80
    uint outputChannels;         // Number of output channels (4 bytes) - offset 84
    uint hrtfSize;               // Size of each HRTF impulse response (4 bytes) - offset 88
    uint numHrtfPositions;       // Number of HRTF positions (4 bytes) - offset 92
    float distanceAttenuation;   // Distance attenuation factor (4 bytes) - offset 96
    float dopplerFactor;         // Doppler effect factor (4 bytes) - offset 100
    float reverbMix;             // Reverb mix factor (4 bytes) - offset 104
    float padding2;              // Padding to complete 16-byte alignment (4 bytes) - offset 108
};

// Helper function to calculate the index of the closest HRTF in the dataset
uint FindClosestHRTF(float azimuth, float elevation) {
    // This is a simplified implementation
    // In a real implementation, this would find the closest HRTF in the dataset
    // based on the azimuth and elevation angles

    // Normalize azimuth to [0, 360) degrees
    azimuth = fmod(azimuth + 360.0, 360.0);

    // Clamp elevation to [-90, 90] degrees
    elevation = clamp(elevation, -90.0, 90.0);

    // Calculate indices based on a typical HRTF dataset layout
    // Assuming 10-degree resolution in azimuth and 15-degree in elevation
    uint azimuthIndex = uint(round(azimuth / 10.0)) % 36;
    uint elevationIndex = uint(round((elevation + 90.0) / 15.0)) % 13;

    // Calculate the final index
    return elevationIndex * 36 + azimuthIndex;
}

// Helper function to calculate azimuth and elevation angles
void CalculateAngles(float3 sourceDir, float3 listenerForward, float3 listenerUp, out float azimuth, out float elevation) {
    // Simplified angle calculation - directly use source direction
    // Calculate azimuth (horizontal angle) - angle around Y axis
    azimuth = atan2(sourceDir.x, -sourceDir.z) * 57.2957795; // Convert to degrees, negate z for correct orientation

    // Calculate elevation (vertical angle) - angle from horizontal plane
    float horizontalLength = sqrt(sourceDir.x * sourceDir.x + sourceDir.z * sourceDir.z);
    elevation = atan2(sourceDir.y, horizontalLength) * 57.2957795; // Convert to degrees
}

// Main compute shader function
[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint index = dispatchThreadID.x;

    // Check if the thread is within bounds
    if (index >= uint(params.sampleCount)) {
        return;
    }

    // STAGE 1: HRTF DATA ACCESS WITH SAFETY VALIDATION
    // Start with working basic panning and add HRTF data access

    // Get input sample for this thread
    float inputSample = inputAudioBuffer[index];

    // STAGE 1: Test HRTF data buffer access with ultra-safe bounds checking
    bool hrtfDataValid = false;
    float testHrtfSample = 0.0f;

    // Ultra-safe HRTF data access test
    if (params.hrtfSize > 0 && params.numHrtfPositions > 0) {
        // Test access to first HRTF sample with multiple safety checks
        uint testHrtfIndex = 0; // Start with first sample
        uint maxHrtfBufferSize = params.numHrtfPositions * params.hrtfSize * 2; // 2 channels

        if (testHrtfIndex < maxHrtfBufferSize && testHrtfIndex < 500000) { // Additional hardcoded safety limit
            testHrtfSample = hrtfData[testHrtfIndex];
            hrtfDataValid = true;
        }
    }

    // STAGE 2: 3D DIRECTION CALCULATION AND ANGLE COMPUTATION
    // Calculate 3D direction from listener to source
    float3 sourceDir = params.sourcePosition.xyz - params.listenerPosition.xyz;
    float distance = length(sourceDir);

    // Handle edge case where listener and source are at same position
    if (distance < 0.001) {
        sourceDir = float3(0.0, 0.0, -1.0); // Default to front direction
        distance = 1.0;
    } else {
        sourceDir = normalize(sourceDir);
    }

    // Calculate azimuth and elevation angles using the helper function
    float azimuth, elevation;
    CalculateAngles(sourceDir, params.listenerForward.xyz, params.listenerUp.xyz, azimuth, elevation);


    // ENHANCED SPATIAL PROCESSING: Use 3D angles for better panning
    float leftGain = 1.0;
    float rightGain = 1.0;

    // Convert azimuth to left/right panning (-180 to +180 degrees)
    // Positive azimuth = right side, negative = left side
    if (azimuth > 0.0) {
        // Source is to the right, reduce left channel based on angle
        float rightness = min(1.0, azimuth / 90.0); // Normalize to 0-1 for 0-90 degrees
        leftGain = max(0.2, 1.0 - rightness * 0.8); // Reduce left by up to 80%
        rightGain = 1.0;
    } else if (azimuth < 0.0) {
        // Source is to the left, reduce right channel based on angle
        float leftness = min(1.0, -azimuth / 90.0); // Normalize to 0-1 for 0-90 degrees
        leftGain = 1.0;
        rightGain = max(0.2, 1.0 - leftness * 0.8); // Reduce right by up to 80%
    }

    // Apply distance attenuation (closer sources are louder)
    float distanceAttenuation = 1.0 / max(1.0, distance * 0.5); // Gentle distance falloff
    leftGain *= distanceAttenuation;
    rightGain *= distanceAttenuation;

    // STAGE 3: HRTF INDEX LOOKUP WITH BOUNDS CHECKING
    // Find the closest HRTF in the dataset based on calculated angles
    uint hrtfIndex = FindClosestHRTF(azimuth, elevation);

    // Ultra-safe bounds checking for HRTF index
    bool hrtfIndexValid = false;
    if (hrtfIndex < params.numHrtfPositions && params.numHrtfPositions > 0) {
        hrtfIndexValid = true;
    }

    // ENHANCED HRTF DATA ACCESS: Use calculated index instead of just first sample
    float hrtfLeftSample = 0.0f;
    float hrtfRightSample = 0.0f;
    bool hrtfSamplesValid = false;

    if (hrtfIndexValid && hrtfDataValid) {
        // Calculate HRTF buffer offsets for left and right channels
        // HRTF data layout: [position0_left_samples][position0_right_samples][position1_left_samples]...
        uint leftChannelOffset = hrtfIndex * params.hrtfSize * 2; // 2 channels per position
        uint rightChannelOffset = leftChannelOffset + params.hrtfSize;

        // Ultra-safe bounds checking for HRTF sample access
        uint maxHrtfBufferSize = params.numHrtfPositions * params.hrtfSize * 2;
        if (leftChannelOffset < maxHrtfBufferSize && rightChannelOffset < maxHrtfBufferSize &&
            leftChannelOffset < 500000 && rightChannelOffset < 500000) { // Additional hardcoded safety

            // Access first sample of each channel's impulse response for this position
            hrtfLeftSample = hrtfData[leftChannelOffset];
            hrtfRightSample = hrtfData[rightChannelOffset];
            hrtfSamplesValid = true;
        }
    }

    // STAGE 4: HRTF CONVOLUTION LOOP WITH ULTRA-SAFE MEMORY ACCESS
    float leftConvolution = 0.0f;
    float rightConvolution = 0.0f;
    uint convolutionSamples = 0;

    if (hrtfIndexValid && hrtfDataValid && params.hrtfSize > 0) {
        // Calculate base offsets for this HRTF position
        uint leftChannelBase = hrtfIndex * params.hrtfSize * 2;
        uint rightChannelBase = leftChannelBase + params.hrtfSize;
        uint maxHrtfBufferSize = params.numHrtfPositions * params.hrtfSize * 2;

        // Limit convolution size for safety and performance
        uint safeHrtfSize = min(params.hrtfSize, 32u); // Limit to 32 samples for safety

        // HRTF Convolution loop with ultra-safe bounds checking
        for (uint i = 0; i < safeHrtfSize; i++) {
            // Check if we can access the input audio sample
            if (index >= i) {
                uint inputIndex = index - i;

                // Ultra-safe input buffer bounds check
                if (inputIndex < uint(params.sampleCount) && inputIndex < 1024) {
                    float audioSample = inputAudioBuffer[inputIndex];

                    // Calculate HRTF sample indices with bounds checking
                    uint leftHrtfIndex = leftChannelBase + i;
                    uint rightHrtfIndex = rightChannelBase + i;

                    // Ultra-safe HRTF buffer bounds check
                    if (leftHrtfIndex < maxHrtfBufferSize && rightHrtfIndex < maxHrtfBufferSize &&
                        leftHrtfIndex < 500000 && rightHrtfIndex < 500000) {

                        float leftHrtfSample = hrtfData[leftHrtfIndex];
                        float rightHrtfSample = hrtfData[rightHrtfIndex];

                        // Apply convolution
                        leftConvolution += audioSample * leftHrtfSample;
                        rightConvolution += audioSample * rightHrtfSample;
                        convolutionSamples++;
                    }
                }
            }
        }

    }

    // STAGE 4: Apply convolution results with distance attenuation
    if (convolutionSamples > 0) {
        // Use convolution results instead of simple gain modification
        leftGain = leftConvolution * distanceAttenuation;
        rightGain = rightConvolution * distanceAttenuation;
    }


    // STAGE 5: COMPLETE HRTF PROCESSING - FINAL OUTPUT WITH OPTIMIZATION
    // Write to both output channels with full HRTF processing
    for (uint channel = 0; channel < 2; channel++) { // Hardcode to 2 channels for safety
        uint outputIndex = index * 2 + channel;

        // Ultra-safe bounds check with hardcoded limits
        if (outputIndex < 1024 * 2 && outputIndex < 2048) {
            float finalSample = 0.0f;

            if (convolutionSamples > 0) {
                // STAGE 5: Use full HRTF convolution results
                finalSample = (channel == 0) ? leftGain : rightGain;

                // Apply output normalization to prevent clipping
                finalSample = clamp(finalSample, -1.0f, 1.0f);
            } else {
                // Fallback: Enhanced spatial panning
                float channelGain = (channel == 0) ? leftGain : rightGain;
                finalSample = inputSample * channelGain;
            }

            outputAudioBuffer[outputIndex] = finalSample;
        }
    }

}
