cmake_minimum_required(VERSION 3.10)

project(flow-core VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    enable_language(OBJC)
elseif(MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

option(flow-core_US_EXTERNAL_JSON "Use an external JSON library" OFF)

if (flow-core_US_EXTERNAL_JSON)
    find_package(nlohmann_json 3.11.3 REQUIRED)
endif()

add_subdirectory(third_party)

# -----------------------------------------------------------------------------
# Library
# -----------------------------------------------------------------------------

set(flow-core_HEADERS_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
file(GLOB flow-core_HEADERS "${flow-core_HEADERS_DIR}/flow/core/*.hpp")
file(GLOB thread_pool_HEADERS "${thread_pool_SOURCE_DIR}/include/*.hpp")
list(APPEND ${flow-core_HEADERS} ${thread_pool_HEADERS})

add_library(flow-core SHARED
  src/Connection.cpp
  src/Connections.cpp
  src/Env.cpp
  src/Graph.cpp
  src/Node.cpp
  src/NodeFactory.cpp
  src/Port.cpp
  src/TypeConversion.cpp
  src/UUID.cpp

  ${flow-core_HEADERS}
)

add_library(flow-core::flow-core ALIAS flow-core)

target_include_directories(flow-core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${thread_pool_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/flow/core
)

if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  target_compile_options(flow-core PRIVATE /W4)
  target_link_libraries(flow-core PRIVATE nlohmann_json::nlohmann_json)
else()
  target_compile_options(flow-core PRIVATE -Wall -Wextra -Wpedantic -Werror)
  if(APPLE)
    target_link_libraries(flow-core PUBLIC
      "-framework CoreFoundation"
      pthread

      nlohmann_json::nlohmann_json
    )
  else()
    target_link_libraries(flow-core PUBLIC
      dl
      pthread
      uuid

      nlohmann_json::nlohmann_json
    )
  endif()
endif()

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------

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

# -----------------------------------------------------------------------------
# Install
# -----------------------------------------------------------------------------

if(flow-core_INSTALL)
  set(export_destination "${CMAKE_INSTALL_LIBDIR}/cmake/flow-core")

  # Install required headers
  install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  install(DIRECTORY ${thread_pool_SOURCE_DIR}/include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

  install(TARGETS flow-core
    EXPORT flow-core
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  export(TARGETS flow-core
    NAMESPACE flow-core::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/flow-coreTargets.cmake"
  )
  install(EXPORT flow-core DESTINATION ${export_destination} NAMESPACE flow-core:: FILE "flow-coreTargets.cmake")

  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/flow-coreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/flow-coreConfig.cmake"
    INSTALL_DESTINATION ${export_destination}
  )

  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/flow-coreConfig.cmake"
    DESTINATION ${export_destination}
  )
endif()
