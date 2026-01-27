cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(asp VERSION 1.0.0)

file(GLOB_RECURSE HEADERS
    include/*.hpp
)

if (ANDROID)
    file(GLOB_RECURSE OS_SOURCES "src/simd/arm/*.cpp" "src/platform/android/*.cpp" "src/platform/unix/*.cpp")
elseif(IOS OR "${CMAKE_SYSTEM_NAME}" STREQUAL "iOS")
    file(GLOB_RECURSE OS_SOURCES "src/simd/arm/*.cpp" "src/platform/ios/*.cpp" "src/platform/unix/*.cpp")
elseif (APPLE)
    file(GLOB_RECURSE OS_SOURCES "src/simd/arm/*.cpp" "src/simd/x86/*.cpp" "src/platform/macos/*.cpp" "src/platform/unix/*.cpp")
elseif (WIN32)
    file(GLOB_RECURSE OS_SOURCES "src/simd/x86/*.cpp" "src/platform/windows/*.cpp")
elseif (UNIX)
    # guesses
    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        file(GLOB_RECURSE OS_SOURCES "src/simd/x86/*.cpp" "src/platform/unix/*.cpp")
    else()
        file(GLOB_RECURSE OS_SOURCES "src/simd/arm/*.cpp" "src/platform/unix/*.cpp")
    endif()
else()
    message(FATAL_ERROR "Unsupported operating system")
endif()

file(GLOB SOURCES
    src/math/*.cpp
    src/sync/*.cpp
    src/thread/*.cpp
    src/time/*.cpp
    src/fs/*.cpp
    src/net/*.cpp
    src/*.cpp
    ${OS_SOURCES}
)

if (NOT CURRENT_CPM_VERSION)
    include(cmake/CPM.cmake)
endif()

add_library(asp STATIC ${HEADERS} ${SOURCES})

if (WIN32)
    # fix debug builds
    target_compile_definitions(${PROJECT_NAME} PRIVATE _HAS_ITERATOR_DEBUGGING=0)
endif()

CPMAddPackage("gh:geode-sdk/result@1.3.2")
CPMAddPackage("gh:fmtlib/fmt#11.2.0")

target_link_libraries(asp PUBLIC GeodeResult fmt)
target_compile_definitions(asp PRIVATE NOMINMAX=1)

target_include_directories(asp PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(asp PRIVATE include/)

install(TARGETS asp
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)

# Google Tests

option(ASP_BUILD_TESTS "Build tests for asp" OFF)

if (ASP_BUILD_TESTS)
    enable_testing()

    CPMAddPackage("gh:google/googletest@1.16.0")

    file(GLOB TEST_SOURCES tests/*.cpp)
    add_executable(asp_tests ${TEST_SOURCES})
    target_link_libraries(asp_tests PRIVATE asp gtest_main GeodeResult)
    include(GoogleTest)
    gtest_discover_tests(asp_tests)
endif()
