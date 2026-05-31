cmake_minimum_required(VERSION 3.21)

add_executable(dispatch_benchmark dispatch_benchmark.cpp)
target_link_libraries(dispatch_benchmark PRIVATE flow-core::flow-core)
target_include_directories(dispatch_benchmark PRIVATE
  ${CMAKE_SOURCE_DIR}/include/flow/core
  ${thread_pool_SOURCE_DIR}/include
)
