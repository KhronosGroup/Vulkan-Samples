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
#include "renderer.h"
#include "entity.h"
#include "mesh_component.h"
#include "transform_component.h"
#include <iostream>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

// Helper function to get buffer device address
vk::DeviceAddress getBufferDeviceAddress(const vk::raii::Device& device, vk::Buffer buffer) {
    vk::BufferDeviceAddressInfo addressInfo{};
    addressInfo.buffer = buffer;
    return device.getBufferAddress(addressInfo);
}

/**
 * @brief Build acceleration structures for ray query rendering.
 * 
 * Builds BLAS for each unique mesh and a TLAS for the entire scene.
 * 
 * @param entities The entities to include in the acceleration structures.
 * @return True if successful, false otherwise.
 */
bool Renderer::buildAccelerationStructures(const std::vector<std::unique_ptr<Entity>>& entities) {
    if (!accelerationStructureEnabled || !rayQueryEnabled) {
        std::cout << "Acceleration structures not supported on this device\n";
        return false;
    }
    
    try {
        std::cout << "Building acceleration structures for " << entities.size() << " entities...\n";

        // PRECHECK: Determine how many renderable entities and unique meshes are READY right now.
        // If the counts would shrink compared to the last successful build (e.g., streaming not done),
        // skip rebuilding to avoid producing a TLAS that only contains a small subset (like animated fans).
        size_t readyRenderableCount = 0;
        size_t readyUniqueMeshCount = 0;
        {
            size_t skippedInactive = 0;
            size_t skippedNoMesh = 0;
            size_t skippedNoRes = 0;
            size_t skippedException = 0;

            std::map<MeshComponent*, uint32_t> meshToBLASProbe;
            for (const auto& entityPtr : entities) {
                Entity* entity = entityPtr.get();
                if (!entity || !entity->IsActive()) {
                    skippedInactive++;
                    continue;
                }
                auto meshComp = entity->GetComponent<MeshComponent>();
                if (!meshComp) {
                    skippedNoMesh++;
                    continue;
                }

                try {
                    auto meshIt = meshResources.find(meshComp);
                    if (meshIt == meshResources.end()) {
                        skippedNoRes++;
                        continue;
                    }
                } catch (...) {
                    skippedException++;
                    continue;
                }

                readyRenderableCount++;
                if (meshToBLASProbe.find(meshComp) == meshToBLASProbe.end()) {
                    meshToBLASProbe[meshComp] = static_cast<uint32_t>(meshToBLASProbe.size());
                }
            }
            readyUniqueMeshCount = meshToBLASProbe.size();

            // Keep this precheck quiet; any meaningful summary is printed in the main AS build block below.
            (void)skippedInactive;
            (void)skippedNoMesh;
            (void)skippedNoRes;
            (void)skippedException;
        }

        if (readyRenderableCount == 0 || readyUniqueMeshCount == 0) {
            std::cout << "AS build skipped: no ready meshes yet (renderables=" << readyRenderableCount
                      << ", uniqueMeshes=" << readyUniqueMeshCount << ")\n";
            return false;
        }
        
        // Move old AS structures to pending deletion queue
        // They will be deleted after MAX_FRAMES_IN_FLIGHT frames to ensure all GPU work finishes
        // This prevents "buffer destroyed while in use" errors without needing device.waitIdle()
        // which would invalidate entity descriptor sets
        if (!blasStructures.empty() || *tlasStructure.handle) {
            PendingASDelete pendingDelete;
            pendingDelete.blasStructures = std::move(blasStructures);
            pendingDelete.tlasStructure = std::move(tlasStructure);
            pendingDelete.framesSinceDestroy = 0;
            pendingASDeletions.push_back(std::move(pendingDelete));
        }
        
        // Clear the moved-from containers (they're now empty)
        blasStructures.clear();
        tlasStructure = AccelerationStructure{};
        
        // Map mesh components to BLAS indices
        std::map<MeshComponent*, uint32_t> meshToBLAS;
        std::vector<MeshComponent*> uniqueMeshes;
        
        // Collect unique meshes and entities
        std::vector<Entity*> renderableEntities;
        auto containsCaseInsensitive = [](const std::string &haystack, const std::string &needle) -> bool {
            std::string h = haystack; std::string n = needle;
            std::transform(h.begin(), h.end(), h.begin(), [](unsigned char c){ return std::tolower(c); });
            std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c){ return std::tolower(c); });
            return h.find(n) != std::string::npos;
        };

        // Collect renderable entities for AS build without spamming logs.
        size_t skippedInactive = 0;
        size_t skippedNoMesh = 0;
        size_t skippedNoRes = 0;
        size_t skippedPendingUploads = 0;
        size_t skippedNullBuffers = 0;
        size_t skippedZeroIndices = 0;
        size_t skippedException = 0;

        for (const auto& entityPtr : entities) {
            Entity* entity = entityPtr.get();
            if (!entity || !entity->IsActive()) {
                skippedInactive++;
                continue;
            }
            
            auto meshComp = entity->GetComponent<MeshComponent>();
            if (!meshComp) {
                skippedNoMesh++;
                continue;
            }

            // Safely check if mesh resources exist - catch any exceptions from dereferencing potentially stale pointers
            try {
                auto meshIt = meshResources.find(meshComp);
                if (meshIt == meshResources.end()) {
                    skippedNoRes++;
                    continue;
                }
                
                // Validate that the mesh resources have valid buffers before adding to AS build
                const auto& meshRes = meshIt->second;
                // Only include when uploads finished (staging sizes are zero)
                if (meshRes.vertexBufferSizeBytes != 0 || meshRes.indexBufferSizeBytes != 0) {
                    // Skip meshes still uploading to avoid partial TLAS builds
                    skippedPendingUploads++;
                    continue;
                }
                // RAII handles: check if they contain valid Vulkan handles by dereferencing
                if (!*meshRes.vertexBuffer || !*meshRes.indexBuffer) {
                    skippedNullBuffers++;
                    continue;
                }
                
                if (meshRes.indexCount == 0) {
                    skippedZeroIndices++;
                    continue;
                }
            } catch (const std::exception& e) {
                // Avoid spamming; a rebuild on the next safe frame should succeed.
                skippedException++;
                continue;
            }
            
            renderableEntities.push_back(entity);
            
            if (meshToBLAS.find(meshComp) == meshToBLAS.end()) {
                meshToBLAS[meshComp] = static_cast<uint32_t>(uniqueMeshes.size());
                uniqueMeshes.push_back(meshComp);
            }
        }
        
        if (uniqueMeshes.empty()) {
            return true;
        }

        // One concise build summary (no per-entity spam)
        std::cout << "Building AS: uniqueMeshes=" << uniqueMeshes.size()
                  << ", instances=" << renderableEntities.size()
                  << " (skipped inactive=" << skippedInactive
                  << ", noMesh=" << skippedNoMesh
                  << ", noRes=" << skippedNoRes
                  << ", pendingUploads=" << skippedPendingUploads
                  << ", nullBuffers=" << skippedNullBuffers
                  << ", zeroIndices=" << skippedZeroIndices
                  << ", exception=" << skippedException
                  << ")\n";
        
        // Create a dedicated command pool for AS building to avoid threading issues
        // The main commandPool may be in use by the render thread
        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        
        vk::raii::CommandPool asBuildCommandPool(device, poolInfo);
        
        // Create command buffer for AS building
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = *asBuildCommandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;
        
        vk::raii::CommandBuffers cmdBuffers(device, allocInfo);
        vk::raii::CommandBuffer& cmdBuffer = cmdBuffers[0];
        
        cmdBuffer.begin(vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        });
        
        // (Vespa-only debugging removed; keep logs quiet.)

        // Build BLAS for each unique mesh
        blasStructures.resize(uniqueMeshes.size());
        
        // Keep scratch buffers alive until GPU execution completes (after fence wait)
        // Destroying them early causes "VkBuffer was destroy" validation errors and crashes
        std::vector<vk::raii::Buffer> scratchBuffers;
        std::vector<std::unique_ptr<MemoryPool::Allocation>> scratchAllocations;
        
        for (size_t i = 0; i < uniqueMeshes.size(); ++i) {
            // Update watchdog every 50 BLAS to prevent false hang detection during long AS build
            if (i > 0 && i % 50 == 0) {
                lastFrameUpdateTime.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
            }
            
            MeshComponent* meshComp = uniqueMeshes[i];
            auto& meshRes = meshResources.at(meshComp);
            
            // Get buffer device addresses
            vk::DeviceAddress vertexAddress = getBufferDeviceAddress(device, *meshRes.vertexBuffer);
            vk::DeviceAddress indexAddress = getBufferDeviceAddress(device, *meshRes.indexBuffer);
            
            // Compute vertex and index counts for this mesh
            const uint32_t vertexCount = static_cast<uint32_t>(meshComp->GetVertices().size());
            const auto &indicesCPU = meshComp->GetIndices();
            uint32_t maxIndexValue = 0;
            if (!indicesCPU.empty()) {
                // Find the maximum index actually referenced by this mesh
                maxIndexValue = *std::max_element(indicesCPU.begin(), indicesCPU.end());
            }
            
            // Create geometry info
            vk::AccelerationStructureGeometryKHR geometry{};
            geometry.geometryType = vk::GeometryTypeKHR::eTriangles;
            // Mark geometry as OPAQUE to ensure closest hits are committed reliably for primary rays
            // (we can re-introduce transparency later with any-hit/candidate handling)
            geometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
            
            geometry.geometry.triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;
            geometry.geometry.triangles.vertexData = vertexAddress;
            geometry.geometry.triangles.vertexStride = sizeof(Vertex);
            // Set maxVertex to the total vertex count for this mesh. This is the most robust
            // setting across drivers and content, and avoids culling triangles that reference
            // high vertex indices (observed to hide unique, single-instance meshes).
            geometry.geometry.triangles.maxVertex = vertexCount;
            geometry.geometry.triangles.indexType = vk::IndexType::eUint32;
            geometry.geometry.triangles.indexData = indexAddress;
            
            // Build info
            vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
            buildInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
            buildInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
            buildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
            buildInfo.geometryCount = 1;
            buildInfo.pGeometries = &geometry;
            
            uint32_t primitiveCount = meshRes.indexCount / 3;
            
            // Get size requirements
            vk::AccelerationStructureBuildSizesInfoKHR sizeInfo = device.getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice,
                buildInfo,
                primitiveCount
            );
            
            // Create BLAS buffer
            auto [blasBuffer, blasAlloc] = createBufferPooled(
                sizeInfo.accelerationStructureSize,
                vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                vk::MemoryPropertyFlagBits::eDeviceLocal
            );
            
            // Create acceleration structure
            vk::AccelerationStructureCreateInfoKHR createInfo{};
            createInfo.buffer = *blasBuffer;
            createInfo.size = sizeInfo.accelerationStructureSize;
            createInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
            
            vk::raii::AccelerationStructureKHR blasHandle(device, createInfo);
            
            // Create scratch buffer
            auto [scratchBuffer, scratchAlloc] = createBufferPooled(
                sizeInfo.buildScratchSize,
                vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                vk::MemoryPropertyFlagBits::eDeviceLocal
            );
            
            vk::DeviceAddress scratchAddress = getBufferDeviceAddress(device, *scratchBuffer);
            
            // Update build info with handles (dereference RAII handle)
            buildInfo.dstAccelerationStructure = *blasHandle;
            buildInfo.scratchData = scratchAddress;
            
            // Keep scratch buffer alive until after GPU execution (after fence wait)
            scratchBuffers.push_back(std::move(scratchBuffer));
            scratchAllocations.push_back(std::move(scratchAlloc));
            
            // Build range info
            vk::AccelerationStructureBuildRangeInfoKHR rangeInfo{};
            rangeInfo.primitiveCount = primitiveCount;
            rangeInfo.primitiveOffset = 0;
            rangeInfo.firstVertex = 0;
            rangeInfo.transformOffset = 0;
            
            // Record build command - Vulkan-Hpp RAII takes array spans, not pointers
            std::array<const vk::AccelerationStructureBuildRangeInfoKHR*, 1> rangeInfos = { &rangeInfo };
            cmdBuffer.buildAccelerationStructuresKHR(buildInfo, rangeInfos);
            
            // Get device address (dereference RAII handle)
            vk::AccelerationStructureDeviceAddressInfoKHR addressInfo{};
            addressInfo.accelerationStructure = *blasHandle;
            vk::DeviceAddress blasAddress = device.getAccelerationStructureAddressKHR(addressInfo);
            
            // Store BLAS (move RAII handle to avoid copy)
            blasStructures[i].buffer = std::move(blasBuffer);
            blasStructures[i].allocation = std::move(blasAlloc);
            blasStructures[i].handle = std::move(blasHandle);
            blasStructures[i].deviceAddress = blasAddress;
            
            // (Per-BLAS logging removed; keep logs quiet in production.)
        }

        // Barrier between BLAS and TLAS builds
        
        // Barrier between BLAS and TLAS builds
        vk::MemoryBarrier2 barrier{};
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR;
        barrier.srcAccessMask = vk::AccessFlagBits2::eAccelerationStructureWriteKHR;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR;
        barrier.dstAccessMask = vk::AccessFlagBits2::eAccelerationStructureReadKHR;
        
        vk::DependencyInfo depInfo{};
        depInfo.memoryBarrierCount = 1;
        depInfo.pMemoryBarriers = &barrier;
        cmdBuffer.pipelineBarrier2(depInfo);
        
        // Build TLAS with instances
        std::vector<vk::AccelerationStructureInstanceKHR> instances;
        instances.reserve(renderableEntities.size());

        // Build per-instance geometry info in the SAME order as TLAS instances
        std::vector<GeometryInfo> geometryInfos; // defined later in file; we reuse the type
        geometryInfos.reserve(renderableEntities.size());
        tlasInstanceOrder.clear();

        // Ray Query texture table (binding 6): seed reserved shared-default slots.
        // We will assign per-material texture indices into this table, and the descriptor update
        // will resolve each slot to either the streamed texture or a type-appropriate fallback.
        rayQueryTexKeys.clear();
        rayQueryTexFallbackSlots.clear();
        rayQueryTexIndex.clear();
        rayQueryTexCount = 0;

        auto seedReservedSlot = [&](uint32_t slot, const std::string &id) {
            if (rayQueryTexKeys.size() <= slot) {
                rayQueryTexKeys.resize(slot + 1);
                rayQueryTexFallbackSlots.resize(slot + 1);
            }
            const std::string key = ResolveTextureId(id);
            rayQueryTexKeys[slot] = key;
            rayQueryTexFallbackSlots[slot] = slot;
            rayQueryTexIndex[key] = slot;
        };

        seedReservedSlot(RQ_SLOT_DEFAULT_BASECOLOR, SHARED_DEFAULT_ALBEDO_ID);
        seedReservedSlot(RQ_SLOT_DEFAULT_NORMAL, SHARED_DEFAULT_NORMAL_ID);
        seedReservedSlot(RQ_SLOT_DEFAULT_METALROUGH, SHARED_DEFAULT_METALLIC_ROUGHNESS_ID);
        seedReservedSlot(RQ_SLOT_DEFAULT_OCCLUSION, SHARED_DEFAULT_OCCLUSION_ID);
        seedReservedSlot(RQ_SLOT_DEFAULT_EMISSIVE, SHARED_DEFAULT_EMISSIVE_ID);
        rayQueryTexCount = static_cast<uint32_t>(rayQueryTexKeys.size());

        auto addTextureSlot = [&](const std::string &texId, uint32_t fallbackSlot) -> uint32_t {
            if (texId.empty()) return fallbackSlot;
            std::string key = ResolveTextureId(texId);
            auto it = rayQueryTexIndex.find(key);
            if (it != rayQueryTexIndex.end()) return it->second;
            if (rayQueryTexCount >= RQ_MAX_TEX) return fallbackSlot;

            uint32_t slot = rayQueryTexCount;
            rayQueryTexKeys.push_back(key);
            rayQueryTexFallbackSlots.push_back(fallbackSlot);
            rayQueryTexIndex[key] = slot;
            rayQueryTexCount++;

            // Ensure streaming is requested (CPU-side decode can happen off-thread; GPU upload stays on main thread).
            try { RegisterTextureUser(key, nullptr); } catch (...) {}
            return slot;
        };

        uint32_t runningInstanceIndex = 0;
        for (auto entity : renderableEntities) {
            auto meshComp = entity->GetComponent<MeshComponent>();
            uint32_t blasIndex = meshToBLAS.at(meshComp);

            auto transform = entity->GetComponent<TransformComponent>();
            const glm::mat4 entityModel = transform ? transform->GetModelMatrix() : glm::mat4(1.0f);

            // Use per-instance transforms whenever at least one instance exists, even if only one.
            const size_t meshInstCount = meshComp->GetInstanceCount();
            const bool hasInstance = (meshInstCount > 0);
            const size_t instCount = std::max<size_t>(1, meshInstCount);

            for (size_t iInst = 0; iInst < instCount; ++iInst) {
                glm::mat4 finalModel = entityModel;
                if (hasInstance && iInst < meshInstCount) {
                    const InstanceData& id = meshComp->GetInstance(iInst);
                    finalModel = entityModel * id.getModelMatrix(); // match raster path: ubo.model * instanceModel
                }

                // Convert to Vulkan 3x4 row-major transform
                const float* m = glm::value_ptr(finalModel);
                vk::TransformMatrixKHR vkTransform;
                for (int row = 0; row < 3; row++) {
                    for (int col = 0; col < 4; col++) {
                        vkTransform.matrix[row][col] = m[col * 4 + row];
                    }
                }

                // (Debug TLAS-XFORM logs removed for production)

                vk::AccelerationStructureInstanceKHR AS_Instance{};
                AS_Instance.transform = vkTransform;
                AS_Instance.instanceCustomIndex = runningInstanceIndex; // per-instance sequential index
                // Instance mask: include all instances by default.
                AS_Instance.mask = 0xFF;
                // Mirror the per-instance index into the SBT record offset so either
                // CommittedInstanceID() or CommittedInstanceContributionToHitGroupIndex()
                // can be used in the shader to recover the per-instance index.
                AS_Instance.instanceShaderBindingTableRecordOffset = runningInstanceIndex;
                // Disable facing cull at the instance level to ensure both front and back faces
                // are considered during traversal.
                //
                // IMPORTANT: For alpha-masked materials (foliage), we must NOT force opaque.
                // Ray Query inline traversal has no any-hit shader, so we emulate alpha testing
                // by committing candidates only when baseColor alpha passes the cutoff.
                VkGeometryInstanceFlagsKHR instFlags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
                bool forceNoOpaque = false;
                {
                    // Determine alpha mode for this entity's material.
                    // Entity name format: "modelName_Material_<index>_<materialName>".
                    const std::string &entityName = entity->GetName();
                    size_t matPos = entityName.find("_Material_");
                    if (matPos != std::string::npos) {
                        size_t numStart = matPos + 10;
                        size_t numEnd = entityName.find('_', numStart);
                        if (numEnd != std::string::npos && numEnd + 1 < entityName.size() && modelLoader) {
                            std::string matName = entityName.substr(numEnd + 1);
                            if (Material* m = modelLoader->GetMaterial(matName)) {
                                // Only MASK requires candidate hits for alpha test.
                                forceNoOpaque = (m->alphaMode == "MASK");
                            }
                        }
                    }
                }
                instFlags |= forceNoOpaque ? VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR
                                           : VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
                AS_Instance.flags = static_cast<VkGeometryInstanceFlagsKHR>(instFlags);
                AS_Instance.accelerationStructureReference = blasStructures[blasIndex].deviceAddress;
                instances.push_back(AS_Instance);

                // Track mapping for refit
                TlasInstanceRef ref{};
                ref.entity = entity;
                ref.instanced = hasInstance;
                ref.instanceIndex = static_cast<uint32_t>(hasInstance ? iInst : 0);
                tlasInstanceOrder.push_back(ref);

                // Build geometry info entry for this instance (addresses identical for all instances of same mesh)
                const auto& meshRes = meshResources.at(meshComp);
                vk::DeviceAddress vertexAddr = getBufferDeviceAddress(device, *meshRes.vertexBuffer);
                vk::DeviceAddress indexAddr = getBufferDeviceAddress(device, *meshRes.indexBuffer);

                // Extract material index from entity name (model_Material_{index}_materialName)
                int32_t materialIndex = -1;
                {
                    const std::string& entityName = entity->GetName();
                    size_t matPos = entityName.find("_Material_");
                    if (matPos != std::string::npos) {
                        size_t numStart = matPos + 10;  // length of "_Material_"
                        size_t numEnd = entityName.find('_', numStart);
                        if (numEnd != std::string::npos) {
                            try {
                                materialIndex = std::stoi(entityName.substr(numStart, numEnd - numStart));
                            } catch (...) {
                                materialIndex = -1;
                            }
                        }
                    }
                }

                GeometryInfo gi{};
                gi.vertexBufferAddress = vertexAddr;
                gi.indexBufferAddress = indexAddr;
                gi.vertexCount = static_cast<uint32_t>(meshComp->GetVertices().size());
                gi.materialIndex = static_cast<uint32_t>(std::max(0, materialIndex));
                // Provide indexCount so shader can bound-check primitiveIndex safely
                gi.indexCount = meshRes.indexCount;
                gi._pad0 = 0;
                // Store normal transform for correct world-space normals and tangent-space normal mapping.
                // Use the full per-instance finalModel (entityModel * instanceModel) to match raster.
                {
                    glm::mat3 nrm = glm::transpose(glm::inverse(glm::mat3(finalModel)));
                    gi.normalMatrix0 = glm::vec4(nrm[0], 0.0f);
                    gi.normalMatrix1 = glm::vec4(nrm[1], 0.0f);
                    gi.normalMatrix2 = glm::vec4(nrm[2], 0.0f);
                }
                geometryInfos.push_back(gi);

                runningInstanceIndex++;
            }
        }

        // Build TLAS

        // Create instances buffer (persistent for TLAS UPDATE/Refit)
        vk::DeviceSize instancesSize = sizeof(vk::AccelerationStructureInstanceKHR) * instances.size();
        auto [instancesBufferTmp, instancesAllocTmp] = createBufferPooled(
            instancesSize,
            vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        
        // Upload instances - use mappedPtr directly
        void* instancesData = instancesAllocTmp->mappedPtr;
        if (!instancesData) {
            std::cerr << "Failed to get mapped pointer for instances buffer\n";
            return false;
        }
        memcpy(instancesData, instances.data(), instancesSize);
        
        // Persist instances buffer/allocation and order for UPDATE (refit)
        tlasInstancesBuffer = std::move(instancesBufferTmp);
        tlasInstancesAllocation = std::move(instancesAllocTmp);
        tlasInstanceCount = static_cast<uint32_t>(instances.size());
        // tlasInstanceOrder already filled above in the same order as 'instances'

        vk::DeviceAddress instancesAddress = getBufferDeviceAddress(device, *tlasInstancesBuffer);
        
        // TLAS geometry
        vk::AccelerationStructureGeometryKHR tlasGeometry{};
        tlasGeometry.geometryType = vk::GeometryTypeKHR::eInstances;
        // Do not force OPAQUE here; leave flags at 0 so ray queries may process
        // transparency/glass more flexibly (any-hit not used in our path).
        tlasGeometry.flags = {};
        tlasGeometry.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
        tlasGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
        tlasGeometry.geometry.instances.data = instancesAddress;
        
        // TLAS build info
        vk::AccelerationStructureBuildGeometryInfoKHR tlasBuildInfo{};
        tlasBuildInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
        tlasBuildInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
                              vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate; // enable UPDATE/Refit
        tlasBuildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
        tlasBuildInfo.geometryCount = 1;
        tlasBuildInfo.pGeometries = &tlasGeometry;
        
        auto instanceCount = static_cast<uint32_t>(instances.size());
        
        // Get TLAS size requirements
        vk::AccelerationStructureBuildSizesInfoKHR tlasSizeInfo = device.getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice,
            tlasBuildInfo,
            instanceCount
        );
        
        // Create TLAS buffer
        auto [tlasBuffer, tlasAlloc] = createBufferPooled(
            tlasSizeInfo.accelerationStructureSize,
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );
        
        // Create TLAS
        vk::AccelerationStructureCreateInfoKHR tlasCreateInfo{};
        tlasCreateInfo.buffer = *tlasBuffer;
        tlasCreateInfo.size = tlasSizeInfo.accelerationStructureSize;
        tlasCreateInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
        
        vk::raii::AccelerationStructureKHR tlasHandle(device, tlasCreateInfo);
        
        // Create TLAS scratch buffer (for initial build)
        auto [tlasScratchBuffer, tlasScratchAlloc] = createBufferPooled(
            tlasSizeInfo.buildScratchSize,
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );
        
        vk::DeviceAddress tlasScratchAddress = getBufferDeviceAddress(device, *tlasScratchBuffer);
        
        // Update TLAS build info (dereference RAII handle)
        tlasBuildInfo.dstAccelerationStructure = *tlasHandle;
        tlasBuildInfo.scratchData = tlasScratchAddress;
        
        // Keep TLAS scratch buffer alive until after GPU execution (after fence wait)
        scratchBuffers.push_back(std::move(tlasScratchBuffer));
        scratchAllocations.push_back(std::move(tlasScratchAlloc));

        // Ensure/update a persistent scratch buffer for TLAS UPDATE (refit)
        // Allocate once sized to updateScratchSize
        if (!*tlasUpdateScratchBuffer || !tlasUpdateScratchAllocation) {
            auto [updBuf, updAlloc] = createBufferPooled(
                tlasSizeInfo.updateScratchSize,
                vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                vk::MemoryPropertyFlagBits::eDeviceLocal
            );
            tlasUpdateScratchBuffer = std::move(updBuf);
            tlasUpdateScratchAllocation = std::move(updAlloc);
        }
        
        // TLAS build range
        vk::AccelerationStructureBuildRangeInfoKHR tlasRangeInfo{};
        tlasRangeInfo.primitiveCount = instanceCount;
        tlasRangeInfo.primitiveOffset = 0;
        tlasRangeInfo.firstVertex = 0;
        tlasRangeInfo.transformOffset = 0;
        
        // Build TLAS - Vulkan-Hpp RAII takes array spans, not pointers
        std::array<const vk::AccelerationStructureBuildRangeInfoKHR*, 1> tlasRangeInfos = { &tlasRangeInfo };
        cmdBuffer.buildAccelerationStructuresKHR(tlasBuildInfo, tlasRangeInfos);
        
        // Get TLAS device address (dereference RAII handle)
        vk::AccelerationStructureDeviceAddressInfoKHR tlasAddressInfo{};
        tlasAddressInfo.accelerationStructure = *tlasHandle;
        vk::DeviceAddress tlasAddress = device.getAccelerationStructureAddressKHR(tlasAddressInfo);
        
        // Store TLAS (move RAII handle to avoid copy)
        tlasStructure.buffer = std::move(tlasBuffer);
        tlasStructure.allocation = std::move(tlasAlloc);
        tlasStructure.handle = std::move(tlasHandle);
        tlasStructure.deviceAddress = tlasAddress;
        
        cmdBuffer.end();
        
        // Submit and wait
        vk::SubmitInfo submitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &(*cmdBuffer);
        
        vk::raii::Fence fence(device, vk::FenceCreateInfo{});
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            graphicsQueue.submit(submitInfo, *fence);
        }
        
        if (device.waitForFences(*fence, VK_TRUE, UINT64_MAX) != vk::Result::eSuccess) {
            std::cerr << "Failed to wait for AS build fence\n";
            return false;
        }
        
        // (Verbose TLAS composition dumps removed; keep logs quiet.)

        // Record the counts we just built so we don't rebuild with smaller subsets later
        lastASBuiltBLASCount = blasStructures.size();
        lastASBuiltInstanceCount = instanceCount;
        
        // Build geometry info buffer PER INSTANCE (same order as TLAS instances)
        // geometryInfos already populated above in TLAS instance loop
        
        // Create and upload geometry info buffer
        if (!geometryInfos.empty()) {
            vk::DeviceSize geoInfoSize = sizeof(GeometryInfo) * geometryInfos.size();
            auto [geoInfoBuf, geoInfoAlloc] = createBufferPooled(
                geoInfoSize,
                vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            );
            
            void* geoInfoData = geoInfoAlloc->mappedPtr;
            if (geoInfoData) {
                memcpy(geoInfoData, geometryInfos.data(), geoInfoSize);
            }
            
            geometryInfoBuffer = std::move(geoInfoBuf);
            geometryInfoAllocation = std::move(geoInfoAlloc);
            geometryInfoCountCPU = geometryInfos.size();
            
            // (Verbose geometry info buffer stats removed.)
        }
        
        // Build material buffer with real materials from ModelLoader
        {
            // Build material buffer
            
            // Collect unique materials with their indices from entities
            std::map<uint32_t, std::string> materialIndexToName;
            
            size_t entityCount = 0;
            for (Entity* entity : renderableEntities) {
                std::string entityName = entity->GetName();
                
                // Parse entity name: "modelName_Material_{materialIndex}_materialName"
                size_t matPos = entityName.find("_Material_");
                if (matPos != std::string::npos) {
                    size_t numStart = matPos + 10;  // length of "_Material_"
                    size_t numEnd = entityName.find('_', numStart);
                    
                    if (numEnd != std::string::npos) {
                        try {
                            uint32_t matIndex = std::stoi(entityName.substr(numStart, numEnd - numStart));
                            
                            // Extract material name (everything after materialIndex_)
                            std::string materialName = entityName.substr(numEnd + 1);
                            materialIndexToName[matIndex] = materialName;
                        } catch (...) {
                            // Failed to parse, skip
                        }
                    }
                }
                
                entityCount++;
                // Progress indicator removed (log-noise)
            }
            
            // (Verbose material discovery logs removed.)
            
            // Create default material for index 0 and any missing indices
            MaterialData defaultMat{};
            defaultMat.albedo = glm::vec3(0.8f, 0.8f, 0.8f);
            defaultMat.metallic = 0.0f;
            defaultMat.roughness = 0.5f;
            defaultMat.emissive = glm::vec3(0.0f);
            defaultMat.ao = 1.0f;
            defaultMat.ior = 1.5f;
            defaultMat.emissiveStrength = 1.0f;
            defaultMat.alpha = 1.0f;
            defaultMat.transmissionFactor = 0.0f;
            defaultMat.alphaCutoff = 0.5f;
            defaultMat.alphaMode = 0; // OPAQUE
            defaultMat.isGlass = 0;
            defaultMat.isLiquid = 0;
            // Texture-set flags: -1 = no texture bound for that channel
            defaultMat.baseColorTextureSet = -1;
            defaultMat.physicalDescriptorTextureSet = -1;
            defaultMat.normalTextureSet = -1;
            defaultMat.occlusionTextureSet = -1;
            defaultMat.emissiveTextureSet = -1;
            // Default texture indices (reserved slots)
            defaultMat.baseColorTexIndex = static_cast<int32_t>(RQ_SLOT_DEFAULT_BASECOLOR);
            defaultMat.normalTexIndex = static_cast<int32_t>(RQ_SLOT_DEFAULT_NORMAL);
            defaultMat.physicalTexIndex = static_cast<int32_t>(RQ_SLOT_DEFAULT_METALROUGH);
            defaultMat.occlusionTexIndex = static_cast<int32_t>(RQ_SLOT_DEFAULT_OCCLUSION);
            defaultMat.emissiveTexIndex = static_cast<int32_t>(RQ_SLOT_DEFAULT_EMISSIVE);
            defaultMat.useSpecGlossWorkflow = 0;
            defaultMat.glossinessFactor = 1.0f;
            defaultMat.specularFactor = glm::vec3(0.04f);
            defaultMat.hasEmissiveStrengthExt = 0;
            defaultMat._padMat[0] = defaultMat._padMat[1] = defaultMat._padMat[2] = 0;
            
            // Build material array with proper indexing
            std::vector<MaterialData> materials;
            
            // Determine max material index to size the array
            uint32_t maxMaterialIndex = 0;
            for (const auto& [index, name] : materialIndexToName) {
                maxMaterialIndex = std::max(maxMaterialIndex, index);
            }
            
            // Ensure minimum size of 100 materials for safety (matches original implementation)
            uint32_t materialCount = std::max(maxMaterialIndex + 1, 100u);
            materials.resize(materialCount, defaultMat);
            
            // Capture per-material texture paths (for streaming requests and debugging)
            rqMaterialTexPaths.clear();
            rqMaterialTexPaths.resize(materials.size());

            // Populate materials from ModelLoader
            uint32_t loadedCount = 0;
            uint32_t glassCount = 0;
            uint32_t transparentCount = 0;
            if (modelLoader) {
                // Populate materials from ModelLoader
                size_t matProcessed = 0;
                for (const auto& [index, materialName] : materialIndexToName) {
                    Material* sourceMat = modelLoader->GetMaterial(materialName);
                    if (sourceMat) {
                        MaterialData& matData = materials[index];
                        
                        // Copy PBR properties from Material to MaterialData
                        matData.albedo = sourceMat->albedo;
                        matData.metallic = sourceMat->metallic;
                        matData.emissive = sourceMat->emissive;
                        matData.roughness = sourceMat->roughness;
                        matData.ao = sourceMat->ao;
                        matData.ior = sourceMat->ior;
                        matData.emissiveStrength = sourceMat->emissiveStrength;
                        matData.alpha = sourceMat->alpha;
                        matData.transmissionFactor = sourceMat->transmissionFactor;
                        matData.alphaCutoff = sourceMat->alphaCutoff;
                        // Alpha mode encoding must match `shaders/ray_query.slang`:
                        // 0=OPAQUE, 1=MASK, 2=BLEND
                        if (sourceMat->alphaMode == "MASK") {
                            matData.alphaMode = 1;
                        } else if (sourceMat->alphaMode == "BLEND") {
                            matData.alphaMode = 2;
                        } else {
                            matData.alphaMode = 0;
                        }
                        matData.isGlass = sourceMat->isGlass ? 1u : 0u;
                        matData.isLiquid = sourceMat->isLiquid ? 1u : 0u;

                        // Texture-set flags (raster parity): -1 means no texture is authored for that slot.
                        matData.baseColorTextureSet = sourceMat->albedoTexturePath.empty() ? -1 : 0;
                        if (sourceMat->useSpecularGlossiness) {
                            matData.physicalDescriptorTextureSet = sourceMat->specGlossTexturePath.empty() ? -1 : 0;
                        } else {
                            matData.physicalDescriptorTextureSet = sourceMat->metallicRoughnessTexturePath.empty() ? -1 : 0;
                        }
                        matData.normalTextureSet = sourceMat->normalTexturePath.empty() ? -1 : 0;
                        matData.occlusionTextureSet = sourceMat->occlusionTexturePath.empty() ? -1 : 0;
                        matData.emissiveTextureSet = sourceMat->emissiveTexturePath.empty() ? -1 : 0;

                        // Texture paths and stable indices into the Ray Query texture table (binding 6)
                        if (index < rqMaterialTexPaths.size()) {
                            RQMaterialTexPaths &paths = rqMaterialTexPaths[index];
                            paths.baseColor = sourceMat->albedoTexturePath;
                            paths.normal = sourceMat->normalTexturePath;
                            paths.physical = sourceMat->useSpecularGlossiness ? sourceMat->specGlossTexturePath
                                                                               : sourceMat->metallicRoughnessTexturePath;
                            paths.occlusion = sourceMat->occlusionTexturePath;
                            paths.emissive = sourceMat->emissiveTexturePath;

                            matData.baseColorTexIndex = static_cast<int32_t>(addTextureSlot(paths.baseColor, RQ_SLOT_DEFAULT_BASECOLOR));
                            matData.normalTexIndex = static_cast<int32_t>(addTextureSlot(paths.normal, RQ_SLOT_DEFAULT_NORMAL));
                            matData.physicalTexIndex = static_cast<int32_t>(addTextureSlot(paths.physical, RQ_SLOT_DEFAULT_METALROUGH));
                            matData.occlusionTexIndex = static_cast<int32_t>(addTextureSlot(paths.occlusion, RQ_SLOT_DEFAULT_OCCLUSION));
                            matData.emissiveTexIndex = static_cast<int32_t>(addTextureSlot(paths.emissive, RQ_SLOT_DEFAULT_EMISSIVE));
                        }

                        // Specular-glossiness workflow support
                        matData.useSpecGlossWorkflow = sourceMat->useSpecularGlossiness ? 1 : 0;
                        matData.glossinessFactor = sourceMat->glossinessFactor;
                        matData.specularFactor = sourceMat->specularFactor;
                        matData.hasEmissiveStrengthExt = (std::abs(sourceMat->emissiveStrength - 1.0f) > 1e-6f) ? 1 : 0;
                        matData._padMat[0] = matData._padMat[1] = matData._padMat[2] = 0;
                        
                        // Track glass and transparent materials for statistics
                        if (sourceMat->isGlass) {
                            glassCount++;
                        }
                        if (sourceMat->transmissionFactor > 0.1f) {
                            transparentCount++;
                        }
                        
                        loadedCount++;
                    } else {
                        std::cerr << "Warning: Material '" << materialName 
                                  << "' not found in ModelLoader for index " << index << "\n";
                    }
                    
                    matProcessed++;
                }
            } else {
                std::cerr << "Warning: ModelLoader not available, using default materials\n";
            }
            
            // Create and upload material buffer (always create, even if no materials found)
            vk::DeviceSize matSize = sizeof(MaterialData) * materials.size();
            auto [matBuf, matAlloc] = createBufferPooled(
                matSize,
                vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            );
            
            void* matData = matAlloc->mappedPtr;
            if (matData) {
                memcpy(matData, materials.data(), matSize);
            }
            
            materialBuffer = std::move(matBuf);
            materialAllocation = std::move(matAlloc);
            
            // (Verbose material buffer stats removed.)

            // Record material count for shader-side bounds (provided via UBO)
            materialCountCPU = materials.size();
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to build acceleration structures: " << e.what() << std::endl;
        return false;
    }
}

