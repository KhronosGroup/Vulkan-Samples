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
#include "audio_system.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <utility>

// OpenAL headers
#ifdef __APPLE__
#	include <OpenAL/al.h>
#	include <OpenAL/alc.h>
#else
#	include <AL/al.h>
#	include <AL/alc.h>
#endif

#include "engine.h"
#include "renderer.h"

// OpenAL error checking utility
static void CheckOpenALError(const std::string &operation)
{
	ALenum error = alGetError();
	if (error != AL_NO_ERROR)
	{
		std::cerr << "OpenAL Error in " << operation << ": ";
		switch (error)
		{
			case AL_INVALID_NAME:
				std::cerr << "AL_INVALID_NAME";
				break;
			case AL_INVALID_ENUM:
				std::cerr << "AL_INVALID_ENUM";
				break;
			case AL_INVALID_VALUE:
				std::cerr << "AL_INVALID_VALUE";
				break;
			case AL_INVALID_OPERATION:
				std::cerr << "AL_INVALID_OPERATION";
				break;
			case AL_OUT_OF_MEMORY:
				std::cerr << "AL_OUT_OF_MEMORY";
				break;
			default:
				std::cerr << "Unknown error " << error;
				break;
		}
		std::cerr << std::endl;
	}
}

// Concrete implementation of AudioSource
class ConcreteAudioSource : public AudioSource
{
  public:
	explicit ConcreteAudioSource(std::string name) :
	    name(std::move(name))
	{}
	~ConcreteAudioSource() override = default;

	void Play() override
	{
		playing           = true;
		playbackPosition  = 0;
		delayTimer        = std::chrono::milliseconds(0);
		inDelayPhase      = false;
		sampleAccumulator = 0.0;
	}

	void Pause() override
	{
		playing = false;
	}

	void Stop() override
	{
		playing           = false;
		playbackPosition  = 0;
		delayTimer        = std::chrono::milliseconds(0);
		inDelayPhase      = false;
		sampleAccumulator = 0.0;
	}

	void SetVolume(float volume) override
	{
		this->volume = volume;
	}

	void SetLoop(bool loop) override
	{
		this->loop = loop;
	}

	void SetPosition(float x, float y, float z) override
	{
		position[0] = x;
		position[1] = y;
		position[2] = z;
	}

	void SetVelocity(float x, float y, float z) override
	{
		velocity[0] = x;
		velocity[1] = y;
		velocity[2] = z;
	}

	[[nodiscard]] bool IsPlaying() const override
	{
		return playing;
	}

	// Additional methods for delay functionality
	void SetAudioLength(uint32_t lengthInSamples)
	{
		audioLengthSamples = lengthInSamples;
	}

	void UpdatePlayback(std::chrono::milliseconds deltaTime, uint32_t samplesProcessed)
	{
		if (!playing)
			return;

		if (inDelayPhase)
		{
			// We're in the delay phase between playthroughs
			delayTimer += deltaTime;
			if (delayTimer >= delayDuration)
			{
				// Delay finished, restart playback
				inDelayPhase     = false;
				playbackPosition = 0;
				delayTimer       = std::chrono::milliseconds(0);
			}
		}
		else
		{
			// Normal playback, update position
			playbackPosition += samplesProcessed;

			// Check if we've reached the end of the audio
			if (audioLengthSamples > 0 && playbackPosition >= audioLengthSamples)
			{
				if (loop)
				{
					// Start the delay phase before looping
					inDelayPhase = true;
					delayTimer   = std::chrono::milliseconds(0);
				}
				else
				{
					// Stop playing if not looping
					playing          = false;
					playbackPosition = 0;
				}
			}
		}
	}

	[[nodiscard]] bool ShouldProcessAudio() const
	{
		return playing && !inDelayPhase;
	}

	[[nodiscard]] uint32_t GetPlaybackPosition() const
	{
		return playbackPosition;
	}

	[[nodiscard]] const std::string &GetName() const
	{
		return name;
	}

	[[nodiscard]] const float *GetPosition() const
	{
		return position;
	}

	[[nodiscard]] double GetSampleAccumulator() const
	{
		return sampleAccumulator;
	}

	void SetSampleAccumulator(double value)
	{
		sampleAccumulator = value;
	}

  private:
	std::string name;
	bool        playing     = false;
	bool        loop        = false;
	float       volume      = 1.0f;
	float       position[3] = {0.0f, 0.0f, 0.0f};
	float       velocity[3] = {0.0f, 0.0f, 0.0f};

	// Delay and timing functionality
	uint32_t                                   playbackPosition   = 0;                                      // Current position in samples
	uint32_t                                   audioLengthSamples = 0;                                      // Total length of audio in samples
	std::chrono::milliseconds                  delayTimer         = std::chrono::milliseconds(0);           // Timer for delay between loops
	bool                                       inDelayPhase       = false;                                  // Whether we're currently in the delay phase
	static constexpr std::chrono::milliseconds delayDuration      = std::chrono::milliseconds(1500);        // 1.5-second delay between loops
	double                                     sampleAccumulator  = 0.0;                                    // Per-source sample accumulator for proper timing
};

// OpenAL audio output device implementation
class OpenALAudioOutputDevice : public AudioOutputDevice
{
  public:
	OpenALAudioOutputDevice() = default;
	~OpenALAudioOutputDevice() override
	{
		OpenALAudioOutputDevice::Stop();
		Cleanup();
	}

	bool Initialize(uint32_t sampleRate, uint32_t channels, uint32_t bufferSize) override
	{
		this->sampleRate = sampleRate;
		this->channels   = channels;
		this->bufferSize = bufferSize;

		// Initialize OpenAL
		device = alcOpenDevice(nullptr);        // Use default device
		if (!device)
		{
			std::cerr << "Failed to open OpenAL device" << std::endl;
			return false;
		}

		context = alcCreateContext(device, nullptr);
		if (!context)
		{
			std::cerr << "Failed to create OpenAL context" << std::endl;
			alcCloseDevice(device);
			device = nullptr;
			return false;
		}

		if (!alcMakeContextCurrent(context))
		{
			std::cerr << "Failed to make OpenAL context current" << std::endl;
			alcDestroyContext(context);
			alcCloseDevice(device);
			context = nullptr;
			device  = nullptr;
			return false;
		}

		// Generate OpenAL source
		alGenSources(1, &source);
		CheckOpenALError("alGenSources");

		// Generate OpenAL buffers for streaming
		alGenBuffers(NUM_BUFFERS, buffers);
		CheckOpenALError("alGenBuffers");

		// Set source properties
		alSourcef(source, AL_PITCH, 1.0f);
		alSourcef(source, AL_GAIN, 1.0f);
		alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
		alSourcei(source, AL_LOOPING, AL_FALSE);
		CheckOpenALError("Source setup");

		// Initialize audio buffer
		audioBuffer.resize(bufferSize * channels);

		// Initialize buffer tracking
		queuedBufferCount = 0;
		while (!availableBuffers.empty())
		{
			availableBuffers.pop();
		}

		initialized = true;
		return true;
	}

