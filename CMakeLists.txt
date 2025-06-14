################################################################################
## The configuration files for the tk-vulkan project. This requires CMake 
## version 3.10 or higher.
##
## Copyright (c) 2025 - RPGtk Team
## This source code is under the GPLv3. For information on what that entails,
## please see <https://www.gnu.org/licenses/gpl-3.0.txt>.
################################################################################

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project("tkvulkan" LANGUAGES C VERSION 0.0.0.0)

# We only support Linux. Everything else can go kick rocks.
if(NOT UNIX OR APPLE)
    message(FATAL_ERROR "This operating system is not supported by tk-vulkan.")
endif()

# https://en.wikipedia.org/wiki/C23_(C_standard_revision)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(TKVUL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source")
set(TKVUL_SOURCES "${TKVUL_SOURCE_DIR}/TKVulkan.c")

add_library(${PROJECT_NAME} STATIC ${TKVUL_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/Include")

target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Werror -Wpedantic -Wextra)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    # https://cmake.org/cmake/help/latest/variable/CMAKE_EXPORT_COMPILE_COMMANDS.html
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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