bool Renderer::refitTopLevelAS(const std::vector<std::unique_ptr<Entity>>& entities) {
    try {
        if (!rayQueryEnabled || !accelerationStructureEnabled) return false;
        if (!*tlasStructure.handle) return false;
        if (!*tlasInstancesBuffer || !tlasInstancesAllocation || tlasInstanceOrder.size() != tlasInstanceCount) return false;

        // Update instance transforms in the persistent instances buffer
        auto* instPtr = reinterpret_cast<vk::AccelerationStructureInstanceKHR*>(tlasInstancesAllocation->mappedPtr);
        if (!instPtr) return false;

        for (uint32_t i = 0; i < tlasInstanceCount; ++i) {
            const TlasInstanceRef &ref = tlasInstanceOrder[i];
            Entity* entity = ref.entity;
            if (!entity || !entity->IsActive()) continue;
            auto* transform = entity->GetComponent<TransformComponent>();
            glm::mat4 entityModel = transform ? transform->GetModelMatrix() : glm::mat4(1.0f);

            // If this TLAS entry represents a MeshComponent instance, multiply by the instance's model
            glm::mat4 finalModel = entityModel;
            if (ref.instanced) {
                auto* meshComp = entity->GetComponent<MeshComponent>();
                if (meshComp && ref.instanceIndex < meshComp->GetInstanceCount()) {
                    const InstanceData &id = meshComp->GetInstance(ref.instanceIndex);
                    finalModel = entityModel * id.getModelMatrix();
                }
            }

            const float* m = glm::value_ptr(finalModel);
            vk::TransformMatrixKHR vkTransform;
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 4; col++) {
                    vkTransform.matrix[row][col] = m[col * 4 + row];
                }
            }
            instPtr[i].transform = vkTransform;
        }

        // Prepare UPDATE build info
        vk::DeviceAddress instancesAddress = getBufferDeviceAddress(device, *tlasInstancesBuffer);

        vk::AccelerationStructureGeometryKHR tlasGeometry{};
        tlasGeometry.geometryType = vk::GeometryTypeKHR::eInstances;
        tlasGeometry.flags = {};
        tlasGeometry.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
        tlasGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
        tlasGeometry.geometry.instances.data = instancesAddress;

        vk::AccelerationStructureBuildGeometryInfoKHR tlasBuildInfo{};
        tlasBuildInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
        tlasBuildInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
                              vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
        tlasBuildInfo.mode = vk::BuildAccelerationStructureModeKHR::eUpdate;
        tlasBuildInfo.geometryCount = 1;
        tlasBuildInfo.pGeometries = &tlasGeometry;
        tlasBuildInfo.srcAccelerationStructure = *tlasStructure.handle;
        tlasBuildInfo.dstAccelerationStructure = *tlasStructure.handle;

        if (!*tlasUpdateScratchBuffer || !tlasUpdateScratchAllocation) {
            // No update scratch; cannot refit
            return false;
        }
        vk::DeviceAddress updateScratch = getBufferDeviceAddress(device, *tlasUpdateScratchBuffer);
        tlasBuildInfo.scratchData = updateScratch;

        vk::AccelerationStructureBuildRangeInfoKHR tlasRangeInfo{};
        tlasRangeInfo.primitiveCount = tlasInstanceCount;
        tlasRangeInfo.primitiveOffset = 0;
        tlasRangeInfo.firstVertex = 0;
        tlasRangeInfo.transformOffset = 0;

        // Create transient command buffer for UPDATE
        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        vk::raii::CommandPool cmdPool(device, poolInfo);

        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = *cmdPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;
        vk::raii::CommandBuffers cmdBuffers(device, allocInfo);
        vk::raii::CommandBuffer& cmd = cmdBuffers[0];
        cmd.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

        std::array<const vk::AccelerationStructureBuildRangeInfoKHR*, 1> ranges = { &tlasRangeInfo };
        cmd.buildAccelerationStructuresKHR(tlasBuildInfo, ranges);

        cmd.end();

        // Submit and wait
        vk::SubmitInfo submitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &(*cmd);
        vk::raii::Fence fence(device, vk::FenceCreateInfo{});
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            graphicsQueue.submit(submitInfo, *fence);
        }
        if (device.waitForFences(*fence, VK_TRUE, UINT64_MAX) != vk::Result::eSuccess) {
            std::cerr << "Failed to wait for TLAS refit fence\n";
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to refit TLAS: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Update ray query descriptor sets with current resources.
 * 
 * Binds UBO, TLAS, output image, and light buffer to the descriptor set.
 * 
 * @param frameIndex The frame index to update.
 * @return True if successful, false otherwise.
 */
bool Renderer::updateRayQueryDescriptorSets(uint32_t frameIndex, const std::vector<std::unique_ptr<Entity>>& entities) {
    if (!rayQueryEnabled || !accelerationStructureEnabled) {
        return false;
    }

    // Do not update descriptors while descriptor sets are known invalid
    if (!descriptorSetsValid.load(std::memory_order_relaxed)) {
        return false;
    }

    // Ensure descriptor sets exist for this frame; if missing/invalid, (re)allocate them now at the safe point
    auto ensureRayQuerySets = [&]() -> bool {
        try {
            if (rayQueryDescriptorSets.empty() || frameIndex >= rayQueryDescriptorSets.size()) {
                std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *rayQueryDescriptorSetLayout);
                vk::DescriptorSetAllocateInfo allocInfo{};
                allocInfo.descriptorPool = *descriptorPool;
                allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
                allocInfo.pSetLayouts = layouts.data();
                {
                    std::lock_guard<std::mutex> lk(descriptorMutex);
                    rayQueryDescriptorSets = vk::raii::DescriptorSets(device, allocInfo);
                }
            }
            // Validate the handle for the current frame
            vk::DescriptorSet testHandle = *rayQueryDescriptorSets[frameIndex];
            if (!testHandle) {
                // Reallocate once more if handle is null
                std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *rayQueryDescriptorSetLayout);
                vk::DescriptorSetAllocateInfo allocInfo{};
                allocInfo.descriptorPool = *descriptorPool;
                allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
                allocInfo.pSetLayouts = layouts.data();
                {
                    std::lock_guard<std::mutex> lk(descriptorMutex);
                    rayQueryDescriptorSets = vk::raii::DescriptorSets(device, allocInfo);
                }
                testHandle = *rayQueryDescriptorSets[frameIndex];
                if (!testHandle) return false;
            }
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Ray query descriptor set (re)allocation failed: " << e.what() << "\n";
            return false;
        }
    };
    if (!ensureRayQuerySets()) {
        return false;
    }
    
    // Validate descriptor set handle is valid before dereferencing
    try {
        vk::DescriptorSet testHandle = *rayQueryDescriptorSets[frameIndex];
        if (!testHandle) {
            // Try reallocate once more
            if (!ensureRayQuerySets()) return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Ray query descriptor set handle invalid for frame " << frameIndex << ": " << e.what() << "\n";
        if (!ensureRayQuerySets()) return false;
    }
    
    // Check if TLAS handle is valid (dereference RAII handle to check underlying VkAccelerationStructureKHR)
    if (!*tlasStructure.handle) {
        std::cerr << "TLAS not built - cannot update ray query descriptor sets\n";
        return false;
    }
    
    // Frame index alignment check: ensure we are updating descriptor set for the frame being recorded
    if (frameIndex != currentFrame) {
        // Not fatal, but indicates a mismatch in frame scheduling
        // Avoid noisy logs every frame
    }

    // TLAS is valid at this point; avoid verbose logging in default builds
    vk::AccelerationStructureKHR tlasHandleValue = *tlasStructure.handle;
    
    if (lightStorageBuffers.empty() || frameIndex >= lightStorageBuffers.size()) {
        std::cerr << "Light storage buffers not initialized\n";
        return false;
    }
    
    try {
        // NOTE: Ray Query no longer stores per-instance texture indices in `GeometryInfo`.
        // Textures are resolved per-material via the material buffer, and the descriptor array
        // is rebuilt each update from current streamed texture handles.

        std::vector<vk::WriteDescriptorSet> writes;

        // NOTE: Do not write into mapped geometry info here. The buffer is built at AS build time
        // and remains immutable to avoid races with refit and descriptor updates.
        
        // Binding 0: UBO - Use dedicated ray query UBO (not entity UBO)
        if (rayQueryUniformBuffers.empty() || frameIndex >= rayQueryUniformBuffers.size()) {
            std::cerr << "Ray query UBO not initialized for frame " << frameIndex << "\n";
            return false;
        }
        
        vk::DescriptorBufferInfo uboInfo{};
        uboInfo.buffer = *rayQueryUniformBuffers[frameIndex];
        uboInfo.offset = 0;
        uboInfo.range = sizeof(RayQueryUniformBufferObject);
        
        vk::WriteDescriptorSet uboWrite{};
        uboWrite.dstSet = *rayQueryDescriptorSets[frameIndex];
        uboWrite.dstBinding = 0;
        uboWrite.dstArrayElement = 0;
        uboWrite.descriptorCount = 1;
        uboWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        uboWrite.pBufferInfo = &uboInfo;
        writes.push_back(uboWrite);
        
        // Binding 1: TLAS (get address of underlying VkAccelerationStructureKHR)
        vk::AccelerationStructureKHR tlasHandleValue = *tlasStructure.handle;
        vk::WriteDescriptorSetAccelerationStructureKHR tlasInfo{};
        tlasInfo.accelerationStructureCount = 1;
        tlasInfo.pAccelerationStructures = &tlasHandleValue;
        
        vk::WriteDescriptorSet tlasWrite{};
        tlasWrite.dstSet = *rayQueryDescriptorSets[frameIndex];
        tlasWrite.dstBinding = 1;
        tlasWrite.dstArrayElement = 0;
        tlasWrite.descriptorCount = 1;
        tlasWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
        tlasWrite.pNext = &tlasInfo;
        writes.push_back(tlasWrite);
        
        // Binding 2: Output image
        vk::DescriptorImageInfo imageInfo{};
        imageInfo.imageView = *rayQueryOutputImageView;
        imageInfo.imageLayout = vk::ImageLayout::eGeneral;
        
        vk::WriteDescriptorSet imageWrite{};
        imageWrite.dstSet = *rayQueryDescriptorSets[frameIndex];
        imageWrite.dstBinding = 2;
        imageWrite.dstArrayElement = 0;
        imageWrite.descriptorCount = 1;
        imageWrite.descriptorType = vk::DescriptorType::eStorageImage;
        imageWrite.pImageInfo = &imageInfo;
        writes.push_back(imageWrite);
        
        // Binding 3: Light buffer
        vk::DescriptorBufferInfo lightInfo{};
        lightInfo.buffer = *lightStorageBuffers[frameIndex].buffer;
        lightInfo.offset = 0;
        lightInfo.range = VK_WHOLE_SIZE;
        
        vk::WriteDescriptorSet lightWrite{};
        lightWrite.dstSet = *rayQueryDescriptorSets[frameIndex];
        lightWrite.dstBinding = 3;
        lightWrite.dstArrayElement = 0;
        lightWrite.descriptorCount = 1;
        lightWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
        lightWrite.pBufferInfo = &lightInfo;
        writes.push_back(lightWrite);
        
        // Binding 4: Geometry info buffer (vertex/index addresses + material indices)
        if (*geometryInfoBuffer) {
            vk::DescriptorBufferInfo geoInfo{};
            geoInfo.buffer = *geometryInfoBuffer;
            geoInfo.offset = 0;
            geoInfo.range = VK_WHOLE_SIZE;
            
            vk::WriteDescriptorSet geoWrite{};
            geoWrite.dstSet = *rayQueryDescriptorSets[frameIndex];
            geoWrite.dstBinding = 4;
            geoWrite.dstArrayElement = 0;
            geoWrite.descriptorCount = 1;
            geoWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
            geoWrite.pBufferInfo = &geoInfo;
            writes.push_back(geoWrite);
        }
        
        // Binding 5: Material buffer (PBR material properties)
        if (*materialBuffer) {
            vk::DescriptorBufferInfo matInfo{};
            matInfo.buffer = *materialBuffer;
            matInfo.offset = 0;
            matInfo.range = VK_WHOLE_SIZE;
            
            vk::WriteDescriptorSet matWrite{};
            matWrite.dstSet = *rayQueryDescriptorSets[frameIndex];
            matWrite.dstBinding = 5;
            matWrite.dstArrayElement = 0;
            matWrite.descriptorCount = 1;
            matWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
            matWrite.pBufferInfo = &matInfo;
            writes.push_back(matWrite);
        }

        // Binding 6: Ray Query texture table (combined image samplers)
        // IMPORTANT: Do NOT cache VkImageView/VkSampler handles across frames; textures can stream
        // and their handles may be destroyed/recreated. Instead, rebuild image infos each update.
        if (rayQueryTexKeys.size() < RQ_SLOT_DEFAULT_EMISSIVE + 1 || rayQueryTexFallbackSlots.size() < RQ_SLOT_DEFAULT_EMISSIVE + 1) {
            // Should be seeded during AS build; if not, fall back to using the generic default texture in all slots.
            rayQueryTexKeys.resize(RQ_SLOT_DEFAULT_EMISSIVE + 1);
            rayQueryTexFallbackSlots.resize(RQ_SLOT_DEFAULT_EMISSIVE + 1);
            rayQueryTexCount = std::max<uint32_t>(rayQueryTexCount, static_cast<uint32_t>(rayQueryTexKeys.size()));
        }

        std::vector<vk::DescriptorImageInfo> rqArray(RQ_MAX_TEX, vk::DescriptorImageInfo{
            .sampler = *defaultTextureResources.textureSampler,
            .imageView = *defaultTextureResources.textureImageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        });

        const uint32_t copyCount = std::min<uint32_t>(rayQueryTexCount, RQ_MAX_TEX);
        std::shared_lock<std::shared_mutex> texLock(textureResourcesMutex);

        // Helper to fill a slot with a key (if ready) or fall back to its declared fallback slot.
        auto fillSlot = [&](uint32_t slot) {
            if (slot >= copyCount) return;
            const std::string &key = rayQueryTexKeys[slot];
            if (!key.empty()) {
                auto itTex = textureResources.find(key);
                if (itTex != textureResources.end() && itTex->second.textureImageView != nullptr && itTex->second.textureSampler != nullptr) {
                    rqArray[slot].sampler = *itTex->second.textureSampler;
                    rqArray[slot].imageView = *itTex->second.textureImageView;
                    rqArray[slot].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                    return;
                }
            }

            // Not ready/missing: use slot-specific fallback.
            uint32_t fb = (slot < rayQueryTexFallbackSlots.size()) ? rayQueryTexFallbackSlots[slot] : RQ_SLOT_DEFAULT_BASECOLOR;
            if (fb >= copyCount) fb = RQ_SLOT_DEFAULT_BASECOLOR;
            const std::string &fbKey = (fb < rayQueryTexKeys.size()) ? rayQueryTexKeys[fb] : std::string{};
            if (!fbKey.empty()) {
                auto itTex = textureResources.find(fbKey);
                if (itTex != textureResources.end() && itTex->second.textureImageView != nullptr && itTex->second.textureSampler != nullptr) {
                    rqArray[slot].sampler = *itTex->second.textureSampler;
                    rqArray[slot].imageView = *itTex->second.textureImageView;
                    rqArray[slot].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                }
            }
        };

        // Fill all active slots.
        for (uint32_t i = 0; i < copyCount; ++i) {
            fillSlot(i);
        }

        vk::WriteDescriptorSet texArrayWrite{};
        texArrayWrite.dstSet = *rayQueryDescriptorSets[frameIndex];
        texArrayWrite.dstBinding = 6;
        texArrayWrite.dstArrayElement = 0;
        texArrayWrite.descriptorCount = RQ_MAX_TEX;
        texArrayWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        texArrayWrite.pImageInfo = rqArray.data();
        writes.push_back(texArrayWrite);

        device.updateDescriptorSets(writes, nullptr);

        // No per-frame or one-shot debug prints here; keep logs quiet in production.
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to update ray query descriptor sets: " << e.what() << std::endl;
        return false;
    }
}