	bool Start() override
	{
		if (!initialized)
		{
			std::cerr << "OpenAL audio output device not initialized" << std::endl;
			return false;
		}

		if (playing)
		{
			return true;        // Already playing
		}

		playing = true;

		// Start an audio playback thread
		audioThread = std::thread(&OpenALAudioOutputDevice::AudioThreadFunction, this);

		return true;
	}

	bool Stop() override
	{
		if (!playing)
		{
			return true;        // Already stopped
		}

		playing = false;

		// Wait for the audio thread to finish
		if (audioThread.joinable())
		{
			audioThread.join();
		}

		// Stop OpenAL source
		if (initialized && source != 0)
		{
			alSourceStop(source);
			CheckOpenALError("alSourceStop");
		}

		return true;
	}

	bool WriteAudio(const float *data, uint32_t sampleCount) override
	{
		if (!initialized || !playing)
		{
			return false;
		}

		std::lock_guard<std::mutex> lock(bufferMutex);

		// Add audio data to the queue
		for (uint32_t i = 0; i < sampleCount * channels; i++)
		{
			audioQueue.push(data[i]);
		}

		return true;
	}

	[[nodiscard]] bool IsPlaying() const override
	{
		return playing;
	}

	[[nodiscard]] uint32_t GetPosition() const override
	{
		return playbackPosition;
	}

  private:
	static constexpr int NUM_BUFFERS = 8;

	uint32_t sampleRate       = 44100;
	uint32_t channels         = 2;
	uint32_t bufferSize       = 1024;
	bool     initialized      = false;
	bool     playing          = false;
	uint32_t playbackPosition = 0;

	// OpenAL objects
	ALCdevice  *device  = nullptr;
	ALCcontext *context = nullptr;
	ALuint      source  = 0;
	ALuint      buffers[NUM_BUFFERS]{};
	int         currentBuffer = 0;

	std::vector<float> audioBuffer;
	std::queue<float>  audioQueue;
	std::mutex         bufferMutex;
	std::thread        audioThread;

	// Buffer management for OpenAL streaming
	std::queue<ALuint> availableBuffers;
	int                queuedBufferCount = 0;

	void Cleanup()
	{
		if (initialized)
		{
			// Clean up OpenAL resources
			if (source != 0)
			{
				alDeleteSources(1, &source);
				source = 0;
			}

			alDeleteBuffers(NUM_BUFFERS, buffers);

			if (context)
			{
				alcMakeContextCurrent(nullptr);
				alcDestroyContext(context);
				context = nullptr;
			}

			if (device)
			{
				alcCloseDevice(device);
				device = nullptr;
			}

			// Reset buffer tracking
			queuedBufferCount = 0;
			while (!availableBuffers.empty())
			{
				availableBuffers.pop();
			}

			initialized = false;
		}
	}

	void AudioThreadFunction()
	{
		// Calculate sleep time for audio buffer updates (in milliseconds)
		const auto sleepTime = std::chrono::milliseconds(
		    static_cast<int>((bufferSize * 1000) / sampleRate / 8)        // Eighth buffer time for responsiveness
		);

		while (playing)
		{
			ProcessAudioBuffer();
			std::this_thread::sleep_for(sleepTime);
		}
	}

	void ProcessAudioBuffer()
	{
		std::lock_guard<std::mutex> lock(bufferMutex);

		// Fill audio buffer from queue in whole stereo frames to preserve channel alignment
		uint32_t       samplesProcessed = 0;
		const uint32_t framesAvailable  = static_cast<uint32_t>(audioQueue.size() / channels);
		if (framesAvailable == 0)
		{
			// Not enough data for a whole frame yet
			return;
		}
		const uint32_t framesToSend  = std::min(framesAvailable, bufferSize);
		const uint32_t samplesToSend = framesToSend * channels;
		for (uint32_t i = 0; i < samplesToSend; i++)
		{
			audioBuffer[i] = audioQueue.front();
			audioQueue.pop();
		}
		samplesProcessed = samplesToSend;

		if (samplesProcessed > 0)
		{
			// Convert float samples to 16-bit PCM for OpenAL
			std::vector<int16_t> pcmBuffer(samplesProcessed);
			for (uint32_t i = 0; i < samplesProcessed; i++)
			{
				// Clamp and convert to 16-bit PCM
				float sample = std::clamp(audioBuffer[i], -1.0f, 1.0f);
				pcmBuffer[i] = static_cast<int16_t>(sample * 32767.0f);
			}

			// Check for processed buffers and unqueue them
			ALint processed = 0;
			alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
			CheckOpenALError("alGetSourcei AL_BUFFERS_PROCESSED");

			// Unqueue processed buffers and add them to available buffers
			while (processed > 0)
			{
				ALuint buffer;
				alSourceUnqueueBuffers(source, 1, &buffer);
				CheckOpenALError("alSourceUnqueueBuffers");

				// Add the unqueued buffer to available buffers
				availableBuffers.push(buffer);
				processed--;
			}

			// Only proceed if we have an available buffer
			ALuint buffer = 0;
			if (!availableBuffers.empty())
			{
				buffer = availableBuffers.front();
				availableBuffers.pop();
			}
			else if (queuedBufferCount < NUM_BUFFERS)
			{
				// Use a buffer that hasn't been queued yet
				buffer = buffers[queuedBufferCount];
			}
			else
			{
				// No available buffers, skip this frame
				return;
			}

			// Validate buffer parameters
			if (pcmBuffer.empty())
			{
				// Re-add buffer to available list if we can't use it
				if (queuedBufferCount >= NUM_BUFFERS)
				{
					availableBuffers.push(buffer);
				}
				return;
			}

			// Determine format based on channels
			ALenum format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

			// Upload audio data to OpenAL buffer
			alBufferData(buffer, format, pcmBuffer.data(),
			             static_cast<ALsizei>(samplesProcessed * sizeof(int16_t)), static_cast<ALsizei>(sampleRate));
			CheckOpenALError("alBufferData");

			// Queue the buffer
			alSourceQueueBuffers(source, 1, &buffer);
			CheckOpenALError("alSourceQueueBuffers");

			// Track that we've queued this buffer
			if (queuedBufferCount < NUM_BUFFERS)
			{
				queuedBufferCount++;
			}

			// Start playing if not already playing
			ALint sourceState;
			alGetSourcei(source, AL_SOURCE_STATE, &sourceState);
			CheckOpenALError("alGetSourcei AL_SOURCE_STATE");

			if (sourceState != AL_PLAYING)
			{
				alSourcePlay(source);
				CheckOpenALError("alSourcePlay");
			}

			playbackPosition += samplesProcessed / channels;
		}
	}
};

AudioSystem::~AudioSystem()
{
	// Stop the audio thread first
	stopAudioThread();

	// Stop and clean up audio output device
	if (outputDevice)
	{
		outputDevice->Stop();
		outputDevice.reset();
	}

	// Destructor implementation
	sources.clear();
	audioData.clear();

	// Clean up HRTF buffers
	cleanupHRTFBuffers();
}

