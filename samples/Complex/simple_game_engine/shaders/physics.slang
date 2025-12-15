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
// Compute shader for physics simulation
// This shader processes rigid body physics data to simulate physical interactions


// Physics data structure
struct PhysicsData {
    float4 position;        // xyz = position, w = inverse mass
    float4 rotation;        // quaternion
    float4 linearVelocity;  // xyz = velocity, w = restitution
    float4 angularVelocity; // xyz = angular velocity, w = friction
    float4 force;           // xyz = force, w = is kinematic (0 or 1)
    float4 torque;          // xyz = torque, w = use gravity (0 or 1)
    float4 colliderData;    // type-specific data (e.g., radius for spheres)
    float4 colliderData2;   // additional collider data (e.g., box half extents)
};

// Collision data structure
struct CollisionData {
    uint bodyA;
    uint bodyB;
    float4 contactNormal;   // xyz = normal, w = penetration depth
    float4 contactPoint;    // xyz = contact point, w = unused
};

// Input/output buffer bindings
[[vk::binding(0, 0)]] RWStructuredBuffer<PhysicsData> physicsBuffer;  // Physics data
[[vk::binding(1, 0)]] RWStructuredBuffer<CollisionData> collisionBuffer; // Collision data
[[vk::binding(2, 0)]] RWStructuredBuffer<uint2> pairBuffer; // Potential collision pairs
[[vk::binding(3, 0)]] RWStructuredBuffer<uint> counterBuffer; // [0] = pair count, [1] = collision count

// Parameters for physics simulation
[[vk::binding(4, 0)]] ConstantBuffer<PhysicsParams> params;

struct PhysicsParams {
    float deltaTime;        // Time step - 4 bytes
    uint numBodies;         // Number of rigid bodies - 4 bytes
    uint maxCollisions;     // Maximum number of collisions - 4 bytes
    float padding;          // Explicit padding to align gravity to 16-byte boundary - 4 bytes
    float4 gravity;         // Gravity vector (xyz) + padding (w) - 16 bytes
    // Total: 32 bytes (aligned to 16-byte boundaries for std140 layout)
};

// Quaternion multiplication
float4 quatMul(float4 q1, float4 q2) {
    return float4(
        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
        q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
        q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
        q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
    );
}

// Quaternion normalization
float4 quatNormalize(float4 q) {
    float len = length(q);
    if (len > 0.0001) {
        return q / len;
    }
    return float4(0, 0, 0, 1);
}

// Integration shader - updates positions and velocities
[shader("compute")]
[numthreads(64, 1, 1)]
void IntegrateCS(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint index = dispatchThreadID.x;

    // Check if this thread is within the number of bodies
    if (index >= params.numBodies) {
        return;
    }

    // Get physics data for this body
    PhysicsData body = physicsBuffer[index];


    // Skip kinematic bodies
    if (body.force.w > 0.5) {
        return;
    }

    // Apply gravity if enabled
    if (body.torque.w > 0.5) {
        float3 gravityForce = params.gravity.xyz * body.position.w;
        body.force.xyz += gravityForce;

    }

    // Integrate forces
    float3 velocityChange = body.force.xyz * body.position.w * params.deltaTime;
    body.linearVelocity.xyz += velocityChange;
    body.angularVelocity.xyz += body.torque.xyz * params.deltaTime; // Simplified, should use inertia tensor


    // Apply damping
    const float linearDamping = 0.01;
    const float angularDamping = 0.01;
    body.linearVelocity.xyz *= (1.0 - linearDamping);
    body.angularVelocity.xyz *= (1.0 - angularDamping);

    // Integrate velocities
    float3 positionChange = body.linearVelocity.xyz * params.deltaTime;
    body.position.xyz += positionChange;


    // Update rotation
    float4 angularVelocityQuat = float4(body.angularVelocity.xyz * 0.5, 0.0);
    float4 rotationDelta = quatMul(angularVelocityQuat, body.rotation);
    body.rotation = quatNormalize(body.rotation + rotationDelta * params.deltaTime);

    // Write updated data back to buffer
    physicsBuffer[index] = body;

}

