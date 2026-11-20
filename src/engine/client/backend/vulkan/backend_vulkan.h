#ifndef ENGINE_CLIENT_BACKEND_VULKAN_BACKEND_VULKAN_H
#define ENGINE_CLIENT_BACKEND_VULKAN_BACKEND_VULKAN_H

#include <cstdint>

class CCommandProcessorFragment_GLBase;

void VulkanSetGpuTimingEnabled(bool Enabled);
uint64_t VulkanGpuFrameTimeNs();

static constexpr int BACKEND_VULKAN_VERSION_MAJOR = 1;
static constexpr int BACKEND_VULKAN_VERSION_MINOR = 1;

CCommandProcessorFragment_GLBase *CreateVulkanCommandProcessorFragment();

#endif
