#define AGERATUM_IMPLEMENTATION
#include <Ageratum.h>
#include <Geranium.h>
#include <Primrose.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

bool geranium_compileShaders(const char **names, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        char *currentName = strdup(names[i]);

        const char *const filename = strtok(currentName, ".");
        if (filename == nullptr)
        {
            printf("File '%s' has no extension and therefore no stage can be "
                   "assumed.\n",
                   currentName);
            free(currentName);
            return false;
        }
        char *extension = strtok(nullptr, ".");

        ageratum_file_t file = {
            .basename = (char *)filename,
            .type = (strcmp(extension, "frag") == 0 ? AGERATUM_GLSL_FRAGMENT
                                                    : AGERATUM_GLSL_VERTEX),
        };
        if (!ageratum_glslToSPIRV(&file)) return false;

        free(currentName);
    }
    return true;
}

bool createShaderStage(const char *name, VkPipelineShaderStageCreateInfo *stage,
                       VkDevice logicalDevice)
{
    char filename[AGERATUM_MAX_PATH_LENGTH];
    // We only need 4, that's the length of the only file extensions being sent.
    char extension[5];
    ageratum_splitStem(name, filename, extension);

    ageratum_type_t type = strcmp(extension, "frag") == 0
                               ? AGERATUM_SPIRV_FRAGMENT
                               : AGERATUM_SPIRV_VERTEX;

    ageratum_file_t file = {
        .basename = filename,
        .type = type,
    };
    if (!ageratum_fileExists(&file) &&
        !geranium_compileShaders((const char **)&filename, 1))
        return false;

    if (!ageratum_openFile(&file, AGERATUM_READ) ||
        !ageratum_getFileSize(&file))
        return false;

    char contents[file.size];
    if (!ageratum_loadFile(&file, contents) || !ageratum_closeFile(&file))
        return false;

    VkShaderModuleCreateInfo moduleCreateInfo = {0};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = file.size;
    moduleCreateInfo.pCode = (uint32_t *)contents;

    VkShaderModule module;
    VkResult result = vkCreateShaderModule(logicalDevice, &moduleCreateInfo,
                                           nullptr, &module);
    if (result != VK_SUCCESS)
    {
        primrose_log(ERROR, "Failed to create shader module. Code: %d.",
                     result);
        return false;
    }

    *stage = (VkPipelineShaderStageCreateInfo){0};
    stage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage->stage =
        (type == AGERATUM_SPIRV_VERTEX ? VK_SHADER_STAGE_VERTEX_BIT
                                       : VK_SHADER_STAGE_FRAGMENT_BIT);
    stage->module = module;
    stage->pName = "main";

    return true;
}
