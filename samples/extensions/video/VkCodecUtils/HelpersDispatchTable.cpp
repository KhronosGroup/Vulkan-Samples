// This file is generated.
#include "HelpersDispatchTable.h"

namespace vk {


void InitDispatchTableTop(PFN_vkGetInstanceProcAddr getInstanceProcAddrFunc, VkInterfaceFunctions* pVkFunctions)
{
    pVkFunctions->GetInstanceProcAddr = getInstanceProcAddrFunc;
    pVkFunctions->CreateInstance = reinterpret_cast<PFN_vkCreateInstance>(getInstanceProcAddrFunc(VK_NULL_HANDLE, "vkCreateInstance"));
    pVkFunctions->EnumerateInstanceExtensionProperties = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(getInstanceProcAddrFunc(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties"));
    pVkFunctions->EnumerateInstanceLayerProperties = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(getInstanceProcAddrFunc(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties"));
    pVkFunctions->EnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(getInstanceProcAddrFunc(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
}

void InitDispatchTableMiddle(VkInstance instance, bool include_bottom, VkInterfaceFunctions* pVkFunctions)
{
    PFN_vkGetInstanceProcAddr getInstanceProcAddrFunc = pVkFunctions->GetInstanceProcAddr;
    pVkFunctions->DestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(getInstanceProcAddrFunc(instance, "vkDestroyInstance"));
    pVkFunctions->EnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(getInstanceProcAddrFunc(instance, "vkEnumeratePhysicalDevices"));
    pVkFunctions->GetPhysicalDeviceFeatures = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceFeatures"));
    pVkFunctions->GetPhysicalDeviceFormatProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceFormatProperties"));
    pVkFunctions->GetPhysicalDeviceImageFormatProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceImageFormatProperties>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceImageFormatProperties"));
    pVkFunctions->GetPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceProperties"));
    pVkFunctions->GetPhysicalDeviceQueueFamilyProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    pVkFunctions->GetPhysicalDeviceMemoryProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceMemoryProperties"));
    pVkFunctions->CreateDevice = reinterpret_cast<PFN_vkCreateDevice>(getInstanceProcAddrFunc(instance, "vkCreateDevice"));
    pVkFunctions->EnumerateDeviceExtensionProperties = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(getInstanceProcAddrFunc(instance, "vkEnumerateDeviceExtensionProperties"));
    pVkFunctions->GetPhysicalDeviceSparseImageFormatProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceSparseImageFormatProperties>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceSparseImageFormatProperties"));
    pVkFunctions->EnumeratePhysicalDeviceGroups = reinterpret_cast<PFN_vkEnumeratePhysicalDeviceGroups>(getInstanceProcAddrFunc(instance, "vkEnumeratePhysicalDeviceGroups"));
    pVkFunctions->GetPhysicalDeviceFeatures2 = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceFeatures2"));
    pVkFunctions->GetPhysicalDeviceProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceProperties2"));
    pVkFunctions->GetPhysicalDeviceFormatProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties2>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceFormatProperties2"));
    pVkFunctions->GetPhysicalDeviceImageFormatProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceImageFormatProperties2>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceImageFormatProperties2"));
    pVkFunctions->GetPhysicalDeviceQueueFamilyProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties2>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceQueueFamilyProperties2"));
    pVkFunctions->GetPhysicalDeviceMemoryProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties2>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceMemoryProperties2"));
    pVkFunctions->GetPhysicalDeviceSparseImageFormatProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceSparseImageFormatProperties2>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceSparseImageFormatProperties2"));
    pVkFunctions->GetPhysicalDeviceExternalBufferProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceExternalBufferProperties>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceExternalBufferProperties"));
    pVkFunctions->GetPhysicalDeviceExternalFenceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceExternalFenceProperties>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceExternalFenceProperties"));
    pVkFunctions->GetPhysicalDeviceExternalSemaphoreProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceExternalSemaphoreProperties>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceExternalSemaphoreProperties"));
    pVkFunctions->DestroySurfaceKHR = reinterpret_cast<PFN_vkDestroySurfaceKHR>(getInstanceProcAddrFunc(instance, "vkDestroySurfaceKHR"));
    pVkFunctions->GetPhysicalDeviceSurfaceSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
    pVkFunctions->GetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    pVkFunctions->GetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
    pVkFunctions->GetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    pVkFunctions->GetPhysicalDeviceDisplayPropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceDisplayPropertiesKHR>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceDisplayPropertiesKHR"));
    pVkFunctions->GetPhysicalDeviceDisplayPlanePropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR"));
    pVkFunctions->GetDisplayPlaneSupportedDisplaysKHR = reinterpret_cast<PFN_vkGetDisplayPlaneSupportedDisplaysKHR>(getInstanceProcAddrFunc(instance, "vkGetDisplayPlaneSupportedDisplaysKHR"));
    pVkFunctions->GetDisplayModePropertiesKHR = reinterpret_cast<PFN_vkGetDisplayModePropertiesKHR>(getInstanceProcAddrFunc(instance, "vkGetDisplayModePropertiesKHR"));
    pVkFunctions->CreateDisplayModeKHR = reinterpret_cast<PFN_vkCreateDisplayModeKHR>(getInstanceProcAddrFunc(instance, "vkCreateDisplayModeKHR"));
    pVkFunctions->GetDisplayPlaneCapabilitiesKHR = reinterpret_cast<PFN_vkGetDisplayPlaneCapabilitiesKHR>(getInstanceProcAddrFunc(instance, "vkGetDisplayPlaneCapabilitiesKHR"));
    pVkFunctions->CreateDisplayPlaneSurfaceKHR = reinterpret_cast<PFN_vkCreateDisplayPlaneSurfaceKHR>(getInstanceProcAddrFunc(instance, "vkCreateDisplayPlaneSurfaceKHR"));
#ifdef VK_USE_PLATFORM_XLIB_KHR
    pVkFunctions->CreateXlibSurfaceKHR = reinterpret_cast<PFN_vkCreateXlibSurfaceKHR>(getInstanceProcAddrFunc(instance, "vkCreateXlibSurfaceKHR"));
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    pVkFunctions->GetPhysicalDeviceXlibPresentationSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR"));
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
    pVkFunctions->CreateXcbSurfaceKHR = reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(getInstanceProcAddrFunc(instance, "vkCreateXcbSurfaceKHR"));
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
    pVkFunctions->GetPhysicalDeviceXcbPresentationSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR"));
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    pVkFunctions->CreateWaylandSurfaceKHR = reinterpret_cast<PFN_vkCreateWaylandSurfaceKHR>(getInstanceProcAddrFunc(instance, "vkCreateWaylandSurfaceKHR"));
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    pVkFunctions->GetPhysicalDeviceWaylandPresentationSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR"));
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
    pVkFunctions->CreateMirSurfaceKHR = reinterpret_cast<PFN_vkCreateMirSurfaceKHR>(getInstanceProcAddrFunc(instance, "vkCreateMirSurfaceKHR"));
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
    pVkFunctions->GetPhysicalDeviceMirPresentationSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceMirPresentationSupportKHR>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceMirPresentationSupportKHR"));
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    pVkFunctions->CreateAndroidSurfaceKHR = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(getInstanceProcAddrFunc(instance, "vkCreateAndroidSurfaceKHR"));
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    pVkFunctions->CreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(getInstanceProcAddrFunc(instance, "vkCreateWin32SurfaceKHR"));
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    pVkFunctions->GetPhysicalDeviceWin32PresentationSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR"));
#endif
    pVkFunctions->CreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(getInstanceProcAddrFunc(instance, "vkCreateDebugReportCallbackEXT"));
    pVkFunctions->DestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(getInstanceProcAddrFunc(instance, "vkDestroyDebugReportCallbackEXT"));
    pVkFunctions->DebugReportMessageEXT = reinterpret_cast<PFN_vkDebugReportMessageEXT>(getInstanceProcAddrFunc(instance, "vkDebugReportMessageEXT"));
#ifdef VK_USE_PLATFORM_IOS_MVK
    pVkFunctions->CreateIOSSurfaceMVK = reinterpret_cast<PFN_vkCreateIOSSurfaceMVK>(getInstanceProcAddrFunc(instance, "vkCreateIOSSurfaceMVK"));
#endif
#ifdef VK_USE_PLATFORM_MACOS_MVK
    pVkFunctions->CreateMacOSSurfaceMVK = reinterpret_cast<PFN_vkCreateMacOSSurfaceMVK>(getInstanceProcAddrFunc(instance, "vkCreateMacOSSurfaceMVK"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->GetPhysicalDeviceVideoFormatPropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceVideoFormatPropertiesKHR>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceVideoFormatPropertiesKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->GetPhysicalDeviceVideoCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceVideoCapabilitiesKHR>(getInstanceProcAddrFunc(instance, "vkGetPhysicalDeviceVideoCapabilitiesKHR"));
#endif

    if (!include_bottom)
        return;

    pVkFunctions->GetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(getInstanceProcAddrFunc(instance, "vkGetDeviceProcAddr"));
    pVkFunctions->DestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(getInstanceProcAddrFunc(instance, "vkDestroyDevice"));
    pVkFunctions->GetDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(getInstanceProcAddrFunc(instance, "vkGetDeviceQueue"));
    pVkFunctions->QueueSubmit = reinterpret_cast<PFN_vkQueueSubmit>(getInstanceProcAddrFunc(instance, "vkQueueSubmit"));
    pVkFunctions->QueueWaitIdle = reinterpret_cast<PFN_vkQueueWaitIdle>(getInstanceProcAddrFunc(instance, "vkQueueWaitIdle"));
    pVkFunctions->DeviceWaitIdle = reinterpret_cast<PFN_vkDeviceWaitIdle>(getInstanceProcAddrFunc(instance, "vkDeviceWaitIdle"));
    pVkFunctions->AllocateMemory = reinterpret_cast<PFN_vkAllocateMemory>(getInstanceProcAddrFunc(instance, "vkAllocateMemory"));
    pVkFunctions->FreeMemory = reinterpret_cast<PFN_vkFreeMemory>(getInstanceProcAddrFunc(instance, "vkFreeMemory"));
    pVkFunctions->MapMemory = reinterpret_cast<PFN_vkMapMemory>(getInstanceProcAddrFunc(instance, "vkMapMemory"));
    pVkFunctions->UnmapMemory = reinterpret_cast<PFN_vkUnmapMemory>(getInstanceProcAddrFunc(instance, "vkUnmapMemory"));
    pVkFunctions->FlushMappedMemoryRanges = reinterpret_cast<PFN_vkFlushMappedMemoryRanges>(getInstanceProcAddrFunc(instance, "vkFlushMappedMemoryRanges"));
    pVkFunctions->InvalidateMappedMemoryRanges = reinterpret_cast<PFN_vkInvalidateMappedMemoryRanges>(getInstanceProcAddrFunc(instance, "vkInvalidateMappedMemoryRanges"));
    pVkFunctions->GetDeviceMemoryCommitment = reinterpret_cast<PFN_vkGetDeviceMemoryCommitment>(getInstanceProcAddrFunc(instance, "vkGetDeviceMemoryCommitment"));
    pVkFunctions->BindBufferMemory = reinterpret_cast<PFN_vkBindBufferMemory>(getInstanceProcAddrFunc(instance, "vkBindBufferMemory"));
    pVkFunctions->BindImageMemory = reinterpret_cast<PFN_vkBindImageMemory>(getInstanceProcAddrFunc(instance, "vkBindImageMemory"));
    pVkFunctions->GetBufferMemoryRequirements = reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(getInstanceProcAddrFunc(instance, "vkGetBufferMemoryRequirements"));
    pVkFunctions->GetImageMemoryRequirements = reinterpret_cast<PFN_vkGetImageMemoryRequirements>(getInstanceProcAddrFunc(instance, "vkGetImageMemoryRequirements"));
    pVkFunctions->GetImageSparseMemoryRequirements = reinterpret_cast<PFN_vkGetImageSparseMemoryRequirements>(getInstanceProcAddrFunc(instance, "vkGetImageSparseMemoryRequirements"));
    pVkFunctions->QueueBindSparse = reinterpret_cast<PFN_vkQueueBindSparse>(getInstanceProcAddrFunc(instance, "vkQueueBindSparse"));
    pVkFunctions->CreateFence = reinterpret_cast<PFN_vkCreateFence>(getInstanceProcAddrFunc(instance, "vkCreateFence"));
    pVkFunctions->DestroyFence = reinterpret_cast<PFN_vkDestroyFence>(getInstanceProcAddrFunc(instance, "vkDestroyFence"));
    pVkFunctions->ResetFences = reinterpret_cast<PFN_vkResetFences>(getInstanceProcAddrFunc(instance, "vkResetFences"));
    pVkFunctions->GetFenceStatus = reinterpret_cast<PFN_vkGetFenceStatus>(getInstanceProcAddrFunc(instance, "vkGetFenceStatus"));
    pVkFunctions->WaitForFences = reinterpret_cast<PFN_vkWaitForFences>(getInstanceProcAddrFunc(instance, "vkWaitForFences"));
    pVkFunctions->CreateSemaphore = reinterpret_cast<PFN_vkCreateSemaphore>(getInstanceProcAddrFunc(instance, "vkCreateSemaphore"));
    pVkFunctions->DestroySemaphore = reinterpret_cast<PFN_vkDestroySemaphore>(getInstanceProcAddrFunc(instance, "vkDestroySemaphore"));
    pVkFunctions->GetSemaphoreCounterValue = reinterpret_cast<PFN_vkGetSemaphoreCounterValue>(getInstanceProcAddrFunc(instance, "vkGetSemaphoreCounterValue"));
    pVkFunctions->WaitSemaphores = reinterpret_cast<PFN_vkWaitSemaphores>(getInstanceProcAddrFunc(instance, "vkWaitSemaphores"));
    pVkFunctions->SignalSemaphore = reinterpret_cast<PFN_vkSignalSemaphore>(getInstanceProcAddrFunc(instance, "vkSignalSemaphore"));
    pVkFunctions->CreateEvent = reinterpret_cast<PFN_vkCreateEvent>(getInstanceProcAddrFunc(instance, "vkCreateEvent"));
    pVkFunctions->DestroyEvent = reinterpret_cast<PFN_vkDestroyEvent>(getInstanceProcAddrFunc(instance, "vkDestroyEvent"));
    pVkFunctions->GetEventStatus = reinterpret_cast<PFN_vkGetEventStatus>(getInstanceProcAddrFunc(instance, "vkGetEventStatus"));
    pVkFunctions->SetEvent = reinterpret_cast<PFN_vkSetEvent>(getInstanceProcAddrFunc(instance, "vkSetEvent"));
    pVkFunctions->ResetEvent = reinterpret_cast<PFN_vkResetEvent>(getInstanceProcAddrFunc(instance, "vkResetEvent"));
    pVkFunctions->CreateQueryPool = reinterpret_cast<PFN_vkCreateQueryPool>(getInstanceProcAddrFunc(instance, "vkCreateQueryPool"));
    pVkFunctions->DestroyQueryPool = reinterpret_cast<PFN_vkDestroyQueryPool>(getInstanceProcAddrFunc(instance, "vkDestroyQueryPool"));
    pVkFunctions->GetQueryPoolResults = reinterpret_cast<PFN_vkGetQueryPoolResults>(getInstanceProcAddrFunc(instance, "vkGetQueryPoolResults"));
    pVkFunctions->CreateBuffer = reinterpret_cast<PFN_vkCreateBuffer>(getInstanceProcAddrFunc(instance, "vkCreateBuffer"));
    pVkFunctions->DestroyBuffer = reinterpret_cast<PFN_vkDestroyBuffer>(getInstanceProcAddrFunc(instance, "vkDestroyBuffer"));
    pVkFunctions->CreateBufferView = reinterpret_cast<PFN_vkCreateBufferView>(getInstanceProcAddrFunc(instance, "vkCreateBufferView"));
    pVkFunctions->DestroyBufferView = reinterpret_cast<PFN_vkDestroyBufferView>(getInstanceProcAddrFunc(instance, "vkDestroyBufferView"));
    pVkFunctions->CreateImage = reinterpret_cast<PFN_vkCreateImage>(getInstanceProcAddrFunc(instance, "vkCreateImage"));
    pVkFunctions->DestroyImage = reinterpret_cast<PFN_vkDestroyImage>(getInstanceProcAddrFunc(instance, "vkDestroyImage"));
    pVkFunctions->GetImageSubresourceLayout = reinterpret_cast<PFN_vkGetImageSubresourceLayout>(getInstanceProcAddrFunc(instance, "vkGetImageSubresourceLayout"));
    pVkFunctions->CreateImageView = reinterpret_cast<PFN_vkCreateImageView>(getInstanceProcAddrFunc(instance, "vkCreateImageView"));
    pVkFunctions->DestroyImageView = reinterpret_cast<PFN_vkDestroyImageView>(getInstanceProcAddrFunc(instance, "vkDestroyImageView"));
    pVkFunctions->CreateShaderModule = reinterpret_cast<PFN_vkCreateShaderModule>(getInstanceProcAddrFunc(instance, "vkCreateShaderModule"));
    pVkFunctions->DestroyShaderModule = reinterpret_cast<PFN_vkDestroyShaderModule>(getInstanceProcAddrFunc(instance, "vkDestroyShaderModule"));
    pVkFunctions->CreatePipelineCache = reinterpret_cast<PFN_vkCreatePipelineCache>(getInstanceProcAddrFunc(instance, "vkCreatePipelineCache"));
    pVkFunctions->DestroyPipelineCache = reinterpret_cast<PFN_vkDestroyPipelineCache>(getInstanceProcAddrFunc(instance, "vkDestroyPipelineCache"));
    pVkFunctions->GetPipelineCacheData = reinterpret_cast<PFN_vkGetPipelineCacheData>(getInstanceProcAddrFunc(instance, "vkGetPipelineCacheData"));
    pVkFunctions->MergePipelineCaches = reinterpret_cast<PFN_vkMergePipelineCaches>(getInstanceProcAddrFunc(instance, "vkMergePipelineCaches"));
    pVkFunctions->CreateGraphicsPipelines = reinterpret_cast<PFN_vkCreateGraphicsPipelines>(getInstanceProcAddrFunc(instance, "vkCreateGraphicsPipelines"));
    pVkFunctions->CreateComputePipelines = reinterpret_cast<PFN_vkCreateComputePipelines>(getInstanceProcAddrFunc(instance, "vkCreateComputePipelines"));
    pVkFunctions->DestroyPipeline = reinterpret_cast<PFN_vkDestroyPipeline>(getInstanceProcAddrFunc(instance, "vkDestroyPipeline"));
    pVkFunctions->CreatePipelineLayout = reinterpret_cast<PFN_vkCreatePipelineLayout>(getInstanceProcAddrFunc(instance, "vkCreatePipelineLayout"));
    pVkFunctions->DestroyPipelineLayout = reinterpret_cast<PFN_vkDestroyPipelineLayout>(getInstanceProcAddrFunc(instance, "vkDestroyPipelineLayout"));
    pVkFunctions->CreateSampler = reinterpret_cast<PFN_vkCreateSampler>(getInstanceProcAddrFunc(instance, "vkCreateSampler"));
    pVkFunctions->DestroySampler = reinterpret_cast<PFN_vkDestroySampler>(getInstanceProcAddrFunc(instance, "vkDestroySampler"));
    pVkFunctions->CreateDescriptorSetLayout = reinterpret_cast<PFN_vkCreateDescriptorSetLayout>(getInstanceProcAddrFunc(instance, "vkCreateDescriptorSetLayout"));
    pVkFunctions->DestroyDescriptorSetLayout = reinterpret_cast<PFN_vkDestroyDescriptorSetLayout>(getInstanceProcAddrFunc(instance, "vkDestroyDescriptorSetLayout"));
    pVkFunctions->CreateDescriptorPool = reinterpret_cast<PFN_vkCreateDescriptorPool>(getInstanceProcAddrFunc(instance, "vkCreateDescriptorPool"));
    pVkFunctions->DestroyDescriptorPool = reinterpret_cast<PFN_vkDestroyDescriptorPool>(getInstanceProcAddrFunc(instance, "vkDestroyDescriptorPool"));
    pVkFunctions->ResetDescriptorPool = reinterpret_cast<PFN_vkResetDescriptorPool>(getInstanceProcAddrFunc(instance, "vkResetDescriptorPool"));
    pVkFunctions->AllocateDescriptorSets = reinterpret_cast<PFN_vkAllocateDescriptorSets>(getInstanceProcAddrFunc(instance, "vkAllocateDescriptorSets"));
    pVkFunctions->FreeDescriptorSets = reinterpret_cast<PFN_vkFreeDescriptorSets>(getInstanceProcAddrFunc(instance, "vkFreeDescriptorSets"));
    pVkFunctions->UpdateDescriptorSets = reinterpret_cast<PFN_vkUpdateDescriptorSets>(getInstanceProcAddrFunc(instance, "vkUpdateDescriptorSets"));
    pVkFunctions->CreateFramebuffer = reinterpret_cast<PFN_vkCreateFramebuffer>(getInstanceProcAddrFunc(instance, "vkCreateFramebuffer"));
    pVkFunctions->DestroyFramebuffer = reinterpret_cast<PFN_vkDestroyFramebuffer>(getInstanceProcAddrFunc(instance, "vkDestroyFramebuffer"));
    pVkFunctions->CreateRenderPass = reinterpret_cast<PFN_vkCreateRenderPass>(getInstanceProcAddrFunc(instance, "vkCreateRenderPass"));
    pVkFunctions->DestroyRenderPass = reinterpret_cast<PFN_vkDestroyRenderPass>(getInstanceProcAddrFunc(instance, "vkDestroyRenderPass"));
    pVkFunctions->GetRenderAreaGranularity = reinterpret_cast<PFN_vkGetRenderAreaGranularity>(getInstanceProcAddrFunc(instance, "vkGetRenderAreaGranularity"));
    pVkFunctions->CreateCommandPool = reinterpret_cast<PFN_vkCreateCommandPool>(getInstanceProcAddrFunc(instance, "vkCreateCommandPool"));
    pVkFunctions->DestroyCommandPool = reinterpret_cast<PFN_vkDestroyCommandPool>(getInstanceProcAddrFunc(instance, "vkDestroyCommandPool"));
    pVkFunctions->ResetCommandPool = reinterpret_cast<PFN_vkResetCommandPool>(getInstanceProcAddrFunc(instance, "vkResetCommandPool"));
    pVkFunctions->AllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(getInstanceProcAddrFunc(instance, "vkAllocateCommandBuffers"));
    pVkFunctions->FreeCommandBuffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(getInstanceProcAddrFunc(instance, "vkFreeCommandBuffers"));
    pVkFunctions->BeginCommandBuffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(getInstanceProcAddrFunc(instance, "vkBeginCommandBuffer"));
    pVkFunctions->EndCommandBuffer = reinterpret_cast<PFN_vkEndCommandBuffer>(getInstanceProcAddrFunc(instance, "vkEndCommandBuffer"));
    pVkFunctions->ResetCommandBuffer = reinterpret_cast<PFN_vkResetCommandBuffer>(getInstanceProcAddrFunc(instance, "vkResetCommandBuffer"));
    pVkFunctions->CmdBindPipeline = reinterpret_cast<PFN_vkCmdBindPipeline>(getInstanceProcAddrFunc(instance, "vkCmdBindPipeline"));
    pVkFunctions->CmdSetViewport = reinterpret_cast<PFN_vkCmdSetViewport>(getInstanceProcAddrFunc(instance, "vkCmdSetViewport"));
    pVkFunctions->CmdSetScissor = reinterpret_cast<PFN_vkCmdSetScissor>(getInstanceProcAddrFunc(instance, "vkCmdSetScissor"));
    pVkFunctions->CmdSetLineWidth = reinterpret_cast<PFN_vkCmdSetLineWidth>(getInstanceProcAddrFunc(instance, "vkCmdSetLineWidth"));
    pVkFunctions->CmdSetDepthBias = reinterpret_cast<PFN_vkCmdSetDepthBias>(getInstanceProcAddrFunc(instance, "vkCmdSetDepthBias"));
    pVkFunctions->CmdSetBlendConstants = reinterpret_cast<PFN_vkCmdSetBlendConstants>(getInstanceProcAddrFunc(instance, "vkCmdSetBlendConstants"));
    pVkFunctions->CmdSetDepthBounds = reinterpret_cast<PFN_vkCmdSetDepthBounds>(getInstanceProcAddrFunc(instance, "vkCmdSetDepthBounds"));
    pVkFunctions->CmdSetStencilCompareMask = reinterpret_cast<PFN_vkCmdSetStencilCompareMask>(getInstanceProcAddrFunc(instance, "vkCmdSetStencilCompareMask"));
    pVkFunctions->CmdSetStencilWriteMask = reinterpret_cast<PFN_vkCmdSetStencilWriteMask>(getInstanceProcAddrFunc(instance, "vkCmdSetStencilWriteMask"));
    pVkFunctions->CmdSetStencilReference = reinterpret_cast<PFN_vkCmdSetStencilReference>(getInstanceProcAddrFunc(instance, "vkCmdSetStencilReference"));
    pVkFunctions->CmdBindDescriptorSets = reinterpret_cast<PFN_vkCmdBindDescriptorSets>(getInstanceProcAddrFunc(instance, "vkCmdBindDescriptorSets"));
    pVkFunctions->CmdBindIndexBuffer = reinterpret_cast<PFN_vkCmdBindIndexBuffer>(getInstanceProcAddrFunc(instance, "vkCmdBindIndexBuffer"));
    pVkFunctions->CmdBindVertexBuffers = reinterpret_cast<PFN_vkCmdBindVertexBuffers>(getInstanceProcAddrFunc(instance, "vkCmdBindVertexBuffers"));
    pVkFunctions->CmdDraw = reinterpret_cast<PFN_vkCmdDraw>(getInstanceProcAddrFunc(instance, "vkCmdDraw"));
    pVkFunctions->CmdDrawIndexed = reinterpret_cast<PFN_vkCmdDrawIndexed>(getInstanceProcAddrFunc(instance, "vkCmdDrawIndexed"));
    pVkFunctions->CmdDrawIndirect = reinterpret_cast<PFN_vkCmdDrawIndirect>(getInstanceProcAddrFunc(instance, "vkCmdDrawIndirect"));
    pVkFunctions->CmdDrawIndexedIndirect = reinterpret_cast<PFN_vkCmdDrawIndexedIndirect>(getInstanceProcAddrFunc(instance, "vkCmdDrawIndexedIndirect"));
    pVkFunctions->CmdDispatch = reinterpret_cast<PFN_vkCmdDispatch>(getInstanceProcAddrFunc(instance, "vkCmdDispatch"));
    pVkFunctions->CmdDispatchIndirect = reinterpret_cast<PFN_vkCmdDispatchIndirect>(getInstanceProcAddrFunc(instance, "vkCmdDispatchIndirect"));
    pVkFunctions->CmdCopyBuffer = reinterpret_cast<PFN_vkCmdCopyBuffer>(getInstanceProcAddrFunc(instance, "vkCmdCopyBuffer"));
    pVkFunctions->CmdCopyImage = reinterpret_cast<PFN_vkCmdCopyImage>(getInstanceProcAddrFunc(instance, "vkCmdCopyImage"));
    pVkFunctions->CmdBlitImage = reinterpret_cast<PFN_vkCmdBlitImage>(getInstanceProcAddrFunc(instance, "vkCmdBlitImage"));
    pVkFunctions->CmdCopyBufferToImage = reinterpret_cast<PFN_vkCmdCopyBufferToImage>(getInstanceProcAddrFunc(instance, "vkCmdCopyBufferToImage"));
    pVkFunctions->CmdCopyImageToBuffer = reinterpret_cast<PFN_vkCmdCopyImageToBuffer>(getInstanceProcAddrFunc(instance, "vkCmdCopyImageToBuffer"));
    pVkFunctions->CmdUpdateBuffer = reinterpret_cast<PFN_vkCmdUpdateBuffer>(getInstanceProcAddrFunc(instance, "vkCmdUpdateBuffer"));
    pVkFunctions->CmdFillBuffer = reinterpret_cast<PFN_vkCmdFillBuffer>(getInstanceProcAddrFunc(instance, "vkCmdFillBuffer"));
    pVkFunctions->CmdClearColorImage = reinterpret_cast<PFN_vkCmdClearColorImage>(getInstanceProcAddrFunc(instance, "vkCmdClearColorImage"));
    pVkFunctions->CmdClearDepthStencilImage = reinterpret_cast<PFN_vkCmdClearDepthStencilImage>(getInstanceProcAddrFunc(instance, "vkCmdClearDepthStencilImage"));
    pVkFunctions->CmdClearAttachments = reinterpret_cast<PFN_vkCmdClearAttachments>(getInstanceProcAddrFunc(instance, "vkCmdClearAttachments"));
    pVkFunctions->CmdResolveImage = reinterpret_cast<PFN_vkCmdResolveImage>(getInstanceProcAddrFunc(instance, "vkCmdResolveImage"));
    pVkFunctions->CmdSetEvent = reinterpret_cast<PFN_vkCmdSetEvent>(getInstanceProcAddrFunc(instance, "vkCmdSetEvent"));
    pVkFunctions->CmdResetEvent = reinterpret_cast<PFN_vkCmdResetEvent>(getInstanceProcAddrFunc(instance, "vkCmdResetEvent"));
    pVkFunctions->CmdWaitEvents = reinterpret_cast<PFN_vkCmdWaitEvents>(getInstanceProcAddrFunc(instance, "vkCmdWaitEvents"));
    pVkFunctions->CmdPipelineBarrier = reinterpret_cast<PFN_vkCmdPipelineBarrier>(getInstanceProcAddrFunc(instance, "vkCmdPipelineBarrier"));
    pVkFunctions->CmdBeginQuery = reinterpret_cast<PFN_vkCmdBeginQuery>(getInstanceProcAddrFunc(instance, "vkCmdBeginQuery"));
    pVkFunctions->CmdEndQuery = reinterpret_cast<PFN_vkCmdEndQuery>(getInstanceProcAddrFunc(instance, "vkCmdEndQuery"));
    pVkFunctions->CmdResetQueryPool = reinterpret_cast<PFN_vkCmdResetQueryPool>(getInstanceProcAddrFunc(instance, "vkCmdResetQueryPool"));
    pVkFunctions->CmdWriteTimestamp = reinterpret_cast<PFN_vkCmdWriteTimestamp>(getInstanceProcAddrFunc(instance, "vkCmdWriteTimestamp"));
    pVkFunctions->CmdCopyQueryPoolResults = reinterpret_cast<PFN_vkCmdCopyQueryPoolResults>(getInstanceProcAddrFunc(instance, "vkCmdCopyQueryPoolResults"));
    pVkFunctions->CmdPushConstants = reinterpret_cast<PFN_vkCmdPushConstants>(getInstanceProcAddrFunc(instance, "vkCmdPushConstants"));
    pVkFunctions->CmdBeginRenderPass = reinterpret_cast<PFN_vkCmdBeginRenderPass>(getInstanceProcAddrFunc(instance, "vkCmdBeginRenderPass"));
    pVkFunctions->CmdNextSubpass = reinterpret_cast<PFN_vkCmdNextSubpass>(getInstanceProcAddrFunc(instance, "vkCmdNextSubpass"));
    pVkFunctions->CmdEndRenderPass = reinterpret_cast<PFN_vkCmdEndRenderPass>(getInstanceProcAddrFunc(instance, "vkCmdEndRenderPass"));
    pVkFunctions->CmdExecuteCommands = reinterpret_cast<PFN_vkCmdExecuteCommands>(getInstanceProcAddrFunc(instance, "vkCmdExecuteCommands"));
    pVkFunctions->BindBufferMemory2 = reinterpret_cast<PFN_vkBindBufferMemory2>(getInstanceProcAddrFunc(instance, "vkBindBufferMemory2"));
    pVkFunctions->BindImageMemory2 = reinterpret_cast<PFN_vkBindImageMemory2>(getInstanceProcAddrFunc(instance, "vkBindImageMemory2"));
    pVkFunctions->GetDeviceGroupPeerMemoryFeatures = reinterpret_cast<PFN_vkGetDeviceGroupPeerMemoryFeatures>(getInstanceProcAddrFunc(instance, "vkGetDeviceGroupPeerMemoryFeatures"));
    pVkFunctions->CmdSetDeviceMask = reinterpret_cast<PFN_vkCmdSetDeviceMask>(getInstanceProcAddrFunc(instance, "vkCmdSetDeviceMask"));
    pVkFunctions->CmdDispatchBase = reinterpret_cast<PFN_vkCmdDispatchBase>(getInstanceProcAddrFunc(instance, "vkCmdDispatchBase"));
    pVkFunctions->GetImageMemoryRequirements2 = reinterpret_cast<PFN_vkGetImageMemoryRequirements2>(getInstanceProcAddrFunc(instance, "vkGetImageMemoryRequirements2"));
    pVkFunctions->GetBufferMemoryRequirements2 = reinterpret_cast<PFN_vkGetBufferMemoryRequirements2>(getInstanceProcAddrFunc(instance, "vkGetBufferMemoryRequirements2"));
    pVkFunctions->GetImageSparseMemoryRequirements2 = reinterpret_cast<PFN_vkGetImageSparseMemoryRequirements2>(getInstanceProcAddrFunc(instance, "vkGetImageSparseMemoryRequirements2"));
    pVkFunctions->TrimCommandPool = reinterpret_cast<PFN_vkTrimCommandPool>(getInstanceProcAddrFunc(instance, "vkTrimCommandPool"));
    pVkFunctions->GetDeviceQueue2 = reinterpret_cast<PFN_vkGetDeviceQueue2>(getInstanceProcAddrFunc(instance, "vkGetDeviceQueue2"));
    pVkFunctions->CreateSamplerYcbcrConversion = reinterpret_cast<PFN_vkCreateSamplerYcbcrConversion>(getInstanceProcAddrFunc(instance, "vkCreateSamplerYcbcrConversion"));
    pVkFunctions->DestroySamplerYcbcrConversion = reinterpret_cast<PFN_vkDestroySamplerYcbcrConversion>(getInstanceProcAddrFunc(instance, "vkDestroySamplerYcbcrConversion"));
    pVkFunctions->CreateDescriptorUpdateTemplate = reinterpret_cast<PFN_vkCreateDescriptorUpdateTemplate>(getInstanceProcAddrFunc(instance, "vkCreateDescriptorUpdateTemplate"));
    pVkFunctions->DestroyDescriptorUpdateTemplate = reinterpret_cast<PFN_vkDestroyDescriptorUpdateTemplate>(getInstanceProcAddrFunc(instance, "vkDestroyDescriptorUpdateTemplate"));
    pVkFunctions->UpdateDescriptorSetWithTemplate = reinterpret_cast<PFN_vkUpdateDescriptorSetWithTemplate>(getInstanceProcAddrFunc(instance, "vkUpdateDescriptorSetWithTemplate"));
    pVkFunctions->GetDescriptorSetLayoutSupport = reinterpret_cast<PFN_vkGetDescriptorSetLayoutSupport>(getInstanceProcAddrFunc(instance, "vkGetDescriptorSetLayoutSupport"));
    pVkFunctions->CmdPushDescriptorSetKHR = reinterpret_cast<PFN_vkCmdPushDescriptorSetKHR>(getInstanceProcAddrFunc(instance, "vkCmdPushDescriptorSetKHR"));
    pVkFunctions->GetDescriptorSetLayoutSizeEXT = reinterpret_cast<PFN_vkGetDescriptorSetLayoutSizeEXT>(getInstanceProcAddrFunc(instance, "vkGetDescriptorSetLayoutSizeEXT"));
    pVkFunctions->GetDescriptorSetLayoutBindingOffsetEXT = reinterpret_cast<PFN_vkGetDescriptorSetLayoutBindingOffsetEXT>(getInstanceProcAddrFunc(instance, "vkGetDescriptorSetLayoutBindingOffsetEXT"));
    pVkFunctions->GetDescriptorEXT = reinterpret_cast<PFN_vkGetDescriptorEXT>(getInstanceProcAddrFunc(instance, "vkGetDescriptorEXT"));
    pVkFunctions->CmdBindDescriptorBuffersEXT = reinterpret_cast<PFN_vkCmdBindDescriptorBuffersEXT>(getInstanceProcAddrFunc(instance, "vkCmdBindDescriptorBuffersEXT"));
    pVkFunctions->CmdSetDescriptorBufferOffsetsEXT = reinterpret_cast<PFN_vkCmdSetDescriptorBufferOffsetsEXT>(getInstanceProcAddrFunc(instance, "vkCmdSetDescriptorBufferOffsetsEXT"));
    pVkFunctions->GetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(getInstanceProcAddrFunc(instance, "vkGetBufferDeviceAddressKHR"));
    pVkFunctions->GetMemoryFdKHR = reinterpret_cast<PFN_vkGetMemoryFdKHR>(getInstanceProcAddrFunc(instance, "vkGetMemoryFdKHR"));
    pVkFunctions->GetFenceFdKHR = reinterpret_cast<PFN_vkGetFenceFdKHR>(getInstanceProcAddrFunc(instance, "vkGetFenceFdKHR"));
    pVkFunctions->CreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(getInstanceProcAddrFunc(instance, "vkCreateSwapchainKHR"));
    pVkFunctions->DestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(getInstanceProcAddrFunc(instance, "vkDestroySwapchainKHR"));
    pVkFunctions->GetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(getInstanceProcAddrFunc(instance, "vkGetSwapchainImagesKHR"));
    pVkFunctions->AcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(getInstanceProcAddrFunc(instance, "vkAcquireNextImageKHR"));
    pVkFunctions->QueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(getInstanceProcAddrFunc(instance, "vkQueuePresentKHR"));
    pVkFunctions->DisplayPowerControlEXT = reinterpret_cast<PFN_vkDisplayPowerControlEXT>(getInstanceProcAddrFunc(instance, "vkDisplayPowerControlEXT"));
    pVkFunctions->CreateSharedSwapchainsKHR = reinterpret_cast<PFN_vkCreateSharedSwapchainsKHR>(getInstanceProcAddrFunc(instance, "vkCreateSharedSwapchainsKHR"));
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->CreateVideoSessionKHR = reinterpret_cast<PFN_vkCreateVideoSessionKHR>(getInstanceProcAddrFunc(instance, "vkCreateVideoSessionKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->DestroyVideoSessionKHR = reinterpret_cast<PFN_vkDestroyVideoSessionKHR>(getInstanceProcAddrFunc(instance, "vkDestroyVideoSessionKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->CreateVideoSessionParametersKHR = reinterpret_cast<PFN_vkCreateVideoSessionParametersKHR>(getInstanceProcAddrFunc(instance, "vkCreateVideoSessionParametersKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->UpdateVideoSessionParametersKHR = reinterpret_cast<PFN_vkUpdateVideoSessionParametersKHR>(getInstanceProcAddrFunc(instance, "vkUpdateVideoSessionParametersKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->DestroyVideoSessionParametersKHR = reinterpret_cast<PFN_vkDestroyVideoSessionParametersKHR>(getInstanceProcAddrFunc(instance, "vkDestroyVideoSessionParametersKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->GetVideoSessionMemoryRequirementsKHR = reinterpret_cast<PFN_vkGetVideoSessionMemoryRequirementsKHR>(getInstanceProcAddrFunc(instance, "vkGetVideoSessionMemoryRequirementsKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->BindVideoSessionMemoryKHR = reinterpret_cast<PFN_vkBindVideoSessionMemoryKHR>(getInstanceProcAddrFunc(instance, "vkBindVideoSessionMemoryKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->CmdBeginVideoCodingKHR = reinterpret_cast<PFN_vkCmdBeginVideoCodingKHR>(getInstanceProcAddrFunc(instance, "vkCmdBeginVideoCodingKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->CmdEndVideoCodingKHR = reinterpret_cast<PFN_vkCmdEndVideoCodingKHR>(getInstanceProcAddrFunc(instance, "vkCmdEndVideoCodingKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->CmdControlVideoCodingKHR = reinterpret_cast<PFN_vkCmdControlVideoCodingKHR>(getInstanceProcAddrFunc(instance, "vkCmdControlVideoCodingKHR"));
#endif
#ifdef VK_USE_VIDEO_DECODE_QUEUE
    pVkFunctions->CmdDecodeVideoKHR = reinterpret_cast<PFN_vkCmdDecodeVideoKHR>(getInstanceProcAddrFunc(instance, "vkCmdDecodeVideoKHR"));
#endif
#ifdef VK_USE_VIDEO_ENCODE_QUEUE
    pVkFunctions->CmdEncodeVideoKHR = reinterpret_cast<PFN_vkCmdEncodeVideoKHR>(getInstanceProcAddrFunc(instance, "vkCmdEncodeVideoKHR"));
#endif
#ifdef VK_USE_VIDEO_ENCODE_QUEUE
    pVkFunctions->GetEncodedVideoSessionParametersKHR = reinterpret_cast<PFN_vkGetEncodedVideoSessionParametersKHR>(getInstanceProcAddrFunc(instance, "vkGetEncodedVideoSessionParametersKHR"));
#endif
    pVkFunctions->CmdSetEvent2KHR = reinterpret_cast<PFN_vkCmdSetEvent2KHR>(getInstanceProcAddrFunc(instance, "vkCmdSetEvent2KHR"));
    pVkFunctions->CmdResetEvent2KHR = reinterpret_cast<PFN_vkCmdResetEvent2KHR>(getInstanceProcAddrFunc(instance, "vkCmdResetEvent2KHR"));
    pVkFunctions->CmdWaitEvents2KHR = reinterpret_cast<PFN_vkCmdWaitEvents2KHR>(getInstanceProcAddrFunc(instance, "vkCmdWaitEvents2KHR"));
    pVkFunctions->CmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(getInstanceProcAddrFunc(instance, "vkCmdPipelineBarrier2KHR"));
    pVkFunctions->CmdWriteTimestamp2KHR = reinterpret_cast<PFN_vkCmdWriteTimestamp2KHR>(getInstanceProcAddrFunc(instance, "vkCmdWriteTimestamp2KHR"));
    pVkFunctions->QueueSubmit2KHR = reinterpret_cast<PFN_vkQueueSubmit2KHR>(getInstanceProcAddrFunc(instance, "vkQueueSubmit2KHR"));
}

void InitDispatchTableBottom(VkInstance instance, VkDevice dev, VkInterfaceFunctions* pVkFunctions)
{
    PFN_vkGetInstanceProcAddr getInstanceProcAddrFunc = pVkFunctions->GetInstanceProcAddr;
    pVkFunctions->GetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(getInstanceProcAddrFunc(instance, "vkGetDeviceProcAddr"));
    PFN_vkGetDeviceProcAddr getDeviceProcAddrFunc = pVkFunctions->GetDeviceProcAddr;
    pVkFunctions->GetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(getDeviceProcAddrFunc(dev, "vkGetDeviceProcAddr"));
    getDeviceProcAddrFunc = pVkFunctions->GetDeviceProcAddr;

    pVkFunctions->DestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(getDeviceProcAddrFunc(dev, "vkDestroyDevice"));
    pVkFunctions->GetDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(getDeviceProcAddrFunc(dev, "vkGetDeviceQueue"));
    pVkFunctions->QueueSubmit = reinterpret_cast<PFN_vkQueueSubmit>(getDeviceProcAddrFunc(dev, "vkQueueSubmit"));
    pVkFunctions->QueueWaitIdle = reinterpret_cast<PFN_vkQueueWaitIdle>(getDeviceProcAddrFunc(dev, "vkQueueWaitIdle"));
    pVkFunctions->DeviceWaitIdle = reinterpret_cast<PFN_vkDeviceWaitIdle>(getDeviceProcAddrFunc(dev, "vkDeviceWaitIdle"));
    pVkFunctions->AllocateMemory = reinterpret_cast<PFN_vkAllocateMemory>(getDeviceProcAddrFunc(dev, "vkAllocateMemory"));
    pVkFunctions->FreeMemory = reinterpret_cast<PFN_vkFreeMemory>(getDeviceProcAddrFunc(dev, "vkFreeMemory"));
    pVkFunctions->MapMemory = reinterpret_cast<PFN_vkMapMemory>(getDeviceProcAddrFunc(dev, "vkMapMemory"));
    pVkFunctions->UnmapMemory = reinterpret_cast<PFN_vkUnmapMemory>(getDeviceProcAddrFunc(dev, "vkUnmapMemory"));
    pVkFunctions->FlushMappedMemoryRanges = reinterpret_cast<PFN_vkFlushMappedMemoryRanges>(getDeviceProcAddrFunc(dev, "vkFlushMappedMemoryRanges"));
    pVkFunctions->InvalidateMappedMemoryRanges = reinterpret_cast<PFN_vkInvalidateMappedMemoryRanges>(getDeviceProcAddrFunc(dev, "vkInvalidateMappedMemoryRanges"));
    pVkFunctions->GetDeviceMemoryCommitment = reinterpret_cast<PFN_vkGetDeviceMemoryCommitment>(getDeviceProcAddrFunc(dev, "vkGetDeviceMemoryCommitment"));
    pVkFunctions->BindBufferMemory = reinterpret_cast<PFN_vkBindBufferMemory>(getDeviceProcAddrFunc(dev, "vkBindBufferMemory"));
    pVkFunctions->BindImageMemory = reinterpret_cast<PFN_vkBindImageMemory>(getDeviceProcAddrFunc(dev, "vkBindImageMemory"));
    pVkFunctions->GetBufferMemoryRequirements = reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(getDeviceProcAddrFunc(dev, "vkGetBufferMemoryRequirements"));
    pVkFunctions->GetImageMemoryRequirements = reinterpret_cast<PFN_vkGetImageMemoryRequirements>(getDeviceProcAddrFunc(dev, "vkGetImageMemoryRequirements"));
    pVkFunctions->GetImageSparseMemoryRequirements = reinterpret_cast<PFN_vkGetImageSparseMemoryRequirements>(getDeviceProcAddrFunc(dev, "vkGetImageSparseMemoryRequirements"));
    pVkFunctions->QueueBindSparse = reinterpret_cast<PFN_vkQueueBindSparse>(getDeviceProcAddrFunc(dev, "vkQueueBindSparse"));
    pVkFunctions->CreateFence = reinterpret_cast<PFN_vkCreateFence>(getDeviceProcAddrFunc(dev, "vkCreateFence"));
    pVkFunctions->DestroyFence = reinterpret_cast<PFN_vkDestroyFence>(getDeviceProcAddrFunc(dev, "vkDestroyFence"));
    pVkFunctions->ResetFences = reinterpret_cast<PFN_vkResetFences>(getDeviceProcAddrFunc(dev, "vkResetFences"));
    pVkFunctions->GetFenceStatus = reinterpret_cast<PFN_vkGetFenceStatus>(getDeviceProcAddrFunc(dev, "vkGetFenceStatus"));
    pVkFunctions->WaitForFences = reinterpret_cast<PFN_vkWaitForFences>(getDeviceProcAddrFunc(dev, "vkWaitForFences"));
    pVkFunctions->CreateSemaphore = reinterpret_cast<PFN_vkCreateSemaphore>(getDeviceProcAddrFunc(dev, "vkCreateSemaphore"));
    pVkFunctions->DestroySemaphore = reinterpret_cast<PFN_vkDestroySemaphore>(getDeviceProcAddrFunc(dev, "vkDestroySemaphore"));
    pVkFunctions->GetSemaphoreCounterValue = reinterpret_cast<PFN_vkGetSemaphoreCounterValue>(getDeviceProcAddrFunc(dev, "vkGetSemaphoreCounterValue"));
    pVkFunctions->WaitSemaphores = reinterpret_cast<PFN_vkWaitSemaphores>(getDeviceProcAddrFunc(dev, "vkWaitSemaphores"));
    pVkFunctions->SignalSemaphore = reinterpret_cast<PFN_vkSignalSemaphore>(getDeviceProcAddrFunc(dev, "vkSignalSemaphore"));
    pVkFunctions->CreateEvent = reinterpret_cast<PFN_vkCreateEvent>(getDeviceProcAddrFunc(dev, "vkCreateEvent"));
    pVkFunctions->DestroyEvent = reinterpret_cast<PFN_vkDestroyEvent>(getDeviceProcAddrFunc(dev, "vkDestroyEvent"));
    pVkFunctions->GetEventStatus = reinterpret_cast<PFN_vkGetEventStatus>(getDeviceProcAddrFunc(dev, "vkGetEventStatus"));
    pVkFunctions->SetEvent = reinterpret_cast<PFN_vkSetEvent>(getDeviceProcAddrFunc(dev, "vkSetEvent"));
    pVkFunctions->ResetEvent = reinterpret_cast<PFN_vkResetEvent>(getDeviceProcAddrFunc(dev, "vkResetEvent"));
    pVkFunctions->CreateQueryPool = reinterpret_cast<PFN_vkCreateQueryPool>(getDeviceProcAddrFunc(dev, "vkCreateQueryPool"));
    pVkFunctions->DestroyQueryPool = reinterpret_cast<PFN_vkDestroyQueryPool>(getDeviceProcAddrFunc(dev, "vkDestroyQueryPool"));
    pVkFunctions->GetQueryPoolResults = reinterpret_cast<PFN_vkGetQueryPoolResults>(getDeviceProcAddrFunc(dev, "vkGetQueryPoolResults"));
    pVkFunctions->CreateBuffer = reinterpret_cast<PFN_vkCreateBuffer>(getDeviceProcAddrFunc(dev, "vkCreateBuffer"));
    pVkFunctions->DestroyBuffer = reinterpret_cast<PFN_vkDestroyBuffer>(getDeviceProcAddrFunc(dev, "vkDestroyBuffer"));
    pVkFunctions->CreateBufferView = reinterpret_cast<PFN_vkCreateBufferView>(getDeviceProcAddrFunc(dev, "vkCreateBufferView"));
    pVkFunctions->DestroyBufferView = reinterpret_cast<PFN_vkDestroyBufferView>(getDeviceProcAddrFunc(dev, "vkDestroyBufferView"));
    pVkFunctions->CreateImage = reinterpret_cast<PFN_vkCreateImage>(getDeviceProcAddrFunc(dev, "vkCreateImage"));
    pVkFunctions->DestroyImage = reinterpret_cast<PFN_vkDestroyImage>(getDeviceProcAddrFunc(dev, "vkDestroyImage"));
    pVkFunctions->GetImageSubresourceLayout = reinterpret_cast<PFN_vkGetImageSubresourceLayout>(getDeviceProcAddrFunc(dev, "vkGetImageSubresourceLayout"));
    pVkFunctions->CreateImageView = reinterpret_cast<PFN_vkCreateImageView>(getDeviceProcAddrFunc(dev, "vkCreateImageView"));
    pVkFunctions->DestroyImageView = reinterpret_cast<PFN_vkDestroyImageView>(getDeviceProcAddrFunc(dev, "vkDestroyImageView"));
    pVkFunctions->CreateShaderModule = reinterpret_cast<PFN_vkCreateShaderModule>(getDeviceProcAddrFunc(dev, "vkCreateShaderModule"));
    pVkFunctions->DestroyShaderModule = reinterpret_cast<PFN_vkDestroyShaderModule>(getDeviceProcAddrFunc(dev, "vkDestroyShaderModule"));
    pVkFunctions->CreatePipelineCache = reinterpret_cast<PFN_vkCreatePipelineCache>(getDeviceProcAddrFunc(dev, "vkCreatePipelineCache"));
    pVkFunctions->DestroyPipelineCache = reinterpret_cast<PFN_vkDestroyPipelineCache>(getDeviceProcAddrFunc(dev, "vkDestroyPipelineCache"));
    pVkFunctions->GetPipelineCacheData = reinterpret_cast<PFN_vkGetPipelineCacheData>(getDeviceProcAddrFunc(dev, "vkGetPipelineCacheData"));
    pVkFunctions->MergePipelineCaches = reinterpret_cast<PFN_vkMergePipelineCaches>(getDeviceProcAddrFunc(dev, "vkMergePipelineCaches"));
    pVkFunctions->CreateGraphicsPipelines = reinterpret_cast<PFN_vkCreateGraphicsPipelines>(getDeviceProcAddrFunc(dev, "vkCreateGraphicsPipelines"));
    pVkFunctions->CreateComputePipelines = reinterpret_cast<PFN_vkCreateComputePipelines>(getDeviceProcAddrFunc(dev, "vkCreateComputePipelines"));
    pVkFunctions->DestroyPipeline = reinterpret_cast<PFN_vkDestroyPipeline>(getDeviceProcAddrFunc(dev, "vkDestroyPipeline"));
    pVkFunctions->CreatePipelineLayout = reinterpret_cast<PFN_vkCreatePipelineLayout>(getDeviceProcAddrFunc(dev, "vkCreatePipelineLayout"));
    pVkFunctions->DestroyPipelineLayout = reinterpret_cast<PFN_vkDestroyPipelineLayout>(getDeviceProcAddrFunc(dev, "vkDestroyPipelineLayout"));
    pVkFunctions->CreateSampler = reinterpret_cast<PFN_vkCreateSampler>(getDeviceProcAddrFunc(dev, "vkCreateSampler"));
    pVkFunctions->DestroySampler = reinterpret_cast<PFN_vkDestroySampler>(getDeviceProcAddrFunc(dev, "vkDestroySampler"));
    pVkFunctions->CreateDescriptorSetLayout = reinterpret_cast<PFN_vkCreateDescriptorSetLayout>(getDeviceProcAddrFunc(dev, "vkCreateDescriptorSetLayout"));
    pVkFunctions->DestroyDescriptorSetLayout = reinterpret_cast<PFN_vkDestroyDescriptorSetLayout>(getDeviceProcAddrFunc(dev, "vkDestroyDescriptorSetLayout"));
    pVkFunctions->CreateDescriptorPool = reinterpret_cast<PFN_vkCreateDescriptorPool>(getDeviceProcAddrFunc(dev, "vkCreateDescriptorPool"));
    pVkFunctions->DestroyDescriptorPool = reinterpret_cast<PFN_vkDestroyDescriptorPool>(getDeviceProcAddrFunc(dev, "vkDestroyDescriptorPool"));
    pVkFunctions->ResetDescriptorPool = reinterpret_cast<PFN_vkResetDescriptorPool>(getDeviceProcAddrFunc(dev, "vkResetDescriptorPool"));
    pVkFunctions->AllocateDescriptorSets = reinterpret_cast<PFN_vkAllocateDescriptorSets>(getDeviceProcAddrFunc(dev, "vkAllocateDescriptorSets"));
    pVkFunctions->FreeDescriptorSets = reinterpret_cast<PFN_vkFreeDescriptorSets>(getDeviceProcAddrFunc(dev, "vkFreeDescriptorSets"));
    pVkFunctions->UpdateDescriptorSets = reinterpret_cast<PFN_vkUpdateDescriptorSets>(getDeviceProcAddrFunc(dev, "vkUpdateDescriptorSets"));
    pVkFunctions->CreateFramebuffer = reinterpret_cast<PFN_vkCreateFramebuffer>(getDeviceProcAddrFunc(dev, "vkCreateFramebuffer"));
    pVkFunctions->DestroyFramebuffer = reinterpret_cast<PFN_vkDestroyFramebuffer>(getDeviceProcAddrFunc(dev, "vkDestroyFramebuffer"));
    pVkFunctions->CreateRenderPass = reinterpret_cast<PFN_vkCreateRenderPass>(getDeviceProcAddrFunc(dev, "vkCreateRenderPass"));
    pVkFunctions->DestroyRenderPass = reinterpret_cast<PFN_vkDestroyRenderPass>(getDeviceProcAddrFunc(dev, "vkDestroyRenderPass"));
    pVkFunctions->GetRenderAreaGranularity = reinterpret_cast<PFN_vkGetRenderAreaGranularity>(getDeviceProcAddrFunc(dev, "vkGetRenderAreaGranularity"));
    pVkFunctions->CreateCommandPool = reinterpret_cast<PFN_vkCreateCommandPool>(getDeviceProcAddrFunc(dev, "vkCreateCommandPool"));
    pVkFunctions->DestroyCommandPool = reinterpret_cast<PFN_vkDestroyCommandPool>(getDeviceProcAddrFunc(dev, "vkDestroyCommandPool"));
    pVkFunctions->ResetCommandPool = reinterpret_cast<PFN_vkResetCommandPool>(getDeviceProcAddrFunc(dev, "vkResetCommandPool"));
    pVkFunctions->AllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(getDeviceProcAddrFunc(dev, "vkAllocateCommandBuffers"));
    pVkFunctions->FreeCommandBuffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(getDeviceProcAddrFunc(dev, "vkFreeCommandBuffers"));
    pVkFunctions->BeginCommandBuffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(getDeviceProcAddrFunc(dev, "vkBeginCommandBuffer"));
    pVkFunctions->EndCommandBuffer = reinterpret_cast<PFN_vkEndCommandBuffer>(getDeviceProcAddrFunc(dev, "vkEndCommandBuffer"));
    pVkFunctions->ResetCommandBuffer = reinterpret_cast<PFN_vkResetCommandBuffer>(getDeviceProcAddrFunc(dev, "vkResetCommandBuffer"));
    pVkFunctions->CmdBindPipeline = reinterpret_cast<PFN_vkCmdBindPipeline>(getDeviceProcAddrFunc(dev, "vkCmdBindPipeline"));
    pVkFunctions->CmdSetViewport = reinterpret_cast<PFN_vkCmdSetViewport>(getDeviceProcAddrFunc(dev, "vkCmdSetViewport"));
    pVkFunctions->CmdSetScissor = reinterpret_cast<PFN_vkCmdSetScissor>(getDeviceProcAddrFunc(dev, "vkCmdSetScissor"));
    pVkFunctions->CmdSetLineWidth = reinterpret_cast<PFN_vkCmdSetLineWidth>(getDeviceProcAddrFunc(dev, "vkCmdSetLineWidth"));
    pVkFunctions->CmdSetDepthBias = reinterpret_cast<PFN_vkCmdSetDepthBias>(getDeviceProcAddrFunc(dev, "vkCmdSetDepthBias"));
    pVkFunctions->CmdSetBlendConstants = reinterpret_cast<PFN_vkCmdSetBlendConstants>(getDeviceProcAddrFunc(dev, "vkCmdSetBlendConstants"));
    pVkFunctions->CmdSetDepthBounds = reinterpret_cast<PFN_vkCmdSetDepthBounds>(getDeviceProcAddrFunc(dev, "vkCmdSetDepthBounds"));
    pVkFunctions->CmdSetStencilCompareMask = reinterpret_cast<PFN_vkCmdSetStencilCompareMask>(getDeviceProcAddrFunc(dev, "vkCmdSetStencilCompareMask"));
    pVkFunctions->CmdSetStencilWriteMask = reinterpret_cast<PFN_vkCmdSetStencilWriteMask>(getDeviceProcAddrFunc(dev, "vkCmdSetStencilWriteMask"));
    pVkFunctions->CmdSetStencilReference = reinterpret_cast<PFN_vkCmdSetStencilReference>(getDeviceProcAddrFunc(dev, "vkCmdSetStencilReference"));
    pVkFunctions->CmdBindDescriptorSets = reinterpret_cast<PFN_vkCmdBindDescriptorSets>(getDeviceProcAddrFunc(dev, "vkCmdBindDescriptorSets"));
    pVkFunctions->CmdBindIndexBuffer = reinterpret_cast<PFN_vkCmdBindIndexBuffer>(getDeviceProcAddrFunc(dev, "vkCmdBindIndexBuffer"));
    pVkFunctions->CmdBindVertexBuffers = reinterpret_cast<PFN_vkCmdBindVertexBuffers>(getDeviceProcAddrFunc(dev, "vkCmdBindVertexBuffers"));
    pVkFunctions->CmdDraw = reinterpret_cast<PFN_vkCmdDraw>(getDeviceProcAddrFunc(dev, "vkCmdDraw"));
    pVkFunctions->CmdDrawIndexed = reinterpret_cast<PFN_vkCmdDrawIndexed>(getDeviceProcAddrFunc(dev, "vkCmdDrawIndexed"));
    pVkFunctions->CmdDrawIndirect = reinterpret_cast<PFN_vkCmdDrawIndirect>(getDeviceProcAddrFunc(dev, "vkCmdDrawIndirect"));
    pVkFunctions->CmdDrawIndexedIndirect = reinterpret_cast<PFN_vkCmdDrawIndexedIndirect>(getDeviceProcAddrFunc(dev, "vkCmdDrawIndexedIndirect"));
    pVkFunctions->CmdDispatch = reinterpret_cast<PFN_vkCmdDispatch>(getDeviceProcAddrFunc(dev, "vkCmdDispatch"));
    pVkFunctions->CmdDispatchIndirect = reinterpret_cast<PFN_vkCmdDispatchIndirect>(getDeviceProcAddrFunc(dev, "vkCmdDispatchIndirect"));
    pVkFunctions->CmdCopyBuffer = reinterpret_cast<PFN_vkCmdCopyBuffer>(getDeviceProcAddrFunc(dev, "vkCmdCopyBuffer"));
    pVkFunctions->CmdCopyImage = reinterpret_cast<PFN_vkCmdCopyImage>(getDeviceProcAddrFunc(dev, "vkCmdCopyImage"));
    pVkFunctions->CmdBlitImage = reinterpret_cast<PFN_vkCmdBlitImage>(getDeviceProcAddrFunc(dev, "vkCmdBlitImage"));
    pVkFunctions->CmdCopyBufferToImage = reinterpret_cast<PFN_vkCmdCopyBufferToImage>(getDeviceProcAddrFunc(dev, "vkCmdCopyBufferToImage"));
    pVkFunctions->CmdCopyImageToBuffer = reinterpret_cast<PFN_vkCmdCopyImageToBuffer>(getDeviceProcAddrFunc(dev, "vkCmdCopyImageToBuffer"));
    pVkFunctions->CmdUpdateBuffer = reinterpret_cast<PFN_vkCmdUpdateBuffer>(getDeviceProcAddrFunc(dev, "vkCmdUpdateBuffer"));
    pVkFunctions->CmdFillBuffer = reinterpret_cast<PFN_vkCmdFillBuffer>(getDeviceProcAddrFunc(dev, "vkCmdFillBuffer"));
    pVkFunctions->CmdClearColorImage = reinterpret_cast<PFN_vkCmdClearColorImage>(getDeviceProcAddrFunc(dev, "vkCmdClearColorImage"));
    pVkFunctions->CmdClearDepthStencilImage = reinterpret_cast<PFN_vkCmdClearDepthStencilImage>(getDeviceProcAddrFunc(dev, "vkCmdClearDepthStencilImage"));
    pVkFunctions->CmdClearAttachments = reinterpret_cast<PFN_vkCmdClearAttachments>(getDeviceProcAddrFunc(dev, "vkCmdClearAttachments"));
    pVkFunctions->CmdResolveImage = reinterpret_cast<PFN_vkCmdResolveImage>(getDeviceProcAddrFunc(dev, "vkCmdResolveImage"));
    pVkFunctions->CmdSetEvent = reinterpret_cast<PFN_vkCmdSetEvent>(getDeviceProcAddrFunc(dev, "vkCmdSetEvent"));
    pVkFunctions->CmdResetEvent = reinterpret_cast<PFN_vkCmdResetEvent>(getDeviceProcAddrFunc(dev, "vkCmdResetEvent"));
    pVkFunctions->CmdWaitEvents = reinterpret_cast<PFN_vkCmdWaitEvents>(getDeviceProcAddrFunc(dev, "vkCmdWaitEvents"));
    pVkFunctions->CmdPipelineBarrier = reinterpret_cast<PFN_vkCmdPipelineBarrier>(getDeviceProcAddrFunc(dev, "vkCmdPipelineBarrier"));
    pVkFunctions->CmdBeginQuery = reinterpret_cast<PFN_vkCmdBeginQuery>(getDeviceProcAddrFunc(dev, "vkCmdBeginQuery"));
    pVkFunctions->CmdEndQuery = reinterpret_cast<PFN_vkCmdEndQuery>(getDeviceProcAddrFunc(dev, "vkCmdEndQuery"));
    pVkFunctions->CmdResetQueryPool = reinterpret_cast<PFN_vkCmdResetQueryPool>(getDeviceProcAddrFunc(dev, "vkCmdResetQueryPool"));
    pVkFunctions->CmdWriteTimestamp = reinterpret_cast<PFN_vkCmdWriteTimestamp>(getDeviceProcAddrFunc(dev, "vkCmdWriteTimestamp"));
    pVkFunctions->CmdCopyQueryPoolResults = reinterpret_cast<PFN_vkCmdCopyQueryPoolResults>(getDeviceProcAddrFunc(dev, "vkCmdCopyQueryPoolResults"));
    pVkFunctions->CmdPushConstants = reinterpret_cast<PFN_vkCmdPushConstants>(getDeviceProcAddrFunc(dev, "vkCmdPushConstants"));
    pVkFunctions->CmdBeginRenderPass = reinterpret_cast<PFN_vkCmdBeginRenderPass>(getDeviceProcAddrFunc(dev, "vkCmdBeginRenderPass"));
    pVkFunctions->CmdNextSubpass = reinterpret_cast<PFN_vkCmdNextSubpass>(getDeviceProcAddrFunc(dev, "vkCmdNextSubpass"));
    pVkFunctions->CmdEndRenderPass = reinterpret_cast<PFN_vkCmdEndRenderPass>(getDeviceProcAddrFunc(dev, "vkCmdEndRenderPass"));
    pVkFunctions->CmdExecuteCommands = reinterpret_cast<PFN_vkCmdExecuteCommands>(getDeviceProcAddrFunc(dev, "vkCmdExecuteCommands"));
    pVkFunctions->BindBufferMemory2 = reinterpret_cast<PFN_vkBindBufferMemory2>(getDeviceProcAddrFunc(dev, "vkBindBufferMemory2"));
    pVkFunctions->BindImageMemory2 = reinterpret_cast<PFN_vkBindImageMemory2>(getDeviceProcAddrFunc(dev, "vkBindImageMemory2"));
    pVkFunctions->GetDeviceGroupPeerMemoryFeatures = reinterpret_cast<PFN_vkGetDeviceGroupPeerMemoryFeatures>(getDeviceProcAddrFunc(dev, "vkGetDeviceGroupPeerMemoryFeatures"));
    pVkFunctions->CmdSetDeviceMask = reinterpret_cast<PFN_vkCmdSetDeviceMask>(getDeviceProcAddrFunc(dev, "vkCmdSetDeviceMask"));
    pVkFunctions->CmdDispatchBase = reinterpret_cast<PFN_vkCmdDispatchBase>(getDeviceProcAddrFunc(dev, "vkCmdDispatchBase"));
    pVkFunctions->GetImageMemoryRequirements2 = reinterpret_cast<PFN_vkGetImageMemoryRequirements2>(getDeviceProcAddrFunc(dev, "vkGetImageMemoryRequirements2"));
    pVkFunctions->GetBufferMemoryRequirements2 = reinterpret_cast<PFN_vkGetBufferMemoryRequirements2>(getDeviceProcAddrFunc(dev, "vkGetBufferMemoryRequirements2"));
    pVkFunctions->GetImageSparseMemoryRequirements2 = reinterpret_cast<PFN_vkGetImageSparseMemoryRequirements2>(getDeviceProcAddrFunc(dev, "vkGetImageSparseMemoryRequirements2"));
    pVkFunctions->TrimCommandPool = reinterpret_cast<PFN_vkTrimCommandPool>(getDeviceProcAddrFunc(dev, "vkTrimCommandPool"));
    pVkFunctions->GetDeviceQueue2 = reinterpret_cast<PFN_vkGetDeviceQueue2>(getDeviceProcAddrFunc(dev, "vkGetDeviceQueue2"));
    pVkFunctions->CreateSamplerYcbcrConversion = reinterpret_cast<PFN_vkCreateSamplerYcbcrConversion>(getDeviceProcAddrFunc(dev, "vkCreateSamplerYcbcrConversion"));
    pVkFunctions->DestroySamplerYcbcrConversion = reinterpret_cast<PFN_vkDestroySamplerYcbcrConversion>(getDeviceProcAddrFunc(dev, "vkDestroySamplerYcbcrConversion"));
    pVkFunctions->CreateDescriptorUpdateTemplate = reinterpret_cast<PFN_vkCreateDescriptorUpdateTemplate>(getDeviceProcAddrFunc(dev, "vkCreateDescriptorUpdateTemplate"));
    pVkFunctions->DestroyDescriptorUpdateTemplate = reinterpret_cast<PFN_vkDestroyDescriptorUpdateTemplate>(getDeviceProcAddrFunc(dev, "vkDestroyDescriptorUpdateTemplate"));
    pVkFunctions->UpdateDescriptorSetWithTemplate = reinterpret_cast<PFN_vkUpdateDescriptorSetWithTemplate>(getDeviceProcAddrFunc(dev, "vkUpdateDescriptorSetWithTemplate"));
    pVkFunctions->GetDescriptorSetLayoutSupport = reinterpret_cast<PFN_vkGetDescriptorSetLayoutSupport>(getDeviceProcAddrFunc(dev, "vkGetDescriptorSetLayoutSupport"));
    pVkFunctions->CmdPushDescriptorSetKHR = reinterpret_cast<PFN_vkCmdPushDescriptorSetKHR>(getDeviceProcAddrFunc(dev, "vkCmdPushDescriptorSetKHR"));
    pVkFunctions->GetDescriptorSetLayoutSizeEXT = reinterpret_cast<PFN_vkGetDescriptorSetLayoutSizeEXT>(getDeviceProcAddrFunc(dev, "vkGetDescriptorSetLayoutSizeEXT"));
    pVkFunctions->GetDescriptorSetLayoutBindingOffsetEXT = reinterpret_cast<PFN_vkGetDescriptorSetLayoutBindingOffsetEXT>(getDeviceProcAddrFunc(dev, "vkGetDescriptorSetLayoutBindingOffsetEXT"));
    pVkFunctions->GetDescriptorEXT = reinterpret_cast<PFN_vkGetDescriptorEXT>(getDeviceProcAddrFunc(dev, "vkGetDescriptorEXT"));
    pVkFunctions->CmdBindDescriptorBuffersEXT = reinterpret_cast<PFN_vkCmdBindDescriptorBuffersEXT>(getDeviceProcAddrFunc(dev, "vkCmdBindDescriptorBuffersEXT"));
    pVkFunctions->CmdSetDescriptorBufferOffsetsEXT = reinterpret_cast<PFN_vkCmdSetDescriptorBufferOffsetsEXT>(getDeviceProcAddrFunc(dev, "vkCmdSetDescriptorBufferOffsetsEXT"));
    pVkFunctions->GetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(getDeviceProcAddrFunc(dev, "vkGetBufferDeviceAddressKHR"));
    pVkFunctions->GetMemoryFdKHR = reinterpret_cast<PFN_vkGetMemoryFdKHR>(getDeviceProcAddrFunc(dev, "vkGetMemoryFdKHR"));
    pVkFunctions->GetFenceFdKHR = reinterpret_cast<PFN_vkGetFenceFdKHR>(getDeviceProcAddrFunc(dev, "vkGetFenceFdKHR"));
    pVkFunctions->CreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(getDeviceProcAddrFunc(dev, "vkCreateSwapchainKHR"));
    pVkFunctions->DestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(getDeviceProcAddrFunc(dev, "vkDestroySwapchainKHR"));
    pVkFunctions->GetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(getDeviceProcAddrFunc(dev, "vkGetSwapchainImagesKHR"));
    pVkFunctions->AcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(getDeviceProcAddrFunc(dev, "vkAcquireNextImageKHR"));
    pVkFunctions->QueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(getDeviceProcAddrFunc(dev, "vkQueuePresentKHR"));
    pVkFunctions->DisplayPowerControlEXT = reinterpret_cast<PFN_vkDisplayPowerControlEXT>(getDeviceProcAddrFunc(dev, "vkDisplayPowerControlEXT"));
    pVkFunctions->CreateSharedSwapchainsKHR = reinterpret_cast<PFN_vkCreateSharedSwapchainsKHR>(getDeviceProcAddrFunc(dev, "vkCreateSharedSwapchainsKHR"));
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->CreateVideoSessionKHR = reinterpret_cast<PFN_vkCreateVideoSessionKHR>(getDeviceProcAddrFunc(dev, "vkCreateVideoSessionKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->DestroyVideoSessionKHR = reinterpret_cast<PFN_vkDestroyVideoSessionKHR>(getDeviceProcAddrFunc(dev, "vkDestroyVideoSessionKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->CreateVideoSessionParametersKHR = reinterpret_cast<PFN_vkCreateVideoSessionParametersKHR>(getDeviceProcAddrFunc(dev, "vkCreateVideoSessionParametersKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->UpdateVideoSessionParametersKHR = reinterpret_cast<PFN_vkUpdateVideoSessionParametersKHR>(getDeviceProcAddrFunc(dev, "vkUpdateVideoSessionParametersKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->DestroyVideoSessionParametersKHR = reinterpret_cast<PFN_vkDestroyVideoSessionParametersKHR>(getDeviceProcAddrFunc(dev, "vkDestroyVideoSessionParametersKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->GetVideoSessionMemoryRequirementsKHR = reinterpret_cast<PFN_vkGetVideoSessionMemoryRequirementsKHR>(getDeviceProcAddrFunc(dev, "vkGetVideoSessionMemoryRequirementsKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->BindVideoSessionMemoryKHR = reinterpret_cast<PFN_vkBindVideoSessionMemoryKHR>(getDeviceProcAddrFunc(dev, "vkBindVideoSessionMemoryKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->CmdBeginVideoCodingKHR = reinterpret_cast<PFN_vkCmdBeginVideoCodingKHR>(getDeviceProcAddrFunc(dev, "vkCmdBeginVideoCodingKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->CmdEndVideoCodingKHR = reinterpret_cast<PFN_vkCmdEndVideoCodingKHR>(getDeviceProcAddrFunc(dev, "vkCmdEndVideoCodingKHR"));
#endif
#ifdef VK_USE_VIDEO_QUEUE
    pVkFunctions->CmdControlVideoCodingKHR = reinterpret_cast<PFN_vkCmdControlVideoCodingKHR>(getDeviceProcAddrFunc(dev, "vkCmdControlVideoCodingKHR"));
#endif
#ifdef VK_USE_VIDEO_DECODE_QUEUE
    pVkFunctions->CmdDecodeVideoKHR = reinterpret_cast<PFN_vkCmdDecodeVideoKHR>(getDeviceProcAddrFunc(dev, "vkCmdDecodeVideoKHR"));
#endif
#ifdef VK_USE_VIDEO_ENCODE_QUEUE
    pVkFunctions->CmdEncodeVideoKHR = reinterpret_cast<PFN_vkCmdEncodeVideoKHR>(getDeviceProcAddrFunc(dev, "vkCmdEncodeVideoKHR"));
#endif
#ifdef VK_USE_VIDEO_ENCODE_QUEUE
    pVkFunctions->GetEncodedVideoSessionParametersKHR = reinterpret_cast<PFN_vkGetEncodedVideoSessionParametersKHR>(getDeviceProcAddrFunc(dev, "vkGetEncodedVideoSessionParametersKHR"));
#endif
    pVkFunctions->CmdSetEvent2KHR = reinterpret_cast<PFN_vkCmdSetEvent2KHR>(getDeviceProcAddrFunc(dev, "vkCmdSetEvent2KHR"));
    pVkFunctions->CmdResetEvent2KHR = reinterpret_cast<PFN_vkCmdResetEvent2KHR>(getDeviceProcAddrFunc(dev, "vkCmdResetEvent2KHR"));
    pVkFunctions->CmdWaitEvents2KHR = reinterpret_cast<PFN_vkCmdWaitEvents2KHR>(getDeviceProcAddrFunc(dev, "vkCmdWaitEvents2KHR"));
    pVkFunctions->CmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(getDeviceProcAddrFunc(dev, "vkCmdPipelineBarrier2KHR"));
    pVkFunctions->CmdWriteTimestamp2KHR = reinterpret_cast<PFN_vkCmdWriteTimestamp2KHR>(getDeviceProcAddrFunc(dev, "vkCmdWriteTimestamp2KHR"));
    pVkFunctions->QueueSubmit2KHR = reinterpret_cast<PFN_vkQueueSubmit2KHR>(getDeviceProcAddrFunc(dev, "vkQueueSubmit2KHR"));
}

} // namespace vk
