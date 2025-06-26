#define AGERATUM_IMPLEMENTATION
#include <Ageratum.h>
#include <WLVulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool waterlily_vulkanCompileShaders(const char **names, size_t count)
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

bool waterlily_vulkanGetShader(const char *name, char **contents, size_t *size)
{
    char *currentName = strdup(name);
    const char *const filename = strtok(currentName, ".");
    const char *const extension = strtok(nullptr, ".");

    ageratum_file_t file = {
        .basename = (char *)filename,
        .type = (strcmp(extension, "frag") == 0 ? AGERATUM_SPIRV_FRAGMENT
                                                : AGERATUM_SPIRV_VERTEX),
    };
    if (!ageratum_openFile(&file, AGERATUM_READ) ||
        !ageratum_getFileSize(&file))
    {
        free(currentName);
        return false;
    }

    *contents = malloc(file.size);
    if (!ageratum_loadFile(&file, *contents) || !ageratum_closeFile(&file))
    {
        free(currentName);
        return false;
    }
    *size = file.size;

    free(currentName);
    return true;
}
