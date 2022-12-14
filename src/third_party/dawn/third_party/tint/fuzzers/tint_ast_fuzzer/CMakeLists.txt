# Copyright 2021 The Tint Authors.
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

set(PROTOBUF_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/protobufs/tint_ast_fuzzer.proto)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/protobufs)

add_custom_command(
        OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/protobufs/tint_ast_fuzzer.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/protobufs/tint_ast_fuzzer.pb.h
        COMMAND
        "protobuf::protoc" -I=${CMAKE_CURRENT_SOURCE_DIR}/protobufs
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/protobufs ${PROTOBUF_SOURCES}
        DEPENDS ${PROTOBUF_SOURCES}
        COMMENT "Generate protobuf sources from proto definition file.")

set(LIBTINT_AST_FUZZER_SOURCES
        mt_rng.h
        mutation.h
        mutation_finder.h
        mutation_finders/replace_identifiers.h
        mutations/replace_identifier.h
        mutator.h
        node_id_map.h
        probability_context.h
        protobufs/tint_ast_fuzzer.h
        random_number_generator.h
        util.h
        ${CMAKE_CURRENT_BINARY_DIR}/protobufs/tint_ast_fuzzer.pb.h)

set(LIBTINT_AST_FUZZER_SOURCES ${LIBTINT_AST_FUZZER_SOURCES}
        mt_rng.cc
        mutation.cc
        mutation_finder.cc
        mutation_finders/replace_identifiers.cc
        mutations/replace_identifier.cc
        mutator.cc
        node_id_map.cc
        probability_context.cc
        random_number_generator.cc
        ${CMAKE_CURRENT_BINARY_DIR}/protobufs/tint_ast_fuzzer.pb.cc)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/protobufs/tint_ast_fuzzer.pb.cc PROPERTIES COMPILE_FLAGS -w)

# Add static library target.
add_library(libtint_ast_fuzzer STATIC ${LIBTINT_AST_FUZZER_SOURCES})
target_link_libraries(libtint_ast_fuzzer protobuf::libprotobuf libtint)
tint_default_compile_options(libtint_ast_fuzzer)
target_include_directories(libtint_ast_fuzzer PRIVATE ${CMAKE_BINARY_DIR})

set(AST_FUZZER_SOURCES
        cli.cc
        cli.h
        fuzzer.cc
        ../tint_common_fuzzer.cc
        ../tint_common_fuzzer.h)

set_source_files_properties(fuzzer.cc PROPERTIES COMPILE_FLAGS -Wno-missing-prototypes)

# Add libfuzzer target.
add_executable(tint_ast_fuzzer ${AST_FUZZER_SOURCES})
target_link_libraries(tint_ast_fuzzer libtint-fuzz libtint_ast_fuzzer)
tint_default_compile_options(tint_ast_fuzzer)
target_compile_definitions(tint_ast_fuzzer PRIVATE CUSTOM_MUTATOR)
target_include_directories(tint_ast_fuzzer PRIVATE ${CMAKE_BINARY_DIR})

# Add tests.
if (${TINT_BUILD_TESTS})
    set(TEST_SOURCES
            mutations/replace_identifier_test.cc)

    add_executable(tint_ast_fuzzer_unittests ${TEST_SOURCES})

    target_include_directories(
            tint_ast_fuzzer_unittests PRIVATE ${gmock_SOURCE_DIR}/include)
    target_link_libraries(tint_ast_fuzzer_unittests gmock_main libtint_ast_fuzzer)
    tint_default_compile_options(tint_ast_fuzzer_unittests)
    target_compile_options(tint_ast_fuzzer_unittests PRIVATE
            -Wno-global-constructors
            -Wno-weak-vtables
            -Wno-covered-switch-default)

    target_include_directories(tint_ast_fuzzer_unittests PRIVATE ${CMAKE_BINARY_DIR})

    add_test(NAME tint_ast_fuzzer_unittests COMMAND tint_ast_fuzzer_unittests)
endif ()
