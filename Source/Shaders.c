#define AGERATUM_IMPLEMENTATION
#include <Ageratum.h>
#include <WLVulkan.h>
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>
#include <stdio.h>
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
            .filename = filename,
            .type = (strcmp(extension, "frag") == 0 ? AGERATUM_GLSL_FRAGMENT
                                                    : AGERATUM_GLSL_VERTEX),
        };
        ageratum_glslToSPIRV(&file);

        free(currentName);
    }
    return true;
}
