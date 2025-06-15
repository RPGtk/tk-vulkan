#ifndef TKVUL_WAYLAND_H
#define TKVUL_WAYLAND_H

#include <vulkan/vulkan.h>
#include <wayland-client.h>

VkSurfaceKHR tkvul_waylandCreate(VkInstance instance,
                                 struct wl_display *display,
                                 struct wl_surface *surface);

#endif // TKVUL_WAYLAND_H
