#include <Primrose.h>
#include <vulkan/vulkan.h>

// Contained in Shaders.c.
extern bool createShaderStage(const char *, VkPipelineShaderStageCreateInfo *,
                              VkDevice);

static VkPipelineLayout pPipelineLayout = nullptr;
static VkPipeline pGraphicsPipeline = nullptr;

VkRenderPass gRenderpass = nullptr;

static VkPipelineViewportStateCreateInfo
getViewport(const VkExtent2D *const extent)
{
    VkViewport viewport = {0};
    viewport.width = (float)extent->width;
    viewport.height = (float)extent->height;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.extent = *extent;

    VkPipelineViewportStateCreateInfo viewportState = {0};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    return viewportState;
}

static VkPipelineVertexInputStateCreateInfo createInput(void)
{
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    return vertexInputInfo;
}

static VkPipelineInputAssemblyStateCreateInfo createAssembly(void)
{
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    return inputAssembly;
}

static VkPipelineRasterizationStateCreateInfo createRasterizer(void)
{
    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    return rasterizer;
}

static VkPipelineMultisampleStateCreateInfo createMultisampling(void)
{
    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    return multisampling;
}

static VkPipelineColorBlendStateCreateInfo createColorBlend(void)
{
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending = {0};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    return colorBlending;
}

static VkAttachmentDescription createColorAttachment(VkFormat format)
{
    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    return colorAttachment;
}

static void createSubpass(VkSubpassDescription *description,
                          VkSubpassDependency *dependency)
{
    VkAttachmentReference colorAttachmentRef = {0};
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    description->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    description->colorAttachmentCount = 1;
    description->pColorAttachments = &colorAttachmentRef;

    dependency->srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency->srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency->dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency->dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
}

static bool createLayout(const VkDevice device)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo,
                                             nullptr, &pPipelineLayout);
    if (result != VK_SUCCESS)
    {
        primrose_log(ERROR, "Failed to create pipeline layout. Code: %d.",
                     result);
        return false;
    }
    primrose_log(VERBOSE_OK, "Created pipeline layout.");
    return true;
}

static bool createRenderpass(VkFormat format, const VkDevice device)
{
    VkAttachmentDescription colorAttachment = createColorAttachment(format);

    VkSubpassDescription description = {0};
    VkSubpassDependency dependency = {0};
    createSubpass(&description, &dependency);

    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &description;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result =
        vkCreateRenderPass(device, &renderPassInfo, nullptr, &gRenderpass);
    if (result != VK_SUCCESS)
    {
        primrose_log(ERROR, "Failed to create renderpass. Code: %d.", result);
        return false;
    }
    primrose_log(VERBOSE_OK, "Created renderpass.");

    return true;
}

bool createPipeline(const VkExtent2D *const extent, const VkDevice device,
                    VkFormat format)
{
    VkPipelineShaderStageCreateInfo stages[2];
    createShaderStage("default.vert", &stages[0], device);
    createShaderStage("default.frag", &stages[1], device);

    VkPipelineVertexInputStateCreateInfo input = createInput();
    VkPipelineInputAssemblyStateCreateInfo assembly = createAssembly();
    VkPipelineViewportStateCreateInfo viewport = getViewport(extent);

    VkPipelineRasterizationStateCreateInfo rasterizer = createRasterizer();
    VkPipelineMultisampleStateCreateInfo multisampling = createMultisampling();
    VkPipelineColorBlendStateCreateInfo colorBlend = createColorBlend();

    createLayout(device);
    createRenderpass(format, device);

    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &input;
    pipelineInfo.pInputAssemblyState = &assembly;
    pipelineInfo.pViewportState = &viewport;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.layout = pPipelineLayout;
    pipelineInfo.renderPass = gRenderpass;

    VkResult result = vkCreateGraphicsPipelines(
        device, nullptr, 1, &pipelineInfo, nullptr, &pGraphicsPipeline);
    if (result != VK_SUCCESS)
    {
        primrose_log(ERROR, "Failed to create graphics pipeline. Code: %d.",
                     result);
        return false;
    }
    primrose_log(VERBOSE_OK, "Created graphics pipeline.");

    vkDestroyShaderModule(device, stages[0].module, nullptr);
    vkDestroyShaderModule(device, stages[1].module, nullptr);
    return true;
}

void beginRenderpass(VkFramebuffer framebuffer, VkCommandBuffer buffer,
                     const VkExtent2D *const extent)
{
    VkRenderPassBeginInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = gRenderpass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
    renderPassInfo.renderArea.extent = *extent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pGraphicsPipeline);
}
