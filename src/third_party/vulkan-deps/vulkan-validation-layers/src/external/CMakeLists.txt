# ~~~
# Copyright (c) 2018,2021 Valve Corporation
# Copyright (c) 2018,2021 LunarG, Inc.
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
# ~~~

# Add your optional dependencies in this "external" directory.

# googletest is an optional external dependency for this repo.
if(BUILD_TESTS)
    # Attempt to enable googletest if available.

    # Suppress all warnings from external projects (i.e.: in this directory's scope).
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS "-w")
    endif()

    if(TARGET gtest_main)
        # Already enabled as a target (perhaps by a project enclosing this one)
        message(STATUS "Vulkan-ValidationLayers/external: " "googletest already configured - using it")
    elseif(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/googletest")
        # The googletest directory exists, so enable it as a target.
        message(STATUS "Vulkan-ValidationLayers/external: " "googletest found - configuring it for tests")
        set(BUILD_GTEST ON CACHE BOOL "Builds the googletest subproject")
        set(BUILD_GMOCK OFF CACHE BOOL "Builds the googlemock subproject")
        set(gtest_force_shared_crt ON CACHE BOOL "Link gtest runtimes dynamically")
        set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries")
        # EXCLUDE_FROM_ALL keeps the install target from installing GTEST files.
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/googletest" EXCLUDE_FROM_ALL)
    else()
        message(SEND_ERROR "Vulkan-ValidationLayers/external: " "Google Test was not found.  "
                           "Provide Google Test in external/googletest or set BUILD_TESTS=OFF")
    endif()

    if (WIN32)
        set_target_properties(gtest PROPERTIES FOLDER ${LAYERS_HELPER_FOLDER})
        set_target_properties(gtest_main PROPERTIES FOLDER ${LAYERS_HELPER_FOLDER})
    endif()
endif()
