#include <stdio.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>

VkSurfaceKHR waterlily_vulkanSurfaceCreate(VkInstance instance, void **data)
{
    VkWaylandSurfaceCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.display = data[0];
    createInfo.surface = data[1];

    VkSurfaceKHR createdSurface;
    VkResult code = vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr,
                                              &createdSurface);
    if (code != VK_SUCCESS)
    {
        fprintf(stderr,
                "Failed to create Vulkan-Wayland interop surface. Code: %d.\n",
                code);
        return nullptr;
    }
    return createdSurface;
}
