#include <Geranium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

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
static uint32_t pImageCount = 0;
static VkImageView *pSwapchainImages = nullptr;
static VkFramebuffer *pSwapchainFramebuffers = nullptr;

static VkRenderPass pRenderpass = nullptr;
static VkPipelineLayout pPipelineLayout = nullptr;
static VkPipeline pGraphicsPipeline = nullptr;
static VkCommandPool pCommandPool;
static VkCommandBuffer *pCommandBuffers;

static VkSemaphore *pImageAvailableSemaphores;
static VkSemaphore *pRenderFinishedSemaphores;
static VkFence *pFences;

static const int MAX_FRAMES_IN_FLIGHT = 2;
static uint32_t currentFrame = 0;

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

    pImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
        pImageCount > capabilities.maxImageCount)
        pImageCount = capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = pSurface;
    createInfo.minImageCount = pImageCount;
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

    vkGetSwapchainImagesKHR(pLogicalDevice, pSwapchain, &pImageCount, nullptr);
    VkImage *images = malloc(sizeof(VkImage) * pImageCount);
    pSwapchainImages = malloc(sizeof(VkImageView) * pImageCount);
    pSwapchainFramebuffers = malloc(sizeof(VkFramebuffer) * pImageCount);
    vkGetSwapchainImagesKHR(pLogicalDevice, pSwapchain, &pImageCount, images);

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

    for (size_t i = 0; i < pImageCount; i++)
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

VkSurfaceFormatKHR *getSurfaceFormats(VkPhysicalDevice device)
{
    if (pFormats != nullptr) return pFormats;

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, pSurface, &pFormatCount,
                                         nullptr);
    if (pFormatCount == 0) return nullptr;
    pFormats = malloc(sizeof(VkSurfaceFormatKHR) * pFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, pSurface, &pFormatCount,
                                         pFormats);
    return pFormats;
}

VkPresentModeKHR *getSurfaceModes(VkPhysicalDevice device)
{
    if (pModes != nullptr) return pModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, pSurface, &pModeCount,
                                              nullptr);
    if (pModeCount == 0) return nullptr;
    pModes = malloc(sizeof(VkPresentModeKHR) * pModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, pSurface, &pModeCount,
                                              pModes);
    return pModes;
}

bool createPipeline(uint32_t framebufferWidth, uint32_t framebufferHeight)
{
    size_t vertexSize, fragmentSize;
    uint32_t *vertexBytes, *fragmentBytes;
    waterlily_vulkanGetShader("default.vert", (char **)&vertexBytes,
                              &vertexSize);
    waterlily_vulkanGetShader("default.frag", (char **)&fragmentBytes,
                              &fragmentSize);

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

    VkPipelineDynamicStateCreateInfo dynamicState = {0};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkExtent2D extent = getSurfaceExtent(framebufferWidth, framebufferHeight);
    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D){0, 0};
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewportState = {0};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {0};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;            // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(pLogicalDevice, &pipelineLayoutInfo, nullptr,
                               &pPipelineLayout) != VK_SUCCESS)
    {
        fprintf(stderr, "Faield to create pipeline layout.\n");
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pPipelineLayout;
    pipelineInfo.renderPass = pRenderpass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(pLogicalDevice, VK_NULL_HANDLE, 1,
                                  &pipelineInfo, nullptr,
                                  &pGraphicsPipeline) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create graphics pipeline.\n");
        return false;
    }

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

    if (getSurfaceFormats(device) == nullptr ||
        getSurfaceModes(device) == nullptr)
    {
        fprintf(stderr, "Failed to find surface format/present modes.\n");
        return 0;
    }

    // TODO: Extra grading to be done here.

    return score;
}

bool createRenderpass(void)
{
    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = pFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {0};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(pLogicalDevice, &renderPassInfo, nullptr,
                           &pRenderpass) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create renderpass.\n");
        return false;
    }

    return true;
}

bool createFramebuffers(uint32_t framebufferWidth, uint32_t framebufferHeight)
{
    VkExtent2D extent = getSurfaceExtent(framebufferWidth, framebufferHeight);
    for (size_t i = 0; i < pImageCount; i++)
    {
        VkImageView attachments[] = {pSwapchainImages[i]};

        VkFramebufferCreateInfo framebufferInfo = {0};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pRenderpass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(pLogicalDevice, &framebufferInfo, nullptr,
                                &pSwapchainFramebuffers[i]) != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to create framebuffer.\n");
            return false;
        }
    }
    return true;
}

bool createCommandBuffers(void)
{
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = pGraphicsIndex;

    if (vkCreateCommandPool(pLogicalDevice, &poolInfo, nullptr,
                            &pCommandPool) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create command pool.\n");
        return false;
    }

    pCommandBuffers = malloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    if (vkAllocateCommandBuffers(pLogicalDevice, &allocInfo, pCommandBuffers) !=
        VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create command buffer.\n");
        return false;
    }

    return true;
}

