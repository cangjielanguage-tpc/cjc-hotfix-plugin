cmake_minimum_required(VERSION 3.22)

project(cjc_hotfix_plugin)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DCANGJIE_CODEGEN_CJNATIVE_BACKEND)

include(FetchContent)

set(CHIR_API "${CMAKE_CURRENT_SOURCE_DIR}/CHIR_API")

FetchContent_Declare(
        chir_api
        DOWNLOAD_COMMAND git clone -b dev --filter=blob:none --no-checkout --depth 1 https://gitcode.com/Cangjie/cangjie_compiler ${CHIR_API}
        COMMAND cd ${CHIR_API} && git sparse-checkout set "include" && git checkout
)

if (NOT EXISTS "${CHIR_API}/.git")
    message(STATUS "Performing sparse-checkout of include directory in Cangjie compiler repo...")
    FetchContent_MakeAvailable(chir_api)
endif ()

include_directories(
        CHIR_API/include
        ../include
        ../libs
)

set(LIB_SOURCES
        EntryPoint.cpp
        PatchableFinder.cpp
        Patcher.cpp
        TypeFilterGenerator.cpp
        PluginContext.h
        Patcher.h
        PatchableFinder.h
)

if (CMAKE_BUILD_TYPE STREQUAL "Test")
    list(APPEND LIB_SOURCES PatcherStub.cpp)
endif ()

add_library(cjc_hotfix_plugin SHARED ${LIB_SOURCES})

if (DEFINED LSP_PATH)
    target_link_directories(cjc_hotfix_plugin PRIVATE "${LSP_PATH}")
    target_link_libraries(cjc_hotfix_plugin PRIVATE "cangjie-lsp")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build mode")
    target_compile_options(cjc_hotfix_plugin PRIVATE
            -O0
    )
    target_compile_definitions(cjc_hotfix_plugin PRIVATE DEBUG)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release build mode")
    target_compile_options(cjc_hotfix_plugin PRIVATE
            -O2
    )
elseif (CMAKE_BUILD_TYPE STREQUAL "Test")
    message(STATUS "Test build mode")
    target_compile_options(cjc_hotfix_plugin PRIVATE
            -O0
    )
    target_compile_definitions(cjc_hotfix_plugin PRIVATE DEBUG)
    target_compile_definitions(cjc_hotfix_plugin PRIVATE TEST)
endif ()
