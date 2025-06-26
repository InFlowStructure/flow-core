cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#=============================================================================#
# Fetch testing dependencies
#=============================================================================#

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

#=============================================================================#
# Test Module
#=============================================================================#

add_subdirectory(test_module)

#=============================================================================#
# Test Executable
#=============================================================================#

add_executable(
  flow_core_tests

  factory_test.cpp
  graph_test.cpp
  indexable_name_test.cpp
  node_test.cpp
  type_name_test.cpp
  module_test.cpp
)

if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  target_compile_options(flow_core_tests PRIVATE /W4 /MP)
endif()

target_link_libraries(
  flow_core_tests

  flow-core
  GTest::gtest_main
)

target_include_directories(flow_core_tests PRIVATE
  ../include/flow/core
  ${thread_pool_SOURCE_DIR}/include
)

if(MSVC)
  add_custom_command(TARGET flow_core_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJECT_NAME}>
        $<TARGET_FILE_DIR:flow_core_tests>
  )
endif()

include(GoogleTest)
gtest_discover_tests(flow_core_tests)
