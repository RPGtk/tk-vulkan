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
project("tkvulkan" LANGUAGES C VERSION 0.0.0.7)

# We only support Linux. Everything else can go kick rocks.
if(NOT UNIX OR APPLE)
    message(FATAL_ERROR "This operating system is not supported by tk-vulkan.")
endif()

option(TKVUL_UNDERLYING "Toggle Wayland (ON) or X11 (OFF) mode." ON)

# https://en.wikipedia.org/wiki/C23_(C_standard_revision)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(TKVUL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source")
set(TKVUL_SOURCES "${TKVUL_SOURCE_DIR}/TKVulkan.c")
if(${TKVUL_UNDERLYING})
    list(APPEND TKVUL_SOURCES "${TKVUL_SOURCE_DIR}/Targets/Wayland.c")
else()
    list(APPEND TKVUL_SOURCES "${TKVUL_SOURCE_DIR}/Targets/X11.c")
endif()

add_library(${PROJECT_NAME} STATIC ${TKVUL_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/Include")

target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Werror -Wpedantic -Wextra)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    # https://cmake.org/cmake/help/latest/variable/CMAKE_EXPORT_COMPILE_COMMANDS.html
    set_target_properties(${PROJECT_NAME} PROPERTIES EXPORT_COMPILE_COMMANDS ON)

    target_compile_options(${PROJECT_NAME} PUBLIC -Og -g3 -ggdb -fsanitize=address 
        -fsanitize=pointer-compare  -fsanitize=leak -fsanitize=pointer-subtract 
        -fsanitize=undefined)
    target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=address 
        -fsanitize=undefined)

    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        # https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html
        target_compile_options(${PROJECT_NAME} PUBLIC -fanalyzer)
    endif()
else()
    # https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html#index-march-15
    # https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html#index-mtune-17
    target_compile_options(${PROJECT_NAME} PUBLIC -march=native -mtune=native 
        -Ofast -flto)
    target_link_options(${PROJECT_NAME} PUBLIC -Ofast -flto)
endif()

find_package(Vulkan REQUIRED COMPONENTS glslang)
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})

if(${TKWIN_UNDERLYING})
    target_compile_definitions(${PROJECT_NAME} PRIVATE WAYLAND)
    find_package(Wayland REQUIRED)
    target_include_directories(${PROJECT_NAME} PUBLIC ${Wayland_INCLUDE_DIRS})
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE X11)
    # TODO: X11.
endif()

file(COPY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Include/TKVulkan.h" "${CMAKE_CURRENT_BINARY_DIR}/TKVulkan.h")
