cmake_minimum_required(VERSION 3.16)

project(cjc-hotfix-plugin)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)

add_definitions(-DCANGJIE_CODEGEN_CJNATIVE_BACKEND)

include_directories(
        ${CANGJIE_PATH}/include
        libs
)

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(hotfix-plugin SHARED ${LIB_SOURCES})

if (CANGJIE_PATH)
    target_link_directories(hotfix-plugin PRIVATE "${CANGJIE_PATH}/tools/lib")
    target_link_libraries(hotfix-plugin PRIVATE "cangjie-lsp" ${CMAKE_DL_LIBS} pthread)

    # The plugin and libcangjie-lsp.dylib are deployed next to each other in tools/lib.
    if (APPLE)
        set_target_properties(hotfix-plugin PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "@loader_path"
        )
    endif ()
endif ()

# ignore specialization of std::remove_pointer for Ptr<T> class inside std namespace (see SafePointer.h)
target_compile_options(hotfix-plugin PRIVATE
    $<$<PLATFORM_ID:Darwin>:-Wno-error=invalid-specialization>
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build mode")
    target_compile_options(hotfix-plugin PRIVATE
            -O0
    )
    target_compile_definitions(hotfix-plugin PRIVATE DEBUG)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release build mode")
    target_compile_options(hotfix-plugin PRIVATE
            -O2
    )
endif ()

if (TEST)
    target_compile_definitions(hotfix-plugin PRIVATE TEST)
    if (STUB_TEST)
        message(STATUS "Stub test mode")
        target_compile_definitions(hotfix-plugin PRIVATE STUB_TEST)
    endif()
endif ()

enable_testing()
add_subdirectory(test)