void AudioSystem::GenerateSineWavePing(float *buffer, uint32_t sampleCount, uint32_t playbackPosition)
{
	constexpr float    sampleRate        = 44100.0f;
	const float        frequency         = 800.0f;        // 800Hz ping
	constexpr float    pingDuration      = 0.75f;         // 0.75 second ping duration
	constexpr auto     pingSamples       = static_cast<uint32_t>(pingDuration * sampleRate);
	constexpr float    silenceDuration   = 1.0f;        // 1 second silence after ping
	constexpr auto     silenceSamples    = static_cast<uint32_t>(silenceDuration * sampleRate);
	constexpr uint32_t totalCycleSamples = pingSamples + silenceSamples;

	const uint32_t  attackSamples  = static_cast<uint32_t>(0.001f * sampleRate);        // ~1ms attack
	const uint32_t  releaseSamples = static_cast<uint32_t>(0.001f * sampleRate);        // ~1ms release
	constexpr float amplitude      = 0.6f;

	for (uint32_t i = 0; i < sampleCount; i++)
	{
		uint32_t globalPosition = playbackPosition + i;
		uint32_t cyclePosition  = globalPosition % totalCycleSamples;

		if (cyclePosition < pingSamples)
		{
			float t = static_cast<float>(cyclePosition) / sampleRate;

			// Minimal envelope for click prevention only
			float envelope = 1.0f;
			if (cyclePosition < attackSamples)
			{
				envelope = static_cast<float>(cyclePosition) / static_cast<float>(std::max(1u, attackSamples));
			}
			else if (cyclePosition > pingSamples - releaseSamples)
			{
				uint32_t relPos = pingSamples - cyclePosition;
				envelope        = static_cast<float>(relPos) / static_cast<float>(std::max(1u, releaseSamples));
			}

			float sineWave = sinf(2.0f * static_cast<float>(M_PI) * frequency * t);
			buffer[i]      = amplitude * envelope * sineWave;
		}
		else
		{
			// Silence phase
			buffer[i] = 0.0f;
		}
	}
}

bool AudioSystem::Initialize(Engine *engine, Renderer *renderer)
{
	// Store the engine reference for accessing active camera
	this->engine = engine;

	if (renderer)
	{
		// Validate renderer if provided
		if (!renderer->IsInitialized())
		{
			std::cerr << "AudioSystem::Initialize: Renderer is not initialized" << std::endl;
			return false;
		}

		// Store the renderer for compute shader support
		this->renderer = renderer;
	}
	else
	{
		this->renderer = nullptr;
	}

	// Generate default HRTF data for spatial audio processing
	LoadHRTFData("");        // Pass empty filename to force generation of default HRTF data

	// Enable HRTF processing by default for 3D spatial audio
	EnableHRTF(true);

	// Set default listener properties
	SetListenerPosition(0.0f, 0.0f, 0.0f);
	SetListenerOrientation(0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);
	SetListenerVelocity(0.0f, 0.0f, 0.0f);
	SetMasterVolume(1.0f);

	// Initialize audio output device
	outputDevice = std::make_unique<OpenALAudioOutputDevice>();
	if (!outputDevice->Initialize(44100, 2, 1024))
	{
		std::cerr << "Failed to initialize audio output device" << std::endl;
		return false;
	}

	// Start audio output
	if (!outputDevice->Start())
	{
		std::cerr << "Failed to start audio output device" << std::endl;
		return false;
	}

	// Start the background audio processing thread
	startAudioThread();

	initialized = true;
	return true;
}

void AudioSystem::Update(std::chrono::milliseconds deltaTime)
{
	if (!initialized)
	{
		return;
	}

	// Synchronize HRTF listener position and orientation with active camera
	if (engine)
	{
		const CameraComponent *activeCamera = engine->GetActiveCamera();
		if (activeCamera)
		{
			// Get camera position
			glm::vec3 cameraPos = activeCamera->GetPosition();
			SetListenerPosition(cameraPos.x, cameraPos.y, cameraPos.z);

			// Calculate camera forward and up vectors for orientation
			// The camera looks at its target, so forward = normalize(target - position)
			glm::vec3 target  = activeCamera->GetTarget();
			glm::vec3 up      = activeCamera->GetUp();
			glm::vec3 forward = glm::normalize(target - cameraPos);

			SetListenerOrientation(forward.x, forward.y, forward.z, up.x, up.y, up.z);
		}
	}

	// Update audio sources and process spatial audio
	for (auto &source : sources)
	{
		if (!source->IsPlaying())
		{
			continue;
		}

		// Cast to ConcreteAudioSource to access timing methods
		auto *concreteSource = dynamic_cast<ConcreteAudioSource *>(source.get());

		// Update playback timing and delay logic
		concreteSource->UpdatePlayback(deltaTime, 0);

		// Only process audio if not in the delay phase
		if (!concreteSource->ShouldProcessAudio())
		{
			continue;
		}

		// Process audio with HRTF spatial processing (works with or without renderer)
		if (hrtfEnabled && !hrtfData.empty())
		{
			// Get source position for spatial processing
			const float *sourcePosition = concreteSource->GetPosition();

			// Accumulate samples based on real time and process in fixed-size chunks to avoid tiny buffers
			double acc = concreteSource->GetSampleAccumulator();
			acc += (static_cast<double>(deltaTime.count()) * 44100.0) / 1000.0;        // ms -> samples
			constexpr uint32_t kChunk    = 33075;
			uint32_t           available = static_cast<uint32_t>(acc);
			if (available < kChunk)
			{
				// Not enough for a full chunk; keep accumulating
				concreteSource->SetSampleAccumulator(acc);
				continue;
			}
			// Process as many full chunks as available this frame
			while (available >= kChunk)
			{
				std::vector<float> inputBuffer(kChunk, 0.0f);
				std::vector<float> outputBuffer(kChunk * 2, 0.0f);
				uint32_t           actualSamplesProcessed = 0;

				// Generate audio signal from loaded audio data or debug ping
				auto audioIt = audioData.find(concreteSource->GetName());
				if (audioIt != audioData.end() && !audioIt->second.empty())
				{
					// Use actual loaded audio data with proper position tracking
					const auto &data        = audioIt->second;
					uint32_t    playbackPos = concreteSource->GetPlaybackPosition();

					for (uint32_t i = 0; i < kChunk; i++)
					{
						uint32_t dataIndex = (playbackPos + i) * 4;        // 4 bytes per sample (16-bit stereo)

						if (dataIndex + 1 < data.size())
						{
							// Convert from 16-bit PCM to float
							int16_t sample = *reinterpret_cast<const int16_t *>(&data[dataIndex]);
							inputBuffer[i] = static_cast<float>(sample) / 32768.0f;
							actualSamplesProcessed++;
						}
						else
						{
							// Reached end of audio data
							inputBuffer[i] = 0.0f;
						}
					}
				}
				else
				{
					// Generate sine wave ping for debugging
					GenerateSineWavePing(inputBuffer.data(), kChunk, concreteSource->GetPlaybackPosition());
					actualSamplesProcessed = kChunk;
				}

				// Build extended input [history | current] to preserve convolution continuity across chunks
				uint32_t                                                             histLen = (hrtfSize > 0) ? (hrtfSize - 1) : 0;
				static std::unordered_map<ConcreteAudioSource *, std::vector<float>> hrtfHistories;
				auto                                                                &hist = hrtfHistories[concreteSource];
				if (hist.size() != histLen)
				{
					hist.assign(histLen, 0.0f);
				}
				std::vector<float> extendedInput(histLen + kChunk, 0.0f);
				if (histLen > 0)
				{
					std::memcpy(extendedInput.data(), hist.data(), histLen * sizeof(float));
				}
				std::memcpy(extendedInput.data() + histLen, inputBuffer.data(), kChunk * sizeof(float));

				// Submit for GPU HRTF processing via the background thread (trim will occur in processAudioTask)
				submitAudioTask(extendedInput.data(), static_cast<uint32_t>(extendedInput.size()), sourcePosition, actualSamplesProcessed, histLen);

				// Update history with the tail of current input
				if (histLen > 0)
				{
					std::memcpy(hist.data(), inputBuffer.data() + (kChunk - histLen), histLen * sizeof(float));
				}

				// Update playback timing with actual samples processed
				concreteSource->UpdatePlayback(std::chrono::milliseconds(0), actualSamplesProcessed);

				// Consume one chunk from the accumulator
				acc -= static_cast<double>(kChunk);
				available -= kChunk;
			}
			// Store fractional remainder for next frame
			concreteSource->SetSampleAccumulator(acc);
		}
	}

	// Apply master volume changes to all active sources
	for (auto &source : sources)
	{
		if (source->IsPlaying())
		{
			// Master volume is applied during HRTF processing and individual source volume control
			// Volume scaling is handled in the ProcessHRTF function
		}
	}

	// Clean up finished audio sources
	std::erase_if(sources,
	              [](const std::unique_ptr<AudioSource> &source) {
		              // Keep all sources active for continuous playback
		              // Audio sources can be stopped/started via their Play/Stop methods
		              return false;
	              });

	// Update timing for audio processing with low-latency chunks
	static std::chrono::milliseconds accumulatedTime = std::chrono::milliseconds(0);
	accumulatedTime += deltaTime;

	// Process audio in 20ms chunks for optimal latency
	constexpr std::chrono::milliseconds audioChunkTime = std::chrono::milliseconds(20);        // 20ms chunks for real-time audio
	if (accumulatedTime >= audioChunkTime)
	{
		// Trigger audio buffer updates for smooth playback
		// The HRTF processing ensures spatial audio is updated continuously
		accumulatedTime = std::chrono::milliseconds(0);

		// Update listener properties if they have changed
		// This ensures spatial audio positioning stays current with camera movement
	}
}

