#define AGERATUM_IMPLEMENTATION
#include <Ageratum.h>
#include <WLVulkan.h>
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>
#include <stdio.h>
#include <string.h>

// TODO: Separate this into shared library or something so we can annihalate
// TODO: the GLSLANG dep once finished.

bool waterlily_vulkanCompileShaders(const char **names, size_t count)
{
    glslang_initialize_process();

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

        glslang_input_t input = {
            .language = GLSLANG_SOURCE_GLSL,
            .stage = GLSLANG_STAGE_VERTEX,
            .client = GLSLANG_CLIENT_VULKAN,
            .client_version = GLSLANG_TARGET_VULKAN_1_4,
            .target_language = GLSLANG_TARGET_SPV,
            .target_language_version = GLSLANG_TARGET_SPV_1_6,
            .code = nullptr,
            .default_version = 460,
            .default_profile = GLSLANG_CORE_PROFILE,
            .force_default_version_and_profile = false,
            .forward_compatible = false,
            .messages = GLSLANG_MSG_DEFAULT_BIT,
            .resource = glslang_default_resource(),
        };

        if (strcmp(extension, "frag") == 0)
            input.stage = GLSLANG_STAGE_FRAGMENT;

        ageratum_file_t file = {.filename = filename};
        if (input.stage == GLSLANG_STAGE_VERTEX)
            file.type = AGERATUM_GLSL_VERTEX;
        if (input.stage == GLSLANG_STAGE_FRAGMENT)
            file.type = AGERATUM_GLSL_FRAGMENT;
        ageratum_openFile(&file, AGERATUM_READ);
        ageratum_getFileSize(&file);

        char fileContents[file.size + 1];
        ageratum_loadFile(&file, fileContents);
        ageratum_closeFile(&file);
        input.code = fileContents;

        glslang_shader_t *shader = glslang_shader_create(&input);
        if (!glslang_shader_preprocess(shader, &input))
        {
            puts("Failed to preprocess shader.");
            free(currentName);
            return false;
        }
        if (!glslang_shader_parse(shader, &input))
        {
            puts("Failed to parse shader.");
            free(currentName);
            return false;
        }

        glslang_program_t *program = glslang_program_create();
        glslang_program_add_shader(program, shader);
        if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT |
                                               GLSLANG_MSG_VULKAN_RULES_BIT))
        {
            puts("Failed to link shader.");
            free(currentName);
            return false;
        }
        glslang_program_SPIRV_generate(program, input.stage);
        if (glslang_program_SPIRV_get_messages(program))
        {
            puts("Failed to get SPIRV messages for shader.");
            free(currentName);
            return false;
        }

        ageratum_file_t outputFile = {
            .filename = filename,
            .type =
                (input.stage == GLSLANG_STAGE_VERTEX ? AGERATUM_SPIRV_VERTEX
                                                     : AGERATUM_SPIRV_FRAGMENT),
            .size =
                glslang_program_SPIRV_get_size(program) * sizeof(unsigned int),
        };

        ageratum_openFile(&outputFile, AGERATUM_WRITE);
        ageratum_writeFile(
            &outputFile, (const char *)glslang_program_SPIRV_get_ptr(program));
        ageratum_closeFile(&outputFile);

        // free(file.content);
        free(currentName);
    }

    glslang_finalize_process();
    return true;
}