// Compute AABB for a body
void computeAABB(PhysicsData body, out float3 min, out float3 max) {
    // Default to a small AABB
    min = body.position.xyz - float3(0.1, 0.1, 0.1);
    max = body.position.xyz + float3(0.1, 0.1, 0.1);

    // Check collider type
    int colliderType = int(body.colliderData.w);

    if (colliderType == 0) { // Sphere
        float radius = body.colliderData.x;
        float3 center = body.position.xyz + body.colliderData2.xyz;
        min = center - float3(radius, radius, radius);
        max = center + float3(radius, radius, radius);
    }
    else if (colliderType == 1) { // Box
        float3 halfExtents = body.colliderData.xyz;
        float3 center = body.position.xyz + body.colliderData2.xyz;
        // This is simplified - should account for rotation
        min = center - halfExtents;
        max = center + halfExtents;
    }
    else if (colliderType == 2) { // Mesh (represented as large bounding box)
        float3 halfExtents = body.colliderData.xyz;
        float3 center = body.position.xyz + body.colliderData2.xyz;
        // This is simplified - should account for rotation
        min = center - halfExtents;
        max = center + halfExtents;
    }
}

// Check if two AABBs overlap
bool aabbOverlap(float3 minA, float3 maxA, float3 minB, float3 maxB) {
    return all(minA < maxB) && all(minB < maxA);
}

// Broad phase collision detection - identifies potential collision pairs
[shader("compute")]
[numthreads(64, 1, 1)]
void BroadPhaseCS(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint index = dispatchThreadID.x;

    // Calculate total number of pairs
    uint numPairs = (params.numBodies * (params.numBodies - 1)) / 2;

    if (index >= numPairs) {
        return;
    }

    // Convert linear index to pair indices (i, j) where i < j
    // Use a more robust algorithm that avoids floating-point precision issues
    uint i = 0;
    uint j = 0;

    // Find i and j using integer arithmetic to avoid precision errors
    uint remaining = index;
    uint currentRow = 0;

    // Find which row (i value) this index belongs to
    while (remaining >= (params.numBodies - 1 - currentRow)) {
        remaining -= (params.numBodies - 1 - currentRow);
        currentRow++;
    }

    i = currentRow;
    j = i + 1 + remaining;

    // Get physics data for both bodies
    PhysicsData bodyA = physicsBuffer[i];
    PhysicsData bodyB = physicsBuffer[j];


    // Skip if both bodies are kinematic
    if (bodyA.force.w > 0.5 && bodyB.force.w > 0.5) {
        return;
    }

    // Skip if either body doesn't have a collider
    if (bodyA.colliderData.w < 0 || bodyB.colliderData.w < 0) {
        return;
    }

    // Early culling: only consider pairs where at least one body is a sphere (shape 0)
    int shapeA = int(bodyA.colliderData.w);
    int shapeB = int(bodyB.colliderData.w);
    if (!(shapeA == 0 || shapeB == 0)) {
        return;
    }

    // Compute AABBs
    float3 minA, maxA, minB, maxB;
    computeAABB(bodyA, minA, maxA);
    computeAABB(bodyB, minB, maxB);

    // Expand sphere AABBs by motion over the timestep to catch fast-moving spheres
    if (shapeA == 0) {
        float3 expandA = abs(bodyA.linearVelocity.xyz) * params.deltaTime;
        minA -= expandA; maxA += expandA;
    }
    if (shapeB == 0) {
        float3 expandB = abs(bodyB.linearVelocity.xyz) * params.deltaTime;
        minB -= expandB; maxB += expandB;
    }

    // Check for AABB overlap
    if (aabbOverlap(minA, maxA, minB, maxB)) {
        // Add to potential collision pairs
        uint pairIndex;
        InterlockedAdd(counterBuffer[0], 1, pairIndex);

        if (pairIndex < params.maxCollisions) {
            pairBuffer[pairIndex] = uint2(i, j);
        }
    }
}

