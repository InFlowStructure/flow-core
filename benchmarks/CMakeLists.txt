cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#=============================================================================#
# Dependencies
#=============================================================================#

set(BENCHMARK_USE_BUNDLED_GTEST OFF)
set(BENCHMARK_ENABLE_TESTING OFF)
CPMAddPackage("gh:google/benchmark@1.9.4")

#=============================================================================#
# Test Executable
#=============================================================================#

set(BENCH_EXE flow_core_bench)
add_executable(
  ${BENCH_EXE}

  event_bench.cpp
  graph_bench.cpp
  indexable_name_bench.cpp
  node_data_bench.cpp
  type_name_bench.cpp
  uuid_bench.cpp
)

if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  target_compile_options(${BENCH_EXE} PRIVATE /W4 /MP)
endif()

target_link_libraries(${BENCH_EXE} PRIVATE
  flow-core::flow-core
  benchmark::benchmark_main
)

target_include_directories(${BENCH_EXE} PRIVATE
  ${CMAKE_SOURCE_DIR}/include/flow/core
  ${thread_pool_SOURCE_DIR}/include
)

if(MSVC)
  add_custom_command(TARGET ${BENCH_EXE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJECT_NAME}>
        $<TARGET_FILE_DIR:${BENCH_EXE}>
  )
endif()
