/*
 * Copyright (C) 2016 Google, Inc.
 * Copyright 2020 NVIDIA Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

#ifndef _PROGRAMSETTINGS_H_
#define _PROGRAMSETTINGS_H_

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <iomanip> 
#include <sstream>
#include "vulkan_interfaces.h"

struct ProgramConfig {

     struct ArgSpec {
       const char *flag;
       const char *short_flag;
       int numArgs;
       const char *help;
       std::function<bool(const char **, const std::vector<ArgSpec> &)> lambda;
     };

    ProgramConfig(const char* programName) {
        appName = programName;
        initialWidth = 1920;
        initialHeight = 1080;
        initialBitdepth = 8;
        videoWidth = 0;
        videoHeight = 0;
        queueCount = 1;
        numDecodeImagesInFlight = 8;
        numDecodeImagesToPreallocate = -1; // pre-allocate the maximum num of images
        numBitstreamBuffersToPreallocate = 8;
        backBufferCount = 3;
        ticksPerSecond = 30;
        vsync = true;

        verbose = false;
        validate = false;
        validateVerbose = false;

        noPresent = false;

        maxFrameCount = -1;
        videoFileName = "";
        loopCount = 1;
        queueId = 0;
        gpuIndex = -1;
        forceParserType = VK_VIDEO_CODEC_OPERATION_NONE_KHR;
        decoderQueueSize = 5;
        enablePostProcessFilter = -1,
        enableStreamDemuxing = true;
        deviceId = (uint32_t)-1;
        directMode = false;
        enableHwLoadBalancing = false;
        selectVideoWithComputeQueue = false;
        enableVideoEncoder = false;
        crcOutput = nullptr;
        outputy4m = false;
        outputcrcPerFrame = false;
        outputcrc = false;
        crcOutputFile = nullptr;
    }

    using ProgramArgs = std::vector<ArgSpec>;
    static bool showHelp(const char ** argv, const ProgramArgs &spec) { 
        std::cout << argv[0] << std::endl;
        for ( auto& flag : spec ) {
            std::stringstream ss;
            if (flag.flag) {
                ss << flag.flag << (flag.short_flag ? ", " : "");
            }
            if (flag.short_flag) {
                ss << flag.short_flag;
            }
            // Print flags column 30 chars wide, left justified
            std::cout << " " << std::left << std::setw(30) << ss.str();
            // Print help if available, left justified
            if (flag.help) {
                std::cout << flag.help;
            }
            std::cout << std::endl;
        }
        return true;
    };

    void ParseArgs(int argc, const char *argv[]) {
        ProgramArgs spec = {
            {"--help", nullptr, 0, "Show this help",
                [argv](const char **, const ProgramArgs &a) {
                    int rtn = showHelp(argv, a);
                    exit(EXIT_SUCCESS);
                    return rtn;
                }},
            {"--enableStrDemux", nullptr, 0, "Enable stream demuxing",
                [this](const char **, const ProgramArgs &a) {
                    enableStreamDemuxing = true;
                    return true;
                }},
            {"--disableStrDemux", nullptr, 0, "Disable stream demuxing",
                [this](const char **, const ProgramArgs &a) {
                    enableStreamDemuxing = false;
                    return true;
                }},
            {"--codec", nullptr, 1, "Codec to decode",
                [this](const char **args, const ProgramArgs &a) {
                    if ((strcmp(args[0], "hevc") == 0) ||
                        (strcmp(args[0], "h265") == 0)) {
                        forceParserType = VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR;
                        return true;
                    } else if ((strcmp(args[0], "avc") == 0) ||
                        (strcmp(args[0], "h264") == 0)) {
                        forceParserType = VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR;
                        return true;
                    } else {
                        std::cerr << "Invalid codec \"" << args[0] << "\"" << std::endl;
                        return false;
                    }
                }},
            {"--disableVsync", "-b", 0, "Disable vsync",
                [this](const char **args, const ProgramArgs &a) {
                    vsync = false;
                    return true;
                }},
            {"--initialWidth", "-w", 1, "Initial width of the video",
                [this](const char **args, const ProgramArgs &a) {
                    initialWidth = std::atoi(args[0]);
                    return true;
                }},
            {"--initialHeight", "-h", 1, "Initial height of the video",
                [this](const char **args, const ProgramArgs &a) {
                    initialHeight = std::atoi(args[0]);
                    return true;
                }},
            {"--validate", "-v", 0, "Validate input bitstream",
                [this](const char **args, const ProgramArgs &a) {
                    validate = true;
                    return true;
                }},
            {"--verboseValidate", "-vv", 0,
                "Validate input bitstream and be verbose",
                [this](const char **args, const ProgramArgs &a) {
                    validate = true;
                    verbose = true;
                    return true;
                }},
            {"--selectVideoWithComputeQueue", nullptr, 0,
                "Select a video queue that supports compute",
                [this](const char **args, const ProgramArgs &a) {
                    selectVideoWithComputeQueue = true;
                    return true;
                }},
            {"--noPresent", nullptr, 0,
                "Runs this program headless without presenting decode result to "
                "screen",
                [this](const char **args, const ProgramArgs &a) {
                    noPresent = true;
                    return true;
                }},
            {"--enableHwLoadBalancing", nullptr, 0,
                "Enable hardware load balancing by doing a round-robin through all "
                "available decode queues",
                [this](const char **args, const ProgramArgs &a) {
                    enableHwLoadBalancing = true;
                    return true;
                }},
            {"--input", "-i", 1, "Input filename to decode",
                [this](const char **args, const ProgramArgs &a) {
                    videoFileName = args[0];
                    std::ifstream validVideoFileStream(videoFileName, std::ifstream::in);
                    return (bool)validVideoFileStream;
                }},
            {"--output", "-o", 1, "Output filename to dump raw video to",
                [this](const char **args, const ProgramArgs &a) {
                    outputFileName = args[0];
                    return true;
                }},
            {"--gpu", "-gpu", 1, "Index to Vulkan physical device to use",
                [this](const char **args, const ProgramArgs &a) {
                    gpuIndex = std::atoi(args[0]);
                    return true;
                }},
            {"--queueSize", nullptr, 1,
                "Size of decode operation in-flight before synchronizing for the "
                "result - only used with --noDisplay (when no presentation is enabled)",
                [this](const char **args, const ProgramArgs &a) {
                    decoderQueueSize = std::atoi(args[0]);
                    return true;
                }},
            {"--decodeImagesInFlight", nullptr, 1,
                "The number of decode images that are in-flight in addition to the DPB required",
                [this](const char **args, const ProgramArgs &a) {
                    numDecodeImagesInFlight = std::atoi(args[0]);
                    return true;
                }},
            {"--displayBackBufferSize", nullptr, 1,
                "Size of display back-buffers swapchain queue size",
                [this](const char **args, const ProgramArgs &a) {
                    backBufferCount = std::atoi(args[0]);
                    return true;
                }},
            {"--enablePostProcessFilter", nullptr, 1, "Enables post processing by running "
                "a compute shader on the decode output",
                [this](const char **args, const ProgramArgs &a) {
                    enablePostProcessFilter = std::atoi(args[0]);
                    return true;
                }},
            {"--loop", nullptr, 1,
                "Number of times the playback from input should be repeated",
                [this](const char **args, const ProgramArgs &a) {
                    loopCount = std::atoi(args[0]);
                    if (loopCount < 0) {
                        std::cerr << "Loop count must not be negative" << std::endl;
                        return false;
                    }
                    return true;
                }},
            {"--maxFrameCount", "-c", 1,
                "Limit number of frames to be processed",
                [this](const char **args, const ProgramArgs &a) {
                    maxFrameCount = std::atoi(args[0]);
                    return true;
                }},
            {"--queueid", nullptr, 1, "Index of the decoder queue to be used",
                [this](const char **args, const ProgramArgs &a) {
                    queueId = std::atoi(args[0]);
                    std::cout << queueId << std::endl;
                    if (queueId < 0) {
                        std::cerr << "queueid must not be negative" << std::endl;
                        return false;
                    }
                    return true;
                }},
            {"--deviceID", "-deviceID", 1, "Hex ID of the device to be used",
                [this](const char **args, const ProgramArgs &a) {
                    sscanf(args[0], "%x", &deviceId);
                    return true;
                }},
            {"--deviceUuid", "-deviceUuid", 1, "UUID HEX string of the device to be used",
                [this](const char **args, const ProgramArgs &a) {
                    size_t size = SetHexDeviceUUID(args[0]);
                    if (size != VK_UUID_SIZE) {
                        std::cerr << "Invalid deviceUuid format used: " << args[0]
                                  << " with size: " << strlen(args[0])
                                  << std::endl;
                        std::cerr << "deviceUuid must be represented by 16 hex (32 bytes) values."
                                  << std::endl;
                        return false;
                    }
                    return true;
                }},
            {"--direct", nullptr, 0, "Direct to display mode",
                [this](const char **args, const ProgramArgs &a) {
                    directMode = true;
                    return true;
                }},
            {"--y4m", nullptr, 0, "Output to a Y4M container for easier loading by tools",
                [this](const char **args, const ProgramArgs &a) {
                    outputy4m = true;
                    return true;
                }},
            {"--crc", nullptr, 0, "Output a CRC for the entire stream",
                [this](const char **args, const ProgramArgs &a) {
                    outputcrc = true;
                    return true;
                }},
            {"--crcperframe", nullptr, 0, "Output a CRC for each frame",
                [this](const char **args, const ProgramArgs &a) {
                    outputcrcPerFrame = true;
                    return true;
                }},
            {"--crcoutfile", nullptr, 1, "Output file to store the CRC output into.",
                    [this](const char **args, const ProgramArgs &a) {
                    crcOutputFile = fopen(args[0], "wt");
                    return true;
                }},
            {"--crcinit", nullptr, 1, "Initial value of the CRC separated by a comma, a set of CRCs can be specified with this commandline parameter",
                [this](const char **args, const ProgramArgs &a) {
                    // Find out the amount of CRCs that need to be calculated.
                    // Using this feature allows the CRC generator to create multiple CRCs for the same frame or stream where the seed of the CRCs is different.
                    // Usually this will just be 1 entry, but can be used if a 32bit integer CRC is causing hash collisions.
                    // Example
                    // --crcinit ffffffff,0,5,6 will produce 4 crc's per frame and 4 crc's for the stream. Where each CRC is different.
                    // --crcinit X,Y,Z,W -> CRC: X^,Y^,Z^,W^
                    // --crcinit X,Y -> CRC: X^,Y^
                    // --crcinit X -> CRC: X^
                    std::vector<uint32_t> crcInitValueTemp;
                    std::istringstream stream(args[0]);
                    std::string token;
                    while (std::getline(stream, token, ',')) {
                        char* endPtr = NULL;
                        uint32_t initValue = strtoul(token.c_str(), &endPtr, 16);
                        if ((endPtr == NULL) || (*endPtr != 0)) {
                            std::cerr << "Failed to parse the following initial CRC value:"
                                  << token << std::endl;
                            return false;
                        }

                        crcInitValueTemp.push_back(initValue);
                    }

                    crcInitValue = crcInitValueTemp;
                    return true;
                }},
        };

        for (int i = 1; i < argc; i++) {
            auto flag = std::find_if(spec.begin(), spec.end(), [&](ArgSpec &a) {
                return (a.flag != nullptr && strcmp(argv[i], a.flag) == 0) ||
                (a.short_flag != nullptr && strcmp(argv[i], a.short_flag) == 0);
            });
            if (flag == spec.end()) {
                std::cerr << "Unknown argument \"" << argv[i] << "\"" << std::endl;
                std::cout << std::endl;
                showHelp(argv, spec);
                exit(EXIT_FAILURE);
            }

            if (i + flag->numArgs >= argc) {
                std::cerr << "Missing arguments for \"" << argv[i] << "\"" << std::endl;
                exit(EXIT_FAILURE);
            }

            bool disableValueCheck = false;
            if (i + 1 < argc && strcmp(argv[i + 1], "--") == 0) {
                if (i + 1 + flag->numArgs >= argc) {
                    std::cerr << "Missing arguments for \"" << argv[i] << "\"" << std::endl;
                    exit(EXIT_FAILURE);
                }
                disableValueCheck = true; 
                i++;
            }

            // Only allow values not starting with `-` unless prefixed with `-- ` (e.g. -i -- --inputfile-starting-with-minus)
            // This allows us to give better error messages as we don't expect any values to start with `-`
            if (!disableValueCheck) {
                for (int j = 1; j <= flag->numArgs; j++) {
                    if (argv[i + j][0] == '-') {
                        std::cerr << "Invalid value \"" << argv[i + j] << "\" for \"" << argv[i] << "\" "
                            "(we don't allow values starting with `-` by default). You probably missed to "
                            "set a value for \"" << argv[i] << "\"." << std::endl;
                        std::cerr << "Use \"-- " << argv[i + j] << "\" if you meant to set \"" << argv[i + j]
                            << "\" for \"" << argv[i] << "\"." << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
            }

            if (!flag->lambda(argv + i + 1, spec)) {
                exit(EXIT_FAILURE);
            }

            i += flag->numArgs;
        }

        // Resolve the CRC request in case there is a --crcinit specified.
        if (((outputcrcPerFrame != 0) || (outputcrc != 0))) {
            if (crcInitValue.empty() != false) {
                if (outputFileName.empty() != false) {
                    std::cerr << "Please specify -o if you intend to use CRC calculation, CRC calculation requires HOST accessible memory."
                                "Host accessible linear images requires an extra copy at the moment."
                                << std::endl;

                    exit(EXIT_FAILURE);
                }

                crcInitValue.push_back(0);
            }

            if (crcOutputFile == nullptr) {
                crcOutputFile = stdout;
            }
        }
    }

    // Assuming we have the length as a parameter:
    size_t SetDeviceUUID(const uint8_t* pDeviceUuid, size_t length) {

        if ((pDeviceUuid == nullptr) || (length == 0)) {
            deviceUUID.clear();
        }

        deviceUUID.assign(pDeviceUuid, pDeviceUuid + length);
        return length;
    }

    // If deviceUuid is null-terminated (less common for binary data):
    size_t SetDeviceUUID(const uint8_t* pDeviceUuid) {
        size_t length = strlen(reinterpret_cast<const char*>(pDeviceUuid));
        return SetDeviceUUID(pDeviceUuid, length);
    }

    size_t SetHexDeviceUUID(const char* pDeviceUuid) {

        size_t deviceUuidLen = strnlen(pDeviceUuid, (VK_UUID_SIZE * 2));

        if (deviceUuidLen <  (VK_UUID_SIZE * 2)) {
            return 0;
        }

        deviceUUID.clear();
        for (size_t i = 0; i < VK_UUID_SIZE; ++i) {
            uint8_t hexByte = 0;
            sscanf(pDeviceUuid, "%2hhx", &hexByte);
            deviceUUID.push_back(hexByte);
            pDeviceUuid += 2;
        }

        return VK_UUID_SIZE;
    }

    const uint8_t* GetDeviceUUID() const {
        return deviceUUID.empty() ? nullptr : deviceUUID.data();
    }

    FILE* crcOutputFile;
    std::string appName;
    std::basic_string<uint8_t> deviceUUID;
    int initialWidth;
    int initialHeight;
    int initialBitdepth;
    int videoWidth;
    int videoHeight;
    int queueCount;
    int32_t numDecodeImagesInFlight;
    int32_t numDecodeImagesToPreallocate;
    int32_t numBitstreamBuffersToPreallocate;
    int backBufferCount;
    int ticksPerSecond;
    int maxFrameCount;

    std::string videoFileName;
    std::string outputFileName;
    int gpuIndex;
    int loopCount;
    int queueId;
    VkVideoCodecOperationFlagBitsKHR forceParserType;
    std::vector<uint32_t> crcInitValue;
    uint32_t *crcValues;
    uint32_t deviceId;
    uint32_t decoderQueueSize;
    int32_t enablePostProcessFilter;
    uint32_t *crcOutput;
    uint32_t enableStreamDemuxing : 1;
    uint32_t directMode : 1;
    uint32_t vsync : 1;
    uint32_t validate : 1;
    uint32_t validateVerbose : 1;
    uint32_t verbose : 1;
    uint32_t noPresent : 1;
    uint32_t enableHwLoadBalancing : 1;
    uint32_t selectVideoWithComputeQueue : 1;
    uint32_t enableVideoEncoder : 1;
    uint32_t outputy4m : 1;
    uint32_t outputcrc : 1;
    uint32_t outputcrcPerFrame : 1;
};

#endif /* _PROGRAMSETTINGS_H_ */