// Narrow phase collision detection - detailed collision detection for potential pairs
[shader("compute")]
[numthreads(64, 1, 1)]
void NarrowPhaseCS(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint index = dispatchThreadID.x;

    // Check if this thread is within the number of potential pairs
    uint numPairs = counterBuffer[0];
    if (index >= numPairs || index >= params.maxCollisions) {
        return;
    }

    // Get the pair of bodies
    uint2 pair = pairBuffer[index];
    uint bodyIndexA = pair.x;
    uint bodyIndexB = pair.y;

    PhysicsData bodyA = physicsBuffer[bodyIndexA];
    PhysicsData bodyB = physicsBuffer[bodyIndexB];

    // Determine collision shapes
    int shapeA = int(bodyA.colliderData.w);
    int shapeB = int(bodyB.colliderData.w);

    // Handle sphere-sphere collisions
    if (shapeA == 0 && shapeB == 0) { // Both are spheres
        float radiusA = bodyA.colliderData.x;
        float radiusB = bodyB.colliderData.x;

        float3 posA = bodyA.position.xyz + bodyA.colliderData2.xyz;
        float3 posB = bodyB.position.xyz + bodyB.colliderData2.xyz;

        float3 direction = posB - posA;
        float distance = length(direction);
        float minDistance = radiusA + radiusB;

        if (distance < minDistance) {
            // Collision detected
            uint collisionIndex;
            InterlockedAdd(counterBuffer[1], 1, collisionIndex);

            if (collisionIndex < params.maxCollisions) {
                // Normalize direction
                float3 normal = direction / max(distance, 0.0001);

                // Create collision data
                CollisionData collision;
                collision.bodyA = bodyIndexA;
                collision.bodyB = bodyIndexB;
                collision.contactNormal = float4(normal, minDistance - distance); // penetration depth
                collision.contactPoint = float4(posA + normal * radiusA, 0);

                // Store collision data
                collisionBuffer[collisionIndex] = collision;
            }
        }
    }
    // Handle sphere-geometry collisions (sphere vs mesh represented as box)
    else if ((shapeA == 0 && shapeB == 2) || (shapeA == 2 && shapeB == 0)) {
        // Determine which is sphere and which is geometry
        PhysicsData sphere = (shapeA == 0) ? bodyA : bodyB;
        PhysicsData geometry = (shapeA == 0) ? bodyB : bodyA;
        uint sphereIndex = (shapeA == 0) ? bodyIndexA : bodyIndexB;
        uint geometryIndex = (shapeA == 0) ? bodyIndexB : bodyIndexA;

        float sphereRadius = sphere.colliderData.x;
        float3 spherePos = sphere.position.xyz + sphere.colliderData2.xyz;
        float3 geometryPos = geometry.position.xyz + geometry.colliderData2.xyz;
        float3 geometryHalfExtents = geometry.colliderData.xyz;

        // Simple sphere-box collision detection
        float3 closestPoint = clamp(spherePos, geometryPos - geometryHalfExtents, geometryPos + geometryHalfExtents);
        float3 direction = spherePos - closestPoint;
        float distance = length(direction);

        if (distance < sphereRadius) {
            // Collision detected (overlap)
            uint collisionIndex;
            InterlockedAdd(counterBuffer[1], 1, collisionIndex);

            if (collisionIndex < params.maxCollisions) {
                // Calculate normal so that it points from sphere(A) to geometry(B)
                float3 normal = (distance > 0.0001) ? (-direction / distance) : float3(0, -1, 0);
                float penetration = sphereRadius - distance;

                // Create collision data
                CollisionData collision;
                collision.bodyA = sphereIndex;
                collision.bodyB = geometryIndex;
                collision.contactNormal = float4(normal, penetration);
                collision.contactPoint = float4(closestPoint, 0);

                // Store collision data
                collisionBuffer[collisionIndex] = collision;
            }
        } else {
            // Swept test (CCD-lite): segment from previous position to current against box expanded by sphere radius
            float3 prevPos = spherePos - sphere.linearVelocity.xyz * params.deltaTime;
            float3 dir = spherePos - prevPos;
            float dirLen = length(dir);
            if (dirLen > 1e-6) {
                float3 bbMin = geometryPos - (geometryHalfExtents + sphereRadius);
                float3 bbMax = geometryPos + (geometryHalfExtents + sphereRadius);

                float3 invDir = 1.0 / max(abs(dir), float3(1e-6, 1e-6, 1e-6));
                float3 t0 = (bbMin - prevPos) / dir;
                float3 t1 = (bbMax - prevPos) / dir;
                float3 tmin3 = min(t0, t1);
                float3 tmax3 = max(t0, t1);
                float tEnter = max(tmin3.x, max(tmin3.y, tmin3.z));
                float tExit  = min(tmax3.x,  min(tmax3.y,  tmax3.z));

                if (tEnter >= 0.0 && tEnter <= 1.0 && tEnter <= tExit) {
                    // Determine contact normal based on entry axis and direction of motion
                    float3 normal = float3(0,0,0);
                    if (tEnter >= tmin3.x && tEnter >= tmin3.y && tEnter >= tmin3.z) {
                        normal = float3((dir.x > 0.0) ? 1.0 : -1.0, 0.0, 0.0);
                    } else if (tEnter >= tmin3.y && tEnter >= tmin3.z) {
                        normal = float3(0.0, (dir.y > 0.0) ? 1.0 : -1.0, 0.0);
                    } else {
                        normal = float3(0.0, 0.0, (dir.z > 0.0) ? 1.0 : -1.0);
                    }

                    float3 hitPoint = prevPos + dir * tEnter;

                    uint collisionIndex;
                    InterlockedAdd(counterBuffer[1], 1, collisionIndex);
                    if (collisionIndex < params.maxCollisions) {
                        CollisionData collision;
                        collision.bodyA = sphereIndex;
                        collision.bodyB = geometryIndex;
                        // Tiny penetration to trigger resolution without large positional correction
                        collision.contactNormal = float4(normalize(normal), 0.0);
                        collision.contactPoint = float4(hitPoint, 0.0);
                        collisionBuffer[collisionIndex] = collision;
                    }
                }
            }
        }
    }
}

