/*
 * Copyright (c) 2026, Holochip Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// RUN: %dxc %s -T cs_6_0 -spirv -fspv-target-env=vulkan1.3 -E main -fspv-debug=vulkan-with-source -O3

// Minimal HLSL compute shader meant to exercise non-semantic DebugPrintf and
// emit SPIR-V relaxed extended instruction patterns under DXC.

// A tiny push constant block to pass a dynamic value from the app.
struct PushConstants
{
    uint value;
};
[[vk::push_constant]] PushConstants pc;

class A {
  void foo(uint v) {
    printf("relaxed-ext-inst demo: value = %u", v);
  }
};

[numthreads(1, 1, 1)]
void main(uint3 gid : SV_DispatchThreadID)
{
  A a;
  // Only thread (0,0,0) prints, and prints the app-provided value.
  if (all(gid == uint3(0,0,0)))
  {
    a.foo(pc.value);
  }
}
