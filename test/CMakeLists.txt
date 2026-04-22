include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
)

# for windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

include(GoogleTest)

include_directories(
    ../src
)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(hotfix_plugin_test "${TEST_SOURCES}")

target_link_directories(hotfix_plugin_test PRIVATE "${CANGJIE_PATH}/tools/lib")
target_link_libraries(hotfix_plugin_test PRIVATE "cangjie-lsp")

target_link_libraries(hotfix_plugin_test PRIVATE hotfix-plugin GTest::gtest_main)

gtest_discover_tests(hotfix_plugin_test)