bool AudioSystem::LoadAudio(const std::string &filename, const std::string &name)
{
	// Open the WAV file
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Failed to open audio file: " << filename << std::endl;
		return false;
	}

	// Read WAV header
	struct WAVHeader
	{
		char     riff[4];              // "RIFF"
		uint32_t fileSize;             // File size - 8
		char     wave[4];              // "WAVE"
		char     fmt[4];               // "fmt "
		uint32_t fmtSize;              // Format chunk size
		uint16_t audioFormat;          // Audio format (1 = PCM)
		uint16_t numChannels;          // Number of channels
		uint32_t sampleRate;           // Sample rate
		uint32_t byteRate;             // Byte rate
		uint16_t blockAlign;           // Block align
		uint16_t bitsPerSample;        // Bits per sample
		char     data[4];              // "data"
		uint32_t dataSize;             // Data size
	};

	WAVHeader header{};
	file.read(reinterpret_cast<char *>(&header), sizeof(WAVHeader));

	// Validate WAV header
	if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
	    std::strncmp(header.wave, "WAVE", 4) != 0 ||
	    std::strncmp(header.fmt, "fmt ", 4) != 0 ||
	    std::strncmp(header.data, "data", 4) != 0)
	{
		std::cerr << "Invalid WAV file format: " << filename << std::endl;
		file.close();
		return false;
	}

	// Only support PCM format for now
	if (header.audioFormat != 1)
	{
		std::cerr << "Unsupported audio format (only PCM supported): " << filename << std::endl;
		file.close();
		return false;
	}

	// Read audio data
	std::vector<uint8_t> data(header.dataSize);
	file.read(reinterpret_cast<char *>(data.data()), header.dataSize);
	file.close();

	if (file.gcount() != static_cast<std::streamsize>(header.dataSize))
	{
		std::cerr << "Failed to read complete audio data from: " << filename << std::endl;
		return false;
	}

	// Store the audio data
	audioData[name] = std::move(data);

	return true;
}

AudioSource *AudioSystem::CreateAudioSource(const std::string &name)
{
	// Check if the audio data exists
	auto it = audioData.find(name);
	if (it == audioData.end())
	{
		std::cerr << "AudioSystem::CreateAudioSource: Audio data not found: " << name << std::endl;
		return nullptr;
	}

	// Create a new audio source
	auto source = std::make_unique<ConcreteAudioSource>(name);

	// Calculate audio length in samples for timing
	const auto &data = it->second;
	if (!data.empty())
	{
		// Assuming 16-bit stereo audio at 44.1kHz (standard WAV format)
		// The audio data reading uses dataIndex = (playbackPos + i) * 4
		// So we need to calculate length based on how many individual samples we can read
		// Each 4 bytes represents one stereo sample pair, so total individual samples = data.size() / 4
		uint32_t totalSamples = static_cast<uint32_t>(data.size()) / 4;

		// Set the audio length for proper timing
		source->SetAudioLength(totalSamples);
	}

	// Store the source
	sources.push_back(std::move(source));

	return sources.back().get();
}

AudioSource *AudioSystem::CreateDebugPingSource(const std::string &name)
{
	// Create a new audio source for debugging
	auto source = std::make_unique<ConcreteAudioSource>(name);

	// Set up debug ping parameters
	// The ping will cycle every 1.5 seconds (0.5s ping + 1.0s silence)
	constexpr float sampleRate        = 44100.0f;
	constexpr float pingDuration      = 0.5f;
	constexpr float silenceDuration   = 1.0f;
	constexpr auto  totalCycleSamples = static_cast<uint32_t>((pingDuration + silenceDuration) * sampleRate);

	// For generated ping, let the generator control the 0.5s ping + 1.0s silence cycle.
	// Disable source-level length/delay to avoid double-silence and audible resets.
	source->SetAudioLength(0);

	// Store the source
	sources.push_back(std::move(source));

	return sources.back().get();
}

void AudioSystem::SetListenerPosition(const float x, const float y, const float z)
{
	listenerPosition[0] = x;
	listenerPosition[1] = y;
	listenerPosition[2] = z;
}

void AudioSystem::SetListenerOrientation(const float forwardX, const float forwardY, const float forwardZ,
                                         const float upX, const float upY, const float upZ)
{
	listenerOrientation[0] = forwardX;
	listenerOrientation[1] = forwardY;
	listenerOrientation[2] = forwardZ;
	listenerOrientation[3] = upX;
	listenerOrientation[4] = upY;
	listenerOrientation[5] = upZ;
}

void AudioSystem::SetListenerVelocity(const float x, const float y, const float z)
{
	listenerVelocity[0] = x;
	listenerVelocity[1] = y;
	listenerVelocity[2] = z;
}

void AudioSystem::SetMasterVolume(const float volume)
{
	masterVolume = volume;
}

void AudioSystem::EnableHRTF(const bool enable)
{
	hrtfEnabled = enable;
}

bool AudioSystem::IsHRTFEnabled() const
{
	return hrtfEnabled;
}

