cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_executable(
  flow_core_tests

  type_name_test.cpp
  indexable_name_test.cpp

  node_test.cpp
  graph_test.cpp
)

if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  target_compile_options(flow_core_tests PRIVATE /W4)
endif()

target_link_libraries(
  flow_core_tests

  flow-core
  GTest::gtest_main
)

target_include_directories(flow_core_tests PRIVATE ../include/flow/core)

if(MSVC)
  add_custom_command(TARGET flow_core_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJECT_NAME}>
        $<TARGET_FILE_DIR:flow_core_tests>
  )
endif()

include(GoogleTest)
gtest_discover_tests(flow_core_tests)
