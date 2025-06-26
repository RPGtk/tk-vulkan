#ifndef WLVULKAN_WAYLAND_H
#define WLVULKAN_WAYLAND_H

#include <vulkan/vulkan.h>
#include <wayland-client.h>

VkSurfaceKHR waterlily_vulkanWaylandCreate(VkInstance instance,
                                           struct wl_display *display,
                                           struct wl_surface *surface);

#endif // WLVULKAN_WAYLAND_H
