#include <Include/Wayland.h>
#include <stdio.h>
#include <vulkan/vulkan_wayland.h>

VkSurfaceKHR tkvul_waylandCreate(VkInstance instance,
                                 struct wl_display *display,
                                 struct wl_surface *surface)
{
    VkWaylandSurfaceCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.display = display;
    createInfo.surface = surface;

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
