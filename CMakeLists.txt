cmake_minimum_required(VERSION 3.10)

if(PROJECT_NAME)
  option(flow-core_BUILD_TESTS "Build tests for flow-core" OFF)
else()
  option(flow-core_BUILD_TESTS "Build tests for flow-core" ON)
endif()

project(flow-core VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    enable_language(OBJC)
elseif(MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

find_package(nlohmann_json CONFIG QUIET)
if (NOT nlohmann_json_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    nlohmann_json OVERRIDE_FIND_PACKAGE
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

add_subdirectory(third_party)

add_library(${PROJECT_NAME} SHARED
  src/Connection.cpp
  src/Connections.cpp
  src/Env.cpp
  src/Graph.cpp
  src/Log.cpp
  src/Node.cpp
  src/NodeFactory.cpp
  src/Port.cpp
  src/TypeConversion.cpp
  src/UUID.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${thread_pool_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/flow/core)

if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    nlohmann_json::nlohmann_json
    spdlog::spdlog_header_only
  )
elseif(APPLE)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    "-framework CoreFoundation"
    pthread

    nlohmann_json::nlohmann_json
    spdlog::spdlog_header_only
  )
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    dl
    pthread
    uuid

    nlohmann_json::nlohmann_json
    spdlog::spdlog_header_only
  )
endif()

if (flow-core_BUILD_TESTS)
  enable_testing()

  find_package(GTest CONFIG QUIET)
  if (NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
      googletest OVERRIDE_FIND_PACKAGE
      URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
      GIT_TAG v1.15.2
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()

  add_subdirectory(tests)
endif()
