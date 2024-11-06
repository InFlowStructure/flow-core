if (NOT flow-core_USE_EXTERNAL_JSON)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory(json)
endif()

set(thread_pool_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thread-pool CACHE STRING "Source directory for thread-pool library")