bool createSyncObjects(void)
{
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    pImageAvailableSemaphores =
        malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    pRenderFinishedSemaphores =
        malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    pFences = malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(pLogicalDevice, &semaphoreInfo, nullptr,
                              &pImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(pLogicalDevice, &semaphoreInfo, nullptr,
                              &pRenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(pLogicalDevice, &fenceInfo, nullptr, &pFences[i]) !=
                VK_SUCCESS)
        {
            fprintf(stderr, "Failed to create sync object.\n");
            return false;
        }
    }
    return true;
}

bool recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                         uint32_t framebufferWidth, uint32_t framebufferHeight)
{
    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to begin command buffer.\n");
        return false;
    }

    VkRenderPassBeginInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pRenderpass;
    renderPassInfo.framebuffer = pSwapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};

    VkExtent2D extent = getSurfaceExtent(framebufferWidth, framebufferHeight);
    renderPassInfo.renderArea.extent = extent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pGraphicsPipeline);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to end command buffer.\n");
        return false;
    }

    return true;
}

bool createDevice(uint32_t framebufferWidth, uint32_t framebufferHeight)
{
    uint32_t physicalCount = 0;
    vkEnumeratePhysicalDevices(pInstance, &physicalCount, nullptr);
    VkPhysicalDevice *physicalDevices =
        malloc(sizeof(VkPhysicalDevice) * physicalCount);
    vkEnumeratePhysicalDevices(pInstance, &physicalCount, physicalDevices);

    const size_t extensionCount = 1;
    const char *extensions[1] = {"VK_KHR_swapchain"};

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
    if (!createRenderpass()) return false;
    if (!createPipeline(framebufferWidth, framebufferHeight)) return false;
    if (!createFramebuffers(framebufferWidth, framebufferHeight)) return false;
    if (!createCommandBuffers()) return false;
    if (!createSyncObjects()) return false;

    return true;
}

bool waterlily_vulkanCreate(const char *name, uint32_t version)
{
    VkApplicationInfo applicationInfo = {0};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = name;
    applicationInfo.applicationVersion = version;
    applicationInfo.pEngineName = nullptr;
    applicationInfo.engineVersion = 0;
    applicationInfo.apiVersion = VK_API_VERSION_1_3;

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

    return true;
}

bool waterlily_vulkanInitialize(uint32_t framebufferWidth,
                                uint32_t framebufferHeight)
{
    if (!createDevice(framebufferWidth, framebufferHeight)) return false;
    return true;
}

bool waterlily_vulkanCreateSurface(void **data)
{
    VkSurfaceKHR waterlily_vulkanSurfaceCreate(VkInstance instance,
                                               void **data);
    pSurface = waterlily_vulkanSurfaceCreate(pInstance, data);
    if (pSurface == nullptr) return false;
    return true;
}

void cleanupSwapchain(void)
{
    for (size_t i = 0; i < pImageCount; i++)
    {
        vkDestroyFramebuffer(pLogicalDevice, pSwapchainFramebuffers[i],
                             nullptr);
        vkDestroyImageView(pLogicalDevice, pSwapchainImages[i], nullptr);
    }
    vkDestroySwapchainKHR(pLogicalDevice, pSwapchain, nullptr);
}

bool recreateSwapchain(uint32_t framebufferWidth, uint32_t framebufferHeight)
{
    vkDeviceWaitIdle(pLogicalDevice);

    cleanupSwapchain();
    if (!createSwapchain(framebufferWidth, framebufferHeight)) return false;
    if (!createFramebuffers(framebufferWidth, framebufferHeight)) return false;

    return true;
}

bool waterlily_vulkanRenderFrame(uint32_t framebufferWidth,
                                 uint32_t framebufferHeight)
{
    vkWaitForFences(pLogicalDevice, 1, &pFences[currentFrame], VK_TRUE,
                    UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        pLogicalDevice, pSwapchain, UINT64_MAX,
        pImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        if (!recreateSwapchain(framebufferWidth, framebufferHeight))
            return false;
        return true;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        fprintf(stderr, "Failed to acquire swapchain image.\n");
        return false;
    }

    vkResetFences(pLogicalDevice, 1, &pFences[currentFrame]);

    vkResetCommandBuffer(pCommandBuffers[currentFrame], 0);
    if (!recordCommandBuffer(pCommandBuffers[currentFrame], imageIndex,
                             framebufferWidth, framebufferHeight))
        return false;

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {pImageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &pCommandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {pRenderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    if (vkQueueSubmit(pGraphicsQueue, 1, &submitInfo, pFences[currentFrame]) !=
        VK_SUCCESS)
    {
        fprintf(stderr, "Failed to submit to the queue.\n");
        return false;
    }

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {pSwapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(pPresentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        if (!recreateSwapchain(framebufferWidth, framebufferHeight))
            return false;
    }
    else if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to present swapchain image.\n");
        return false;
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}

bool waterlily_vulkanSync(void)
{
    return vkDeviceWaitIdle(pLogicalDevice) == VK_SUCCESS;
}
