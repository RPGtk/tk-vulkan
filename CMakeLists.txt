################################################################################
## The configuration files for the Waterlily Vulkan project. This requires CMake 
## version 3.10 or higher. The following options affect this files output:
##
## Copyright (c) 2025 - the Waterlily Team
## This source code is under the GPLv3. For information on what that entails,
## please see <https://www.gnu.org/licenses/gpl-3.0.txt>.
################################################################################

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project("WLVulkan" LANGUAGES C VERSION 0.0.0.33)

include(Helpers)
run_checks(ON)
create_target(OFF RAW_SOURCES "WLVulkan" "${WATERLILY_UNDERLYING_SYSTEM}" "Shaders")
link_external(Vulkan)

add_dependencies(${PROJECT_NAME} WLLogging)
target_include_directories(${PROJECT_NAME} PRIVATE "$<TARGET_PROPERTY:WLLogging,INTERFACE_INCLUDE_DIRECTORIES>")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/Ageratum")

if(${WATERLILY_UNDERLYING})
    target_compile_definitions(${PROJECT_NAME} PRIVATE WAYLAND)
    find_package(Wayland REQUIRED)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE X11)
    # TODO: X11.
endif()
