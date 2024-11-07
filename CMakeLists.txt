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
# Options
# -----------------------------------------------------------------------------

option(flow-core_BUILD_TESTS "Build tests - requires gtest" OFF)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

option(flow-core_USE_EXTERNAL_JSON "Use an external JSON library" OFF)

if (flow-core_USE_EXTERNAL_JSON)
    find_package(nlohmann_json 3.11.3 REQUIRED)
else()
  set(JSON_INSTALL ${flow-core_INSTALL})
endif()

add_subdirectory(third_party)

# -----------------------------------------------------------------------------
# Library
# -----------------------------------------------------------------------------

set(flow-core_HEADERS_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
file(GLOB flow-core_HEADERS "${flow-core_HEADERS_DIR}/flow/core/*.hpp")
file(GLOB thread_pool_HEADERS "${thread_pool_SOURCE_DIR}/include/*.hpp")
list(APPEND ${flow-core_HEADERS} ${thread_pool_HEADERS})

add_library(${PROJECT_NAME} SHARED
  src/Connection.cpp
  src/Connections.cpp
  src/Env.cpp
  src/Graph.cpp
  src/Node.cpp
  src/NodeFactory.cpp
  src/Port.cpp
  src/TypeConversion.cpp
  src/UUID.cpp

#  ${flow-core_HEADERS}
)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${thread_pool_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${thread_pool_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/flow/core
)

if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
  target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  if(APPLE)
    target_link_libraries(${PROJECT_NAME} PUBLIC
      "-framework CoreFoundation"
      pthread

      nlohmann_json::nlohmann_json
    )
  else()
    target_link_libraries(${PROJECT_NAME} PUBLIC
      dl
      pthread
      uuid

      nlohmann_json::nlohmann_json
    )
  endif()
endif()

# -----------------------------------------------------------------------------
# Install
# -----------------------------------------------------------------------------

if(flow-core_INSTALL)
  set(export_destination "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
  set(export_targets ${PROJECT_NAME})

  if (NOT flow-core_USE_EXTERNAL_JSON)
    list(APPEND export_targets nlohmann_json)
  endif()

  install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  install(DIRECTORY ${thread_pool_SOURCE_DIR}/include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

  install(TARGETS ${export_targets}
    EXPORT ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  export(TARGETS ${export_targets}
    NAMESPACE ${PROJECT_NAME}::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake"
  )
  install(EXPORT ${PROJECT_NAME} DESTINATION ${export_destination} NAMESPACE ${PROJECT_NAME}:: FILE "${PROJECT_NAME}Targets.cmake")

  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION ${export_destination}
  )

  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    DESTINATION ${export_destination}
  )

  write_basic_package_version_file(${PROJECT_NAME}ConfigVersion.cmake COMPATIBILITY SameMajorVersion)
endif()

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------

if (flow-core_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