void AudioSystem::SetHRTFCPUOnly(const bool cpuOnly)
{
	(void) cpuOnly;
	// Enforce GPU-only HRTF processing: ignore CPU-only requests
	hrtfCPUOnly = false;
}

bool AudioSystem::IsHRTFCPUOnly() const
{
	return hrtfCPUOnly;
}

bool AudioSystem::LoadHRTFData(const std::string &filename)
{
	// HRTF parameters
	constexpr uint32_t hrtfSampleCount = 256;             // Number of samples per impulse response
	constexpr uint32_t positionCount   = 36 * 13;         // 36 azimuths (10-degree steps) * 13 elevations (15-degree steps)
	constexpr uint32_t channelCount    = 2;               // Stereo (left and right ears)
	const float        sampleRate      = 44100.0f;        // Sample rate for HRTF data
	const float        speedOfSound    = 343.0f;          // Speed of sound in m/s
	const float        headRadius      = 0.0875f;         // Average head radius in meters

	// Try to load from a file first (only if the filename is provided)
	if (!filename.empty())
	{
		if (std::ifstream file(filename, std::ios::binary); file.is_open())
		{
			// Read the file header to determine a format
			char header[4];
			file.read(header, 4);

			if (std::strncmp(header, "HRTF", 4) == 0)
			{
				// Custom HRTF format
				uint32_t fileHrtfSize, filePositionCount, fileChannelCount;
				file.read(reinterpret_cast<char *>(&fileHrtfSize), sizeof(uint32_t));
				file.read(reinterpret_cast<char *>(&filePositionCount), sizeof(uint32_t));
				file.read(reinterpret_cast<char *>(&fileChannelCount), sizeof(uint32_t));

				if (fileChannelCount == channelCount)
				{
					hrtfData.resize(fileHrtfSize * filePositionCount * fileChannelCount);
					file.read(reinterpret_cast<char *>(hrtfData.data()), static_cast<std::streamsize>(hrtfData.size() * sizeof(float)));

					hrtfSize         = fileHrtfSize;
					numHrtfPositions = filePositionCount;

					file.close();
					return true;
				}
			}
			file.close();
		}
	}

	// Generate realistic HRTF data based on acoustic modeling
	// Resize the HRTF data vector
	hrtfData.resize(hrtfSampleCount * positionCount * channelCount);

	// Generate HRTF impulse responses for each position
	for (uint32_t pos = 0; pos < positionCount; pos++)
	{
		// Calculate azimuth and elevation for this position
		uint32_t azimuthIndex   = pos % 36;
		uint32_t elevationIndex = pos / 36;

		float azimuth   = (static_cast<float>(azimuthIndex) * 10.0f - 180.0f) * static_cast<float>(M_PI) / 180.0f;
		float elevation = (static_cast<float>(elevationIndex) * 15.0f - 90.0f) * static_cast<float>(M_PI) / 180.0f;

		// Convert to Cartesian coordinates
		float x = std::cos(elevation) * std::sin(azimuth);
		float y = std::sin(elevation);
		float z = std::cos(elevation) * std::cos(azimuth);

		for (uint32_t channel = 0; channel < channelCount; channel++)
		{
			// Calculate ear position (left ear: -0.1m, right ear: +0.1m on x-axis)
			float earX = (channel == 0) ? -0.1f : 0.1f;

			// Calculate distance from source to ear
			float dx       = x - earX;
			float dy       = y;
			float dz       = z;
			float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

			// Calculate time delay (ITD - Interaural Time Difference)
			float timeDelay   = distance / speedOfSound;
			auto  sampleDelay = static_cast<uint32_t>(timeDelay * sampleRate);

			// Calculate head shadow effect (ILD - Interaural Level Difference)
			float shadowFactor = 1.0f;
			if (channel == 0 && azimuth > 0)
			{        // Left ear, source on right
				shadowFactor = 0.3f + 0.7f * std::exp(-azimuth * 2.0f);
			}
			else if (channel == 1 && azimuth < 0)
			{        // Right ear, source on left
				shadowFactor = 0.3f + 0.7f * std::exp(azimuth * 2.0f);
			}

			// Generate impulse response
			uint32_t samplesGenerated = 0;
			for (uint32_t i = 0; i < hrtfSampleCount; i++)
			{
				float value = 0.0f;

				// Direct path impulse
				if (i >= sampleDelay && i < sampleDelay + 10)
				{
					float t = static_cast<float>(i - sampleDelay) / sampleRate;
					value   = shadowFactor * std::exp(-t * 1000.0f) * std::cos(2.0f * static_cast<float>(M_PI) * 1000.0f * t);
				}

				// Apply distance attenuation
				value /= std::max(1.0f, distance);

				uint32_t index  = pos * hrtfSampleCount * channelCount + channel * hrtfSampleCount + i;
				hrtfData[index] = value;
			}
		}
	}

	// Store HRTF parameters
	hrtfSize         = hrtfSampleCount;
	numHrtfPositions = positionCount;

	return true;
}

