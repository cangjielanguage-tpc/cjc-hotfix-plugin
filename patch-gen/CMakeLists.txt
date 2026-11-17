cmake_minimum_required(VERSION 3.16)

project(patch-gen)

set(CMAKE_CXX_STANDARD 17)
# Read the shared version list; its last entry is the default.
set(SDK_VERSIONS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${SDK_VERSIONS_FILE}")
file(STRINGS "${SDK_VERSIONS_FILE}" SDK_VERSION_ROWS)
set(SDK_VERSIONS "")
foreach(ROW IN LISTS SDK_VERSION_ROWS)
    string(STRIP "${ROW}" VERSION)
    if (VERSION STREQUAL "")
        continue()
    endif()
    if (NOT VERSION MATCHES "^[0-9]+$" OR VERSION IN_LIST SDK_VERSIONS)
        message(FATAL_ERROR "Invalid or duplicate SDK version: ${VERSION}")
    endif()
    list(APPEND SDK_VERSIONS "${VERSION}")
endforeach()
if (NOT SDK_VERSIONS)
    message(FATAL_ERROR "No SDK versions in ${SDK_VERSIONS_FILE}")
endif()
list(GET SDK_VERSIONS -1 DEFAULT_SDK_VERSION)
set(HOTFIX_SDK_VERSION "${DEFAULT_SDK_VERSION}" CACHE STRING "Cangjie SDK compatibility version")
set_property(CACHE HOTFIX_SDK_VERSION PROPERTY STRINGS ${SDK_VERSIONS})
if (NOT HOTFIX_SDK_VERSION IN_LIST SDK_VERSIONS)
    message(FATAL_ERROR "Unsupported HOTFIX_SDK_VERSION: ${HOTFIX_SDK_VERSION}")
endif()

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
target_compile_definitions(patch-gen PRIVATE HOTFIX_SDK_VERSION=${HOTFIX_SDK_VERSION})


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
