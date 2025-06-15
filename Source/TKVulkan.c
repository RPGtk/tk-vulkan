#include <TKVulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#ifdef WAYLAND
#include <Targets/Wayland.h>
#elifdef X11
#include <Targets/X11.h>
#endif

static VkInstance pInstance = nullptr;
static VkPhysicalDevice pPhysicalDevice = nullptr;
static VkDevice pLogicalDevice = nullptr;
static VkQueue pGraphicsQueue = nullptr;
static VkQueue pPresentQueue = nullptr;
static uint32_t pGraphicsIndex = 0;
static uint32_t pPresentIndex = 0;

static VkSurfaceKHR pSurface = nullptr;
static uint32_t pFormatCount = 0;
static VkSurfaceFormatKHR *pFormats = nullptr;
static uint32_t pModeCount = 0;
static VkPresentModeKHR *pModes = nullptr;

static VkSurfaceCapabilitiesKHR pCapabilities;
static VkSurfaceFormatKHR pFormat;
static VkPresentModeKHR pMode;

static VkSwapchainKHR pSwapchain = nullptr;
static VkImageView *pSwapchainImages = nullptr;

// TODO: Get this the fuck outta here.
// https://stackoverflow.com/questions/427477/fastest-way-to-clamp-a-real-fixed-floating-point-value#16659263
uint32_t _clamp(uint32_t d, uint32_t min, uint32_t max)
{
    const uint32_t t = d < min ? min : d;
    return t > max ? max : t;
}

VkExtent2D getSurfaceExtent(uint32_t width, uint32_t height)
{
    if (pCapabilities.currentExtent.width != UINT32_MAX)
        return pCapabilities.currentExtent;

    VkExtent2D surfaceExtent = {.width = width, .height = height};

    surfaceExtent.width =
        _clamp(surfaceExtent.width, pCapabilities.minImageExtent.width,
               pCapabilities.maxImageExtent.width);
    surfaceExtent.height =
        _clamp(surfaceExtent.height, pCapabilities.minImageExtent.height,
               pCapabilities.maxImageExtent.height);

    return surfaceExtent;
}

VkSurfaceFormatKHR chooseSurfaceFormat(void)
{
    for (size_t i = 0; i < pFormatCount; i++)
    {
        VkSurfaceFormatKHR format = pFormats[i];
        // This is the best combination. If not available, we'll just
        // select the first provided colorspace.
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            pFormat = format;
            return pFormat;
        }
    }
    return pFormats[0];
}

VkPresentModeKHR chooseSurfaceMode(void)
{
    for (size_t i = 0; i < pModeCount; i++)
    {
        // This is the best mode. If not available, we'll just select
        // VK_PRESENT_MODE_FIFO_KHR.
        if (pModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            pMode = pModes[i];
            return pMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceCapabilitiesKHR findSurfaceCapabilities(void)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pPhysicalDevice, pSurface,
                                              &pCapabilities);
    return pCapabilities;
}

VkSurfaceCapabilitiesKHR getSurfaceCapabilities() { return pCapabilities; }

bool createSwapchain(uint32_t framebufferWidth, uint32_t framebufferHeight)
{
    VkSurfaceFormatKHR format = chooseSurfaceFormat();
    VkPresentModeKHR mode = chooseSurfaceMode();
    VkExtent2D extent = getSurfaceExtent(framebufferWidth, framebufferHeight);
    VkSurfaceCapabilitiesKHR capabilities = getSurfaceCapabilities();

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
        imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = pSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = format.format;
    createInfo.imageColorSpace = format.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = mode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    uint32_t indices[2] = {pGraphicsIndex, pPresentIndex};
    if (indices[0] != indices[1])
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = indices;
    }
    else createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateSwapchainKHR(pLogicalDevice, &createInfo, nullptr,
                             &pSwapchain) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create swapchain.\n");
        return false;
    }

    vkGetSwapchainImagesKHR(pLogicalDevice, pSwapchain, &imageCount, nullptr);
    VkImage *images = malloc(sizeof(VkImage) * imageCount);
    pSwapchainImages = malloc(sizeof(VkImageView) * imageCount);
    vkGetSwapchainImagesKHR(pLogicalDevice, pSwapchain, &imageCount, images);

    VkImageViewCreateInfo imageCreateInfo = {0};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageCreateInfo.format = format.format;
    imageCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCreateInfo.subresourceRange.baseMipLevel = 0;
    imageCreateInfo.subresourceRange.levelCount = 1;
    imageCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageCreateInfo.subresourceRange.layerCount = 1;

    for (size_t i = 0; i < imageCount; i++)
    {
        imageCreateInfo.image = images[i];
        if (vkCreateImageView(pLogicalDevice, &imageCreateInfo, nullptr,
                              &pSwapchainImages[i]) != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to create image view %zu.", i);
            return false;
        }
    }

    return true;
}

