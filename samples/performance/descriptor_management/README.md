### Descriptor management<br/>
An application using Vulkan will have to implement a system to manage descriptor pools and sets. The most straightforward and flexible approach is to re-create them for each frame, but doing so might be very inefficient, especially on mobile platforms. The problem of descriptor management is intertwined with that of buffer management, that is choosing how to pack data in `VkBuffer` objects. This sample will explore a few options to improve both descriptor and buffer management.

- 🎓 [Descriptor and buffer management](./descriptor_management_tutorial.md)
