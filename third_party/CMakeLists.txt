set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(spdlog)

set(thread_pool_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thread-pool CACHE STRING "Source directory for thread-pool library")
