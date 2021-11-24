### Sub passes<br/>
Vulkan introduces the concept of _subpasses_ to subdivide a single [render pass](../../performance/render_passes/render_passes_tutorial.md) into separate logical phases. The benefit of using subpasses over multiple render passes is that a GPU is able to perform various optimizations. Tile-based renderers, for example, can take advantage of tile memory, which being on chip is decisively faster than external memory, potentially saving a considerable amount of bandwidth.

  - 🎓 [Benefits of subpasses over multiple render passes, use of transient attachments, and G-buffer recommended size](./subpasses_tutorial.md)
