# Copyright 2020 The Dawn Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.10)

# When upgrading to CMake 3.11 we can remove DAWN_DUMMY_FILE because source-less add_library
# becomes available.
# When upgrading to CMake 3.12 we should add CONFIGURE_DEPENDS to DawnGenerator to rerun CMake in
# case any of the generator files changes. We should also remove the CACHE "" FORCE stuff to
# override options in third_party dependencies. We can also add the HOMEPAGE_URL
# entry to the project `HOMEPAGE_URL "https://dawn.googlesource.com/dawn"`

project(
    Dawn
    DESCRIPTION "Dawn, a WebGPU implementation"
    LANGUAGES C CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT CMAKE_BUILD_TYPE)
    message(WARNING "CMAKE_BUILD_TYPE not set, forcing it to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
        "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

set(DAWN_BUILD_GEN_DIR "${Dawn_BINARY_DIR}/gen")
set(DAWN_GENERATOR_DIR "${Dawn_SOURCE_DIR}/generator")
set(DAWN_SRC_DIR "${Dawn_SOURCE_DIR}/src")
set(DAWN_INCLUDE_DIR "${DAWN_SRC_DIR}/include")
set(DAWN_TEMPLATE_DIR "${DAWN_GENERATOR_DIR}/templates")

set(DAWN_DUMMY_FILE "${DAWN_SRC_DIR}/Dummy.cpp")

################################################################################
# Configuration options
################################################################################

# Default values for the backend-enabling options
set(ENABLE_D3D12 OFF)
set(ENABLE_METAL OFF)
set(ENABLE_OPENGLES OFF)
set(ENABLE_DESKTOP_GL OFF)
set(ENABLE_VULKAN OFF)
set(USE_X11 OFF)
set(BUILD_EXAMPLE OFF)
if (WIN32)
    set(ENABLE_D3D12 ON)
    if (NOT WINDOWS_STORE)
        # Enable Vulkan in win32 compilation only
        # since UWP only supports d3d
        set(ENABLE_VULKAN ON)
    endif()
elseif(APPLE)
    set(ENABLE_METAL ON)
elseif(UNIX)
    set(ENABLE_OPENGLES ON)
    set(ENABLE_DESKTOP_GL ON)
    set(ENABLE_VULKAN ON)
    set(USE_X11 ON)
endif()

# GLFW is not supported in UWP
if((WIN32 AND NOT WINDOWS_STORE) OR UNIX)
    set(DAWN_SUPPORTS_GLFW_FOR_WINDOWING ON)
endif()

# Current examples are depend on GLFW
if (DAWN_SUPPORTS_GLFW_FOR_WINDOWING)
    set(BUILD_EXAMPLE ON)
endif()

option(DAWN_ENABLE_D3D12 "Enable compilation of the D3D12 backend" ${ENABLE_D3D12})
option(DAWN_ENABLE_METAL "Enable compilation of the Metal backend" ${ENABLE_METAL})
option(DAWN_ENABLE_NULL "Enable compilation of the Null backend" ON)
option(DAWN_ENABLE_DESKTOP_GL "Enable compilation of the OpenGL backend" ${ENABLE_DESKTOP_GL})
option(DAWN_ENABLE_OPENGLES "Enable compilation of the OpenGL ES backend" ${ENABLE_OPENGLES})
option(DAWN_ENABLE_VULKAN "Enable compilation of the Vulkan backend" ${ENABLE_VULKAN})
option(DAWN_ALWAYS_ASSERT "Enable assertions on all build types" OFF)
option(DAWN_USE_X11 "Enable support for X11 surface" ${USE_X11})

option(DAWN_BUILD_EXAMPLES "Enables building Dawn's exmaples" ${BUILD_EXAMPLE})

set(DAWN_THIRD_PARTY_DIR "${Dawn_SOURCE_DIR}/third_party" CACHE STRING "Directory in which to find third-party dependencies.")

set(DAWN_GLFW_DIR "${DAWN_THIRD_PARTY_DIR}/glfw" CACHE STRING "Directory in which to find GLFW")
set(DAWN_GLM_DIR "${DAWN_THIRD_PARTY_DIR}/glm" CACHE STRING "Directory in which to find GLM")
set(DAWN_JINJA2_DIR "${DAWN_THIRD_PARTY_DIR}/jinja2" CACHE STRING "Directory in which to find Jinja2")
set(DAWN_SPIRV_CROSS_DIR "${DAWN_THIRD_PARTY_DIR}/vulkan-deps/spirv-cross/src" CACHE STRING "Directory in which to find SPIRV-Cross")
set(DAWN_SPIRV_HEADERS_DIR "${DAWN_THIRD_PARTY_DIR}/vulkan-deps/spirv-headers/src" CACHE STRING "Directory in which to find SPIRV-Headers")
set(DAWN_SPIRV_TOOLS_DIR "${DAWN_THIRD_PARTY_DIR}/vulkan-deps/spirv-tools/src" CACHE STRING "Directory in which to find SPIRV-Tools")
set(DAWN_TINT_DIR "${DAWN_THIRD_PARTY_DIR}/tint" CACHE STRING "Directory in which to find Tint")

# Much of the backend code is shared among desktop OpenGL and OpenGL ES
if (${DAWN_ENABLE_DESKTOP_GL} OR ${DAWN_ENABLE_OPENGLES})
    set(DAWN_ENABLE_OPENGL ON)
endif()

################################################################################
# Dawn's public and internal "configs"
################################################################################

# The public config contains only the include paths for the Dawn headers.
add_library(dawn_public_config INTERFACE)
target_include_directories(dawn_public_config INTERFACE
    "${DAWN_SRC_DIR}/include"
    "${DAWN_BUILD_GEN_DIR}/src/include"
)

# The internal config conatins additional path but includes the dawn_public_config include paths
add_library(dawn_internal_config INTERFACE)
target_include_directories(dawn_internal_config INTERFACE
    "${DAWN_SRC_DIR}"
    "${DAWN_BUILD_GEN_DIR}/src"
)
target_link_libraries(dawn_internal_config INTERFACE dawn_public_config)

# Compile definitions for the internal config
if (DAWN_ALWAYS_ASSERT OR $<CONFIG:Debug>)
    target_compile_definitions(dawn_internal_config INTERFACE "DAWN_ENABLE_ASSERTS")
endif()
if (DAWN_ENABLE_D3D12)
    target_compile_definitions(dawn_internal_config INTERFACE "DAWN_ENABLE_BACKEND_D3D12")
endif()
if (DAWN_ENABLE_METAL)
    target_compile_definitions(dawn_internal_config INTERFACE "DAWN_ENABLE_BACKEND_METAL")
endif()
if (DAWN_ENABLE_NULL)
    target_compile_definitions(dawn_internal_config INTERFACE "DAWN_ENABLE_BACKEND_NULL")
endif()
if (DAWN_ENABLE_DESKTOP_GL)
    target_compile_definitions(dawn_internal_config INTERFACE "DAWN_ENABLE_BACKEND_DESKTOP_GL")
endif()
if (DAWN_ENABLE_OPENGLES)
    target_compile_definitions(dawn_internal_config INTERFACE "DAWN_ENABLE_BACKEND_OPENGLES")
endif()
if (DAWN_ENABLE_OPENGL)
    target_compile_definitions(dawn_internal_config INTERFACE "DAWN_ENABLE_BACKEND_OPENGL")
endif()
if (DAWN_ENABLE_VULKAN)
    target_compile_definitions(dawn_internal_config INTERFACE "DAWN_ENABLE_BACKEND_VULKAN")
endif()
if (DAWN_USE_X11)
    target_compile_definitions(dawn_internal_config INTERFACE "DAWN_USE_X11")
endif()
if (WIN32)
    target_compile_definitions(dawn_internal_config INTERFACE "NOMINMAX" "WIN32_LEAN_AND_MEAN")
endif()

set(CMAKE_CXX_STANDARD "14")

################################################################################
# Run on all subdirectories
################################################################################

add_subdirectory(third_party)
add_subdirectory(src/common)
add_subdirectory(generator)
add_subdirectory(src/dawn)
add_subdirectory(src/dawn_platform)
add_subdirectory(src/dawn_native)
add_subdirectory(src/dawn_wire)
# TODO(dawn:269): Remove once the implementation-based swapchains are removed.
add_subdirectory(src/utils)

if (DAWN_BUILD_EXAMPLES)
    #TODO(dawn:269): Add this once implementation-based swapchains are removed.
    #add_subdirectory(src/utils)
    add_subdirectory(examples)
endif()