bool createSurface(void)
{
#ifdef WAYLAND
    pSurface = tkvul_waylandCreate(instance);
    if (pSurface == nullptr) return false;
#elifdef X11
    // TODO: Implement X11.
#endif

    return true;
}

VkSurfaceFormatKHR *getSurfaceFormats(void)
{
    if (pFormats != nullptr) return pFormats;

    vkGetPhysicalDeviceSurfaceFormatsKHR(pPhysicalDevice, pSurface,
                                         &pFormatCount, nullptr);
    if (pFormatCount == 0) return nullptr;
    pFormats = malloc(sizeof(VkSurfaceFormatKHR) * pFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(pPhysicalDevice, pSurface,
                                         &pFormatCount, pFormats);
    return pFormats;
}

VkPresentModeKHR *getSurfaceModes(void)
{
    if (pModes != nullptr) return pModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pPhysicalDevice, pSurface,
                                              &pModeCount, nullptr);
    if (pModeCount == 0) return nullptr;
    pModes = malloc(sizeof(VkPresentModeKHR) * pModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(pPhysicalDevice, pSurface,
                                              &pModeCount, pModes);
    return pModes;
}

// TODO: File utilities.

static void compileShader(const char *name, size_t *size, uint32_t **bytes)
{
    FILE *file = fopen(name, "r");

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *bytes = malloc(*size);
    fread(*bytes, 1, *size, file);

    fclose(file);
}

bool createPipeline(void)
{
    size_t vertexSize, fragmentSize;
    uint32_t *vertexBytes, *fragmentBytes;
    compileShader("default.vert.spv", &vertexSize, &vertexBytes);
    compileShader("default.frag.spv", &fragmentSize, &fragmentBytes);
    printf("%zu :: %zu\n", vertexSize, fragmentSize);

    VkShaderModuleCreateInfo vertexCreateInfo = {0};
    vertexCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexCreateInfo.codeSize = vertexSize;
    vertexCreateInfo.pCode = vertexBytes;

    VkShaderModuleCreateInfo fragmentCreateInfo = {0};
    fragmentCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentCreateInfo.codeSize = fragmentSize;
    fragmentCreateInfo.pCode = fragmentBytes;

    VkShaderModule vertexModule, fragmentModule;
    if (vkCreateShaderModule(pLogicalDevice, &vertexCreateInfo, nullptr,
                             &vertexModule) != VK_SUCCESS ||
        vkCreateShaderModule(pLogicalDevice, &fragmentCreateInfo, nullptr,
                             &fragmentModule) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create shader module.\n");
        return false;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertexModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragmentModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                      fragShaderStageInfo};
    (void)shaderStages;

    vkDestroyShaderModule(pLogicalDevice, fragmentModule, nullptr);
    vkDestroyShaderModule(pLogicalDevice, vertexModule, nullptr);
    return true;
}

uint32_t scoreDevice(VkPhysicalDevice device, const char **extensions,
                     size_t extensionCount)
{
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);

    uint32_t score = 0;
    switch (properties.deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   score += 4; break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score += 3; break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    score += 2; break;
        default:                                     score += 1; break;
    }

    uint32_t availableExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr,
                                         &availableExtensionCount, nullptr);
    VkExtensionProperties *availableExtensions =
        malloc(sizeof(VkExtensionProperties) * availableExtensionCount);
    vkEnumerateDeviceExtensionProperties(
        device, nullptr, &availableExtensionCount, availableExtensions);

    size_t foundCount = 0;
    for (size_t i = 0; i < availableExtensionCount; i++)
    {
        VkExtensionProperties extension = availableExtensions[i];

        for (size_t j = 0; j < extensionCount; j++)
            if (strcmp(extension.extensionName, extensions[j]) == 0)
            {
                foundCount++;
                printf("Found: %s device extension.\n",
                       extension.extensionName);
                break;
            }
    }
    free(availableExtensions);
    if (foundCount != extensionCount)
    {
        fprintf(stderr, "Failed to find all device extensions.\n");
        return 0;
    }

    if (getSurfaceFormats() == nullptr || getSurfaceModes() == nullptr)
    {
        fprintf(stderr, "Failed to find surface format/present modes.\n");
        return 0;
    }

    // TODO: Extra grading to be done here.

    return score;
}

