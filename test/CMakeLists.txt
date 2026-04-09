cmake_minimum_required(VERSION 3.22)

project(cjc_hotfix_plugin_test)

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
)

# for windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

include_directories(
        ../include
        ../CHIR_API
        ${source_dir}/googletest/include
)

add_executable(cjc_hotfix_plugin_test
        ../src/TypeFilterGenerator.cpp
        TypeFilterGeneratorTest.cpp
)

target_compile_definitions(cjc_hotfix_plugin_test PRIVATE DEBUG)
target_link_libraries(cjc_hotfix_plugin_test PRIVATE gtest_main)

enable_testing()
add_test(NAME cjc_hotfix_plugin_test
        COMMAND cjc_hotfix_plugin_test)
