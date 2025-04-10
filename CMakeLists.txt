cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(asp VERSION 0.2.0)

file(GLOB_RECURSE HEADERS
    include/*.hpp
)

if (ANDROID)
    file(GLOB_RECURSE OS_SOURCES "src/simd/arm/*.cpp")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS")
    file(GLOB_RECURSE OS_SOURCES "src/simd/arm/*.cpp")
elseif (APPLE)
    file(GLOB_RECURSE OS_SOURCES "src/simd/arm/*.cpp" "src/simd/x86/*.cpp")
elseif (WIN32)
    file(GLOB_RECURSE OS_SOURCES "src/simd/x86/*.cpp")
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

CPMAddPackage("gh:geode-sdk/result@1.3.2")

target_link_libraries(asp PRIVATE GeodeResult)
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
