################################################################################
## The configuration files for the tk-vulkan project. This requires CMake 
## version 3.10 or higher. The following options affect this files output:
##
## TKVUL_UNDERLYING<ON> - Toggle Wayland (ON) or X11 (OFF) mode.
##
## Copyright (c) 2025 - RPGtk Team
## This source code is under the GPLv3. For information on what that entails,
## please see <https://www.gnu.org/licenses/gpl-3.0.txt>.
################################################################################

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project("TKVulkan" LANGUAGES C VERSION 0.0.0.20)

include(Helpers)
run_checks(ON)
create_target(OFF RAW_SOURCES "TKVulkan" "${TK_UNDERLYING_SYSTEM}" "Shaders")
link_external(Vulkan COMPONENTS glslang)

# We just hope that if GLSLang is found, glslang-default-resource-limits is
# also found.
add_library(glslang-default-resource-limits SHARED IMPORTED)

cmake_path(GET Vulkan_glslang_LIBRARY PARENT_PATH GLSLANG_LIB_DIR)
set_target_properties(glslang-default-resource-limits PROPERTIES IMPORTED_LOCATION "${GLSLANG_LIB_DIR}/libglslang-default-resource-limits.so")
target_link_libraries(${PROJECT_NAME} PRIVATE glslang-default-resource-limits)

add_dependencies(${PROJECT_NAME} TKAssets)
target_link_libraries(${PROJECT_NAME} PRIVATE TKAssets)

add_dependencies(${PROJECT_NAME} TKLogging)
target_include_directories(${PROJECT_NAME} PRIVATE "$<TARGET_PROPERTY:TKLogging,INTERFACE_INCLUDE_DIRECTORIES>")

if(${TK_UNDERLYING})
    target_compile_definitions(${PROJECT_NAME} PRIVATE WAYLAND)
    find_package(Wayland REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Wayland_INCLUDE_DIRS})
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE X11)
    # TODO: X11.
endif()
