### Pipeline barriers<br/>
Vulkan gives the application significant control over memory access for resources. Pipeline barriers are particularly convenient for synchronizing memory accesses between render passes. Having barriers is required whenever there is a memory dependency - the application should not assume that render passes are executed in order. However, having too many or too strict barriers can affect the application's performance. This sample will cover how to set up pipeline barriers efficiently, with a focus on pipeline stages.

- 🎓 [Using pipeline barriers efficiently](./pipeline_barriers_tutorial.md)
