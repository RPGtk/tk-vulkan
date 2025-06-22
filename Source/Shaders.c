#include <WLAssets.h>
#include <WLVulkan.h>
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>
#include <stdio.h>
#include <string.h>

// TODO: Separate this into shared library or something so we can annihalate
// TODO: the GLSLANG dep once finished.

bool waterlily_compileShaders(const char **names, size_t count)
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

        waterlily_file_t file = {0};
        file.name = filename;
        if (input.stage == GLSLANG_STAGE_VERTEX)
            file.type = WATERLILY_VERTEX_FILE;
        if (input.stage == GLSLANG_STAGE_FRAGMENT)
            file.type = WATERLILY_FRAGMENT_FILE;
        waterlily_loadFile(&file);
        input.code = (const char *)file.content;

        glslang_shader_t *shader = glslang_shader_create(&input);
        if (!glslang_shader_preprocess(shader, &input))
        {
            puts("Failed to preprocess shader.");
            free(file.content);
            free(currentName);
            return false;
        }
        if (!glslang_shader_parse(shader, &input))
        {
            puts("Failed to parse shader.");
            free(file.content);
            free(currentName);
            return false;
        }

        glslang_program_t *program = glslang_program_create();
        glslang_program_add_shader(program, shader);
        if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT |
                                               GLSLANG_MSG_VULKAN_RULES_BIT))
        {
            puts("Failed to link shader.");
            free(file.content);
            free(currentName);
            return false;
        }
        glslang_program_SPIRV_generate(program, input.stage);
        if (glslang_program_SPIRV_get_messages(program))
        {
            puts("Failed to get SPIRV messages for shader.");
            free(file.content);
            free(currentName);
            return false;
        }

        char outputPath[waterlily_maxPathLength];
        sprintf(outputPath, "%s-%s", filename,
                (input.stage == GLSLANG_STAGE_FRAGMENT ? "frag" : "vert"));

        waterlily_file_t outputFile = {
            .name = outputPath,
            .type = WATERLILY_SPIRV_FILE,
            .size =
                glslang_program_SPIRV_get_size(program) * sizeof(unsigned int),
        };

        waterlily_writeFile(
            &outputFile,
            (const uint8_t *)glslang_program_SPIRV_get_ptr(program));

        free(file.content);
        free(currentName);
    }

    glslang_finalize_process();
    return true;
}
