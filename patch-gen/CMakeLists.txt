cmake_minimum_required(VERSION 3.16)

project(patch-gen)

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


file(GLOB_RECURSE EXE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(patch-gen ${EXE_SOURCES})

if (CANGJIE_PATH)
    set(CANGJIE_TOOLS_LIB "${CANGJIE_PATH}/tools/lib")
    target_link_directories(patch-gen PRIVATE "${CANGJIE_TOOLS_LIB}")
    target_link_libraries(patch-gen PRIVATE "cangjie-lsp" ${CMAKE_DL_LIBS} pthread)

    if (APPLE)
        set_target_properties(patch-gen PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "@loader_path/../lib"
            INSTALL_RPATH_USE_LINK_PATH FALSE
        )
    endif ()
endif ()

# ignore specialization of std::remove_pointer for Ptr<T> class inside std namespace (see SafePointer.h)
target_compile_options(patch-gen PRIVATE
    $<$<PLATFORM_ID:Darwin>:-Wno-error=invalid-specialization>
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build mode")
    target_compile_options(patch-gen PRIVATE
            -O0
    )
    target_compile_definitions(patch-gen PRIVATE DEBUG)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release build mode")
    target_compile_options(patch-gen PRIVATE
            -O2
    )
endif ()

if (TEST)
    message(STATUS "Test mode")
    target_compile_definitions(patch-gen PRIVATE TEST)
endif()
