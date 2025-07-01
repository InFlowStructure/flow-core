cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#=============================================================================#
# Dependencies
#=============================================================================#

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
CPMAddPackage("gh:google/googletest@1.15.2")

#=============================================================================#
# Add Test Module
#=============================================================================#

add_subdirectory(test_module)

#=============================================================================#
# Test Executable
#=============================================================================#

set(TEST_EXE flow_core_tests)
add_executable(
  ${TEST_EXE}

  factory_test.cpp
  graph_test.cpp
  indexable_name_test.cpp
  node_test.cpp
  type_name_test.cpp
  module_test.cpp
)

if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  target_compile_options(${TEST_EXE} PRIVATE /W4 /MP)
endif()

target_link_libraries(${TEST_EXE}
  flow-core
  GTest::gtest_main
)

target_include_directories(${TEST_EXE} PRIVATE
  ${CMAKE_SOURCE_DIR}/include/flow/core
  ${thread_pool_SOURCE_DIR}/include
)

if(MSVC)
  add_custom_command(TARGET flow_core_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJECT_NAME}>
        $<TARGET_FILE_DIR:${TEST_EXE}>
  )
endif()

include(GoogleTest)
gtest_discover_tests(${TEST_EXE})
