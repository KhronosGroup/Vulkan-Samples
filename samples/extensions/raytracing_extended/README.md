### Extended hardware accelerated ray tracing<br/>
**Extensions**: [```VK_KHR_ray_tracing_pipeline```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_ray_tracing_pipeline), [```VK_KHR_acceleration_structure```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_acceleration_structure)<br/>
Render Sponza with Ambient Occlusion.  Place a vase in center.  Generate a particle fire that 
demonstrates the TLAS (Top Level Acceleration Structure) animation for the same underlying geometry.
Procedurally generate a transparent quad and deform the geometry of the quad in the BLAS (Bottom Level Acceleration 
Structure) to demonstrate how to animate with deforming geometry.
Shows how to rebuild the acceleration structure and when to set it to fast rebuild vs fast traversal.

- 🎓 [Ray-tracing: Extended features and dynamic objects](./tutorial.md)