bool AudioSystem::ProcessHRTF(const float *inputBuffer, float *outputBuffer, uint32_t sampleCount, const float *sourcePosition)
{
	if (!hrtfEnabled)
	{
		// If HRTF is disabled, just copy input to output
		for (uint32_t i = 0; i < sampleCount; i++)
		{
			outputBuffer[i * 2]     = inputBuffer[i];        // Left channel
			outputBuffer[i * 2 + 1] = inputBuffer[i];        // Right channel
		}
		return true;
	}

	// Check if we should use CPU-only processing or if Vulkan is not available
	// Also force CPU processing if we've detected threading issues previously
	static bool forceGPUFallback = false;
	if (hrtfCPUOnly || !renderer || !renderer->IsInitialized() || forceGPUFallback)
	{
		// Use CPU-based HRTF processing (either forced or fallback)

		// Create buffers for HRTF processing if they don't exist or if the sample count has changed
		if (!createHRTFBuffers(sampleCount))
		{
			std::cerr << "Failed to create HRTF buffers" << std::endl;
			return false;
		}

		// Copy input data to input buffer
		void *data = inputBufferMemory.mapMemory(0, sampleCount * sizeof(float));
		memcpy(data, inputBuffer, sampleCount * sizeof(float));
		inputBufferMemory.unmapMemory();

		// Copy source and listener positions
		memcpy(params.sourcePosition, sourcePosition, sizeof(float) * 3);
		memcpy(params.listenerPosition, listenerPosition, sizeof(float) * 3);
		memcpy(params.listenerOrientation, listenerOrientation, sizeof(float) * 6);
		params.sampleCount      = sampleCount;
		params.hrtfSize         = hrtfSize;
		params.numHrtfPositions = numHrtfPositions;
		params.padding          = 0.0f;

		// Copy parameters to parameter buffer using persistent memory mapping
		if (persistentParamsMemory)
		{
			memcpy(persistentParamsMemory, &params, sizeof(HRTFParams));
		}
		else
		{
			std::cerr << "WARNING: Persistent memory not available, falling back to map/unmap" << std::endl;
			data = paramsBufferMemory.mapMemory(0, sizeof(HRTFParams));
			memcpy(data, &params, sizeof(HRTFParams));
			paramsBufferMemory.unmapMemory();
		}

		// Perform HRTF processing using CPU-based convolution
		// This implementation provides real-time 3D audio spatialization

		// Calculate direction from listener to source
		float direction[3];
		direction[0] = sourcePosition[0] - listenerPosition[0];
		direction[1] = sourcePosition[1] - listenerPosition[1];
		direction[2] = sourcePosition[2] - listenerPosition[2];

		// Normalize direction
		float length = std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
		if (length > 0.0001f)
		{
			direction[0] /= length;
			direction[1] /= length;
			direction[2] /= length;
		}
		else
		{
			direction[0] = 0.0f;
			direction[1] = 0.0f;
			direction[2] = -1.0f;        // Default to front
		}

		// Calculate azimuth and elevation
		float azimuth   = std::atan2(direction[0], direction[2]);
		float elevation = std::asin(std::max(-1.0f, std::min(1.0f, direction[1])));

		// Convert to indices
		int azimuthIndex   = static_cast<int>((azimuth + M_PI) / (2.0f * M_PI) * 36.0f) % 36;
		int elevationIndex = static_cast<int>((elevation + M_PI / 2.0f) / M_PI * 13.0f);
		elevationIndex     = std::max(0, std::min(12, elevationIndex));

		// Get HRTF index
		int hrtfIndex = elevationIndex * 36 + azimuthIndex;
		hrtfIndex     = std::min(hrtfIndex, static_cast<int>(numHrtfPositions) - 1);

		// Perform convolution for left and right ears with simple overlap-add using per-direction input history
		static std::unordered_map<int, std::vector<float>> convHistories;        // mono histories keyed by hrtfIndex
		const uint32_t                                     histLenDesired = (hrtfSize > 0) ? (hrtfSize - 1) : 0;
		auto                                              &convHistory    = convHistories[hrtfIndex];
		if (convHistory.size() != histLenDesired)
		{
			convHistory.assign(histLenDesired, 0.0f);
		}

		// Build extended input: [history | current input]
		std::vector<float> extInput(histLenDesired + sampleCount, 0.0f);
		if (histLenDesired > 0)
		{
			std::memcpy(extInput.data(), convHistory.data(), histLenDesired * sizeof(float));
		}
		if (sampleCount > 0)
		{
			std::memcpy(extInput.data() + histLenDesired, inputBuffer, sampleCount * sizeof(float));
		}

		for (uint32_t i = 0; i < sampleCount; i++)
		{
			float leftSample  = 0.0f;
			float rightSample = 0.0f;

			// Convolve with HRTF impulse response using extended input
			// extIndex = histLenDesired + i - j; ensure extIndex >= 0
			uint32_t jMax = std::min<uint32_t>(hrtfSize - 1, histLenDesired + i);
			for (uint32_t j = 0; j <= jMax; j++)
			{
				uint32_t extIndex       = histLenDesired + i - j;
				uint32_t hrtfLeftIndex  = hrtfIndex * hrtfSize * 2 + j;
				uint32_t hrtfRightIndex = hrtfIndex * hrtfSize * 2 + hrtfSize + j;

				if (hrtfLeftIndex < hrtfData.size() && hrtfRightIndex < hrtfData.size())
				{
					float in = extInput[extIndex];
					leftSample += in * hrtfData[hrtfLeftIndex];
					rightSample += in * hrtfData[hrtfRightIndex];
				}
			}

			// Apply distance attenuation
			float distanceAttenuation = 1.0f / std::max(1.0f, length);
			leftSample *= distanceAttenuation;
			rightSample *= distanceAttenuation;

			// Write to output buffer
			outputBuffer[i * 2]     = leftSample;
			outputBuffer[i * 2 + 1] = rightSample;
		}

		// Update history with the tail of the extended input
		if (histLenDesired > 0)
		{
			std::memcpy(convHistory.data(), extInput.data() + sampleCount, histLenDesired * sizeof(float));
		}

		return true;
	}
	else
	{
		// Use Vulkan shader-based HRTF processing with fallback to CPU
		try
		{
			// Validate HRTF data exists
			if (hrtfData.empty())
			{
				LoadHRTFData("");        // Generate HRTF data
			}

			// Create buffers for HRTF processing if they don't exist or if the sample count has changed
			if (!createHRTFBuffers(sampleCount))
			{
				std::cerr << "Failed to create HRTF buffers, falling back to CPU processing" << std::endl;
				throw std::runtime_error("Buffer creation failed");
			}

			// Copy input data to input buffer
			void *data = inputBufferMemory.mapMemory(0, sampleCount * sizeof(float));
			memcpy(data, inputBuffer, sampleCount * sizeof(float));

			inputBufferMemory.unmapMemory();

			// Set up HRTF parameters with proper std140 uniform buffer layout
			struct alignas(16) HRTFParams
			{
				float    listenerPosition[4];        // vec3 + padding (16 bytes) - offset 0
				float    listenerForward[4];         // vec3 + padding (16 bytes) - offset 16
				float    listenerUp[4];              // vec3 + padding (16 bytes) - offset 32
				float    sourcePosition[4];          // vec3 + padding (16 bytes) - offset 48
				float    sampleCount;                // float (4 bytes) - offset 64
				float    padding1[3];                // Padding to align to 16-byte boundary - offset 68
				uint32_t inputChannels;              // uint (4 bytes) - offset 80
				uint32_t outputChannels;             // uint (4 bytes) - offset 84
				uint32_t hrtfSize;                   // uint (4 bytes) - offset 88
				uint32_t numHrtfPositions;           // uint (4 bytes) - offset 92
				float    distanceAttenuation;        // float (4 bytes) - offset 96
				float    dopplerFactor;              // float (4 bytes) - offset 100
				float    reverbMix;                  // float (4 bytes) - offset 104
				float    padding2;                   // Padding to complete 16-byte alignment - offset 108
			} params{};

			// Copy listener and source positions with proper padding for GPU alignment
			memcpy(params.listenerPosition, listenerPosition, sizeof(float) * 3);
			params.listenerPosition[3] = 0.0f;                                                 // Padding for float3 alignment
			memcpy(params.listenerForward, &listenerOrientation[0], sizeof(float) * 3);        // Forward vector
			params.listenerForward[3] = 0.0f;                                                  // Padding for float3 alignment
			memcpy(params.listenerUp, &listenerOrientation[3], sizeof(float) * 3);             // Up vector
			params.listenerUp[3] = 0.0f;                                                       // Padding for float3 alignment
			memcpy(params.sourcePosition, sourcePosition, sizeof(float) * 3);
			params.sourcePosition[3] = 0.0f;                                            // Padding for float3 alignment
			params.sampleCount       = static_cast<float>(sampleCount);                 // Number of samples to process
			params.padding1[0] = params.padding1[1] = params.padding1[2] = 0.0f;        // Initialize padding
			params.inputChannels                                         = 1;           // Mono input
			params.outputChannels                                        = 2;           // Stereo output
			params.hrtfSize                                              = hrtfSize;
			params.numHrtfPositions                                      = numHrtfPositions;
			params.distanceAttenuation                                   = 1.0f;
			params.dopplerFactor                                         = 1.0f;
			params.reverbMix                                             = 0.0f;
			params.padding2                                              = 0.0f;        // Initialize padding

			// Copy parameters to parameter buffer using persistent memory mapping
			if (persistentParamsMemory)
			{
				memcpy(persistentParamsMemory, &params, sizeof(HRTFParams));
			}
			else
			{
				std::cerr << "ERROR: Persistent memory not available for GPU processing!" << std::endl;
				throw std::runtime_error("Persistent memory required for GPU processing");
			}

			// Use renderer's main compute pipeline instead of dedicated HRTF pipeline
			uint32_t workGroupSize = 64;        // Must match the numthreads in the shader
			uint32_t groupCountX   = (sampleCount + workGroupSize - 1) / workGroupSize;

			// Use renderer's main compute pipeline dispatch method
			auto computeFence = renderer->DispatchCompute(groupCountX, 1, 1,
			                                              *this->inputBuffer, *this->outputBuffer,
			                                              *this->hrtfBuffer, *this->paramsBuffer);

			// Wait for compute shader to complete using fence-based synchronization
			const vk::raii::Device &device = renderer->GetRaiiDevice();
			vk::Result              result = device.waitForFences(*computeFence, VK_TRUE, UINT64_MAX);
			if (result != vk::Result::eSuccess)
			{
				std::cerr << "Failed to wait for compute fence: " << vk::to_string(result) << std::endl;
				throw std::runtime_error("Fence wait failed");
			}

			// Copy results from output buffer to the output array
			void *outputData = outputBufferMemory.mapMemory(0, sampleCount * 2 * sizeof(float));

			memcpy(outputBuffer, outputData, sampleCount * 2 * sizeof(float));
			outputBufferMemory.unmapMemory();

			return true;
		}
		catch (const std::exception &e)
		{
			std::cerr << "GPU HRTF processing failed: " << e.what() << std::endl;
			std::cerr << "CPU fallback disabled - GPU path required" << std::endl;
			throw;        // Re-throw the exception to ensure failure without CPU fallback
		}
	}
}