// Collision resolution - resolves detected collisions
[shader("compute")]
[numthreads(64, 1, 1)]
void ResolveCS(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint index = dispatchThreadID.x;

    // Check if this thread is within the number of collisions
    uint numCollisions = counterBuffer[1];
    if (index >= numCollisions || index >= params.maxCollisions) {
        return;
    }

    // Get collision data
    CollisionData collision = collisionBuffer[index];

    // Get the bodies involved in the collision
    PhysicsData bodyA = physicsBuffer[collision.bodyA];
    PhysicsData bodyB = physicsBuffer[collision.bodyB];

    // Skip if both bodies are kinematic
    if (bodyA.force.w > 0.5 && bodyB.force.w > 0.5) {
        return;
    }

    // Calculate relative velocity
    float3 relativeVelocity = bodyB.linearVelocity.xyz - bodyA.linearVelocity.xyz;

    // Calculate velocity along normal
    float velocityAlongNormal = dot(relativeVelocity, collision.contactNormal.xyz);

    // Don't resolve if velocities are separating
    if (velocityAlongNormal > 0) {
        return;
    }

    // Calculate restitution (bounciness)
    float restitution = min(bodyA.linearVelocity.w, bodyB.linearVelocity.w);

    // Calculate impulse scalar
    float j = -(1.0 + restitution) * velocityAlongNormal;
    j /= bodyA.position.w + bodyB.position.w;

    // Apply impulse
    float3 impulse = collision.contactNormal.xyz * j;

    // Update velocities
    if (bodyA.force.w < 0.5) { // If not kinematic
        bodyA.linearVelocity.xyz -= impulse * bodyA.position.w;
        physicsBuffer[collision.bodyA] = bodyA;
    }

    if (bodyB.force.w < 0.5) { // If not kinematic
        bodyB.linearVelocity.xyz += impulse * bodyB.position.w;
        physicsBuffer[collision.bodyB] = bodyB;
    }

    // Position correction to prevent sinking
    const float percent = 0.2; // usually 20% to 80%
    const float slop = 0.01; // small penetration allowed
    float3 correction = max(collision.contactNormal.w - slop, 0.0) * percent * collision.contactNormal.xyz / (bodyA.position.w + bodyB.position.w);

    if (bodyA.force.w < 0.5) { // If not kinematic
        bodyA.position.xyz -= correction * bodyA.position.w;
        physicsBuffer[collision.bodyA] = bodyA;
    }

    if (bodyB.force.w < 0.5) { // If not kinematic
        bodyB.position.xyz += correction * bodyB.position.w;
        physicsBuffer[collision.bodyB] = bodyB;
    }
}
