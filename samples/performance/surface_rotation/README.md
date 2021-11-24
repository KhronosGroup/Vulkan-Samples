### Surface rotation<br/>
Mobile devices can be rotated, therefore the logical orientation of the application window and the physical orientation of the display may not match. Applications then need to be able to operate in two modes: portrait and landscape. The difference between these two modes can be simplified to just a change in resolution. However, some display subsystems always work on the "native" (or "physical") orientation of the display panel. Since the device has been rotated, to achieve the desired effect the application output must also rotate. In this sample we focus on the rotation step, and analyze the performance implications of implementing it correctly with Vulkan.

- 🎓 [Appropriate use of surface rotation](./surface_rotation_tutorial.md)