bool AudioSystem::createHRTFBuffers(uint32_t sampleCount)
{
	// Smart buffer reuse: only recreate if sample count changed significantly or buffers don't exist
	if (currentSampleCount == sampleCount && *inputBuffer && *outputBuffer && *hrtfBuffer && *paramsBuffer)
	{
		return true;
	}

	// Ensure all GPU operations complete before cleaning up existing buffers.
	// External synchronization required (VVL): use renderer helper which serializes against queue usage.
	if (renderer)
	{
		renderer->WaitIdle();
	}

	// Clean up existing buffers only if we need to recreate them
	cleanupHRTFBuffers();

	if (!renderer)
	{
		std::cerr << "AudioSystem::createHRTFBuffers: Renderer is null" << std::endl;
		return false;
	}

	const vk::raii::Device &device = renderer->GetRaiiDevice();
	try
	{
		// Create input buffer (mono audio)
		vk::BufferCreateInfo inputBufferInfo;
		inputBufferInfo.size        = sampleCount * sizeof(float);
		inputBufferInfo.usage       = vk::BufferUsageFlagBits::eStorageBuffer;
		inputBufferInfo.sharingMode = vk::SharingMode::eExclusive;

		inputBuffer = vk::raii::Buffer(device, inputBufferInfo);

		vk::MemoryRequirements inputMemRequirements = inputBuffer.getMemoryRequirements();

		vk::MemoryAllocateInfo inputAllocInfo;
		inputAllocInfo.allocationSize  = inputMemRequirements.size;
		inputAllocInfo.memoryTypeIndex = renderer->FindMemoryType(
		    inputMemRequirements.memoryTypeBits,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		inputBufferMemory = vk::raii::DeviceMemory(device, inputAllocInfo);
		inputBuffer.bindMemory(*inputBufferMemory, 0);

		// Create output buffer (stereo audio)
		vk::BufferCreateInfo outputBufferInfo;
		outputBufferInfo.size        = sampleCount * 2 * sizeof(float);        // Stereo (2 channels)
		outputBufferInfo.usage       = vk::BufferUsageFlagBits::eStorageBuffer;
		outputBufferInfo.sharingMode = vk::SharingMode::eExclusive;

		outputBuffer = vk::raii::Buffer(device, outputBufferInfo);

		vk::MemoryRequirements outputMemRequirements = outputBuffer.getMemoryRequirements();

		vk::MemoryAllocateInfo outputAllocInfo;
		outputAllocInfo.allocationSize  = outputMemRequirements.size;
		outputAllocInfo.memoryTypeIndex = renderer->FindMemoryType(
		    outputMemRequirements.memoryTypeBits,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		outputBufferMemory = vk::raii::DeviceMemory(device, outputAllocInfo);
		outputBuffer.bindMemory(*outputBufferMemory, 0);

		// Create HRTF data buffer
		vk::BufferCreateInfo hrtfBufferInfo;
		hrtfBufferInfo.size        = hrtfData.size() * sizeof(float);
		hrtfBufferInfo.usage       = vk::BufferUsageFlagBits::eStorageBuffer;
		hrtfBufferInfo.sharingMode = vk::SharingMode::eExclusive;

		hrtfBuffer = vk::raii::Buffer(device, hrtfBufferInfo);

		vk::MemoryRequirements hrtfMemRequirements = hrtfBuffer.getMemoryRequirements();

		vk::MemoryAllocateInfo hrtfAllocInfo;
		hrtfAllocInfo.allocationSize  = hrtfMemRequirements.size;
		hrtfAllocInfo.memoryTypeIndex = renderer->FindMemoryType(
		    hrtfMemRequirements.memoryTypeBits,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		hrtfBufferMemory = vk::raii::DeviceMemory(device, hrtfAllocInfo);
		hrtfBuffer.bindMemory(*hrtfBufferMemory, 0);

		// Copy HRTF data to buffer
		void *hrtfMappedMemory = hrtfBufferMemory.mapMemory(0, hrtfData.size() * sizeof(float));
		memcpy(hrtfMappedMemory, hrtfData.data(), hrtfData.size() * sizeof(float));
		hrtfBufferMemory.unmapMemory();

		// Create parameters buffer - use the correct GPU structure size
		// The GPU processing uses a larger aligned structure (112 bytes) not the header struct (64 bytes)
		struct alignas(16) GPUHRTFParams
		{
			float    listenerPosition[4];        // vec3 + padding (16 bytes)
			float    listenerForward[4];         // vec3 + padding (16 bytes)
			float    listenerUp[4];              // vec3 + padding (16 bytes)
			float    sourcePosition[4];          // vec3 + padding (16 bytes)
			float    sampleCount;                // float (4 bytes)
			float    padding1[3];                // Padding to align to 16-byte boundary
			uint32_t inputChannels;              // uint (4 bytes)
			uint32_t outputChannels;             // uint (4 bytes)
			uint32_t hrtfSize;                   // uint (4 bytes)
			uint32_t numHrtfPositions;           // uint (4 bytes)
			float    distanceAttenuation;        // float (4 bytes)
			float    dopplerFactor;              // float (4 bytes)
			float    reverbMix;                  // float (4 bytes)
			float    padding2;                   // Padding to complete 16-byte alignment
		};

		vk::BufferCreateInfo paramsBufferInfo;
		paramsBufferInfo.size        = sizeof(GPUHRTFParams);        // Use correct GPU structure size (112 bytes)
		paramsBufferInfo.usage       = vk::BufferUsageFlagBits::eUniformBuffer;
		paramsBufferInfo.sharingMode = vk::SharingMode::eExclusive;

		paramsBuffer = vk::raii::Buffer(device, paramsBufferInfo);

		vk::MemoryRequirements paramsMemRequirements = paramsBuffer.getMemoryRequirements();

		vk::MemoryAllocateInfo paramsAllocInfo;
		paramsAllocInfo.allocationSize  = paramsMemRequirements.size;
		paramsAllocInfo.memoryTypeIndex = renderer->FindMemoryType(
		    paramsMemRequirements.memoryTypeBits,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		paramsBufferMemory = vk::raii::DeviceMemory(device, paramsAllocInfo);
		paramsBuffer.bindMemory(*paramsBufferMemory, 0);

		// Set up persistent memory mapping for parameters buffer to avoid repeated map/unmap operations
		persistentParamsMemory = paramsBufferMemory.mapMemory(0, sizeof(GPUHRTFParams));
		// Update current sample count to track buffer size
		currentSampleCount = sampleCount;
		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error creating HRTF buffers: " << e.what() << std::endl;
		cleanupHRTFBuffers();
		return false;
	}
}

void AudioSystem::cleanupHRTFBuffers()
{
	// Unmap persistent memory if it exists
	if (persistentParamsMemory && *paramsBufferMemory)
	{
		paramsBufferMemory.unmapMemory();
		persistentParamsMemory = nullptr;
	}

	// With RAII, we just need to set the resources to nullptr
	// The destructors will handle the cleanup
	inputBuffer        = nullptr;
	inputBufferMemory  = nullptr;
	outputBuffer       = nullptr;
	outputBufferMemory = nullptr;
	hrtfBuffer         = nullptr;
	hrtfBufferMemory   = nullptr;
	paramsBuffer       = nullptr;
	paramsBufferMemory = nullptr;

	// Reset sample count tracking
	currentSampleCount = 0;
}

// Threading implementation methods

void AudioSystem::startAudioThread()
{
	if (audioThreadRunning.load())
	{
		return;        // Thread already running
	}

	audioThreadShouldStop.store(false);
	audioThreadRunning.store(true);

	audioThread = std::thread(&AudioSystem::audioThreadLoop, this);
}

void AudioSystem::stopAudioThread()
{
	if (!audioThreadRunning.load())
	{
		return;        // Thread not running
	}

	// Signal the thread to stop
	audioThreadShouldStop.store(true);

	// Wake up the thread if it's waiting
	audioCondition.notify_all();

	// Wait for the thread to finish
	if (audioThread.joinable())
	{
		audioThread.join();
	}

	audioThreadRunning.store(false);
}

void AudioSystem::audioThreadLoop()
{
	while (!audioThreadShouldStop.load())
	{
		std::shared_ptr<AudioTask> task = nullptr;

		// Wait for a task or stop signal
		{
			std::unique_lock<std::mutex> lock(taskQueueMutex);
			audioCondition.wait(lock, [this] {
				return !audioTaskQueue.empty() || audioThreadShouldStop.load();
			});

			if (audioThreadShouldStop.load())
			{
				break;
			}

			if (!audioTaskQueue.empty())
			{
				task = audioTaskQueue.front();
				audioTaskQueue.pop();
			}
		}

		// Process the task if we have one
		if (task)
		{
			processAudioTask(task);
		}
	}
}

void AudioSystem::processAudioTask(const std::shared_ptr<AudioTask> &task)
{
	// Process HRTF in the background thread
	bool success = ProcessHRTF(task->inputBuffer.data(), task->outputBuffer.data(),
	                           task->sampleCount, task->sourcePosition);

	if (success && task->outputDevice && task->outputDevice->IsPlaying())
	{
		// We used extended input of length sampleCount = histLen + outFrames.
		// Trim the first trimFront frames from the stereo output and only write actualSamplesProcessed frames.
		uint32_t startFrame    = task->trimFront;
		uint32_t framesToWrite = task->actualSamplesProcessed;
		if (startFrame * 2 > task->outputBuffer.size())
		{
			startFrame = 0;        // safety
		}
		if (startFrame * 2 + framesToWrite * 2 > task->outputBuffer.size())
		{
			framesToWrite = static_cast<uint32_t>((task->outputBuffer.size() / 2) - startFrame);
		}
		float *startPtr = task->outputBuffer.data() + startFrame * 2;
		// Apply master volume only to the range we will write
		for (uint32_t i = 0; i < framesToWrite * 2; i++)
		{
			startPtr[i] *= task->masterVolume;
		}
		// Send processed audio directly to output device from background thread
		if (!task->outputDevice->WriteAudio(startPtr, framesToWrite))
		{
			std::cerr << "Failed to write audio data to output device from background thread" << std::endl;
		}
	}
}

bool AudioSystem::submitAudioTask(const float *inputBuffer, uint32_t sampleCount,
                                  const float *sourcePosition, uint32_t actualSamplesProcessed, uint32_t trimFront)
{
	if (!audioThreadRunning.load())
	{
		// Fallback to synchronous processing if the thread is not running
		std::vector<float> outputBuffer(sampleCount * 2);
		bool               success = ProcessHRTF(inputBuffer, outputBuffer.data(), sampleCount, sourcePosition);

		if (success && outputDevice && outputDevice->IsPlaying())
		{
			// Apply master volume
			for (uint32_t i = 0; i < sampleCount * 2; i++)
			{
				outputBuffer[i] *= masterVolume;
			}

			// Send to audio output device
			if (!outputDevice->WriteAudio(outputBuffer.data(), sampleCount))
			{
				std::cerr << "Failed to write audio data to output device" << std::endl;
				return false;
			}
		}
		return success;
	}

	// Create a new task for asynchronous processing
	auto task = std::make_shared<AudioTask>();
	task->inputBuffer.assign(inputBuffer, inputBuffer + sampleCount);
	task->outputBuffer.resize(sampleCount * 2);        // Stereo output
	memcpy(task->sourcePosition, sourcePosition, sizeof(float) * 3);
	task->sampleCount            = sampleCount;                                 // includes history frames
	task->actualSamplesProcessed = actualSamplesProcessed;                      // new frames only (kChunk)
	task->trimFront              = sampleCount - actualSamplesProcessed;        // history length (histLen)
	task->outputDevice           = outputDevice.get();
	task->masterVolume           = masterVolume;

	// Submit the task to the queue (non-blocking)
	{
		std::lock_guard<std::mutex> lock(taskQueueMutex);
		audioTaskQueue.push(task);
	}
	audioCondition.notify_one();

	return true;        // Return immediately without waiting
}

void AudioSystem::FlushOutput()
{
	// Stop background processing to avoid races while flushing
	stopAudioThread();

	// Clear any pending audio processing tasks
	{
		std::lock_guard<std::mutex>            lock(taskQueueMutex);
		std::queue<std::shared_ptr<AudioTask>> empty;
		std::swap(audioTaskQueue, empty);
	}

	// Flush the output device buffers and queues by restart
	if (outputDevice)
	{
		outputDevice->Stop();
		outputDevice->Start();
	}

	// Restart background processing
	startAudioThread();
}