bool createDevice(uint32_t framebufferWidth, uint32_t framebufferHeight)
{
    uint32_t physicalCount = 0;
    vkEnumeratePhysicalDevices(pInstance, &physicalCount, nullptr);
    VkPhysicalDevice *physicalDevices =
        malloc(sizeof(VkPhysicalDevice) * physicalCount);
    vkEnumeratePhysicalDevices(pInstance, &physicalCount, physicalDevices);

    constexpr size_t extensionCount = 1;
    const char *extensions[extensionCount] = {"VK_KHR_swapchain"};

    VkPhysicalDevice currentChosen = nullptr;
    uint32_t bestScore = 0;
    for (size_t i = 0; i < physicalCount; i++)
    {
        uint32_t score =
            scoreDevice(physicalDevices[i], extensions, extensionCount);
        if (score > bestScore)
        {
            currentChosen = physicalDevices[i];
            bestScore = score;
        }
    }

    if (currentChosen == nullptr)
    {
        fprintf(stderr, "Failed to find suitable Vulkan device.\n");
        return false;
    }
    pPhysicalDevice = currentChosen;
    free(physicalDevices);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevice, &queueFamilyCount,
                                             nullptr);

    VkQueueFamilyProperties *queueFamilies =
        malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevice, &queueFamilyCount,
                                             queueFamilies);

    bool foundGraphicsQueue = false, foundPresentQueue = false;
    for (size_t i = 0; i < queueFamilyCount; i++)
    {
        if (foundGraphicsQueue && foundPresentQueue) break;

        VkQueueFamilyProperties family = queueFamilies[i];
        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            pGraphicsIndex = i;
            foundGraphicsQueue = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(pPhysicalDevice, i, pSurface,
                                             &presentSupport);
        if (presentSupport)
        {
            pPresentIndex = i;
            foundPresentQueue = true;
        }
    }

    if (!foundGraphicsQueue)
    {
        fprintf(stderr, "Failed to find graphics queue.\n");
        return false;
    }

    if (!foundPresentQueue)
    {
        fprintf(stderr, "Failed to find presentation queue.\n");
        return false;
    }

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfos[2] = {{0}, {0}};
    queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[0].queueFamilyIndex = pGraphicsIndex;
    queueCreateInfos[0].queueCount = 1;
    queueCreateInfos[0].pQueuePriorities = &priority;

    queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[1].queueFamilyIndex = pPresentIndex;
    queueCreateInfos[1].queueCount = 1;
    queueCreateInfos[1].pQueuePriorities = &priority;

    // We don't need any features at the moment.
    VkPhysicalDeviceFeatures usedFeatures = {0};

    // Layers for logical devices no longer need to be set in newer
    // implementations.
    VkDeviceCreateInfo logicalDeviceCreateInfo = {0};
    logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logicalDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    if (pPresentIndex != pGraphicsIndex)
        logicalDeviceCreateInfo.queueCreateInfoCount = 2;
    else logicalDeviceCreateInfo.queueCreateInfoCount = 1;
    logicalDeviceCreateInfo.pEnabledFeatures = &usedFeatures;

    logicalDeviceCreateInfo.enabledExtensionCount = extensionCount;
    logicalDeviceCreateInfo.ppEnabledExtensionNames = extensions;

    VkResult code = vkCreateDevice(pPhysicalDevice, &logicalDeviceCreateInfo,
                                   nullptr, &pLogicalDevice);
    if (code != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create logical device. Code: %d.\n", code);
        return false;
    }

    vkGetDeviceQueue(pLogicalDevice, pGraphicsIndex, 0, &pGraphicsQueue);
    vkGetDeviceQueue(pLogicalDevice, pPresentIndex, 0, &pPresentQueue);

    findSurfaceCapabilities();
    if (!createSwapchain(framebufferWidth, framebufferHeight)) return false;
    if (!createPipeline()) return false;

    return true;
}

bool tkvul_initialize(const char *name, uint32_t version,
                      uint32_t framebufferWidth, uint32_t framebufferHeight)
{
    VkApplicationInfo applicationInfo = {0};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = name;
    applicationInfo.applicationVersion = version;
    applicationInfo.pEngineName = nullptr;
    applicationInfo.engineVersion = 0;
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo = {0};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &applicationInfo;

    const char *extensions[2] = {"VK_KHR_surface",
#ifdef WAYLAND
                                 "VK_KHR_wayland_surface"
#elifdef X11
    // TODO: Implement X11.
#endif
    };
    instanceInfo.enabledExtensionCount = 2;
    instanceInfo.ppEnabledExtensionNames = extensions;

    const char *layers[1] = {"VK_LAYER_KHRONOS_validation"};
    instanceInfo.enabledLayerCount = 1;
    instanceInfo.ppEnabledLayerNames = layers;

    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &pInstance);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create Vulkan instance. Code: %d.\n",
                result);
        return false;
    }

    if (!createSurface()) return false;
    if (!createDevice(framebufferWidth, framebufferHeight)) return false;
    return true;
}
