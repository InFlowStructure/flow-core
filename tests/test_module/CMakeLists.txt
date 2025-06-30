cmake_minimum_required(VERSION 3.10)

project(test_module VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    enable_language(OBJC)
endif()

add_library(${PROJECT_NAME} SHARED src/register.cpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE TEST_MODULE_EXPORT)

if(MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

add_dependencies(${PROJECT_NAME} flow-core)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PUBLIC flow-core)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(MODULE_BINARY_DIR "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin") # macOS uses Darwin as its kernel name
  set(MODULE_BINARY_DIR "macos")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(MODULE_BINARY_DIR "windows")
else()
  message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^(x86_64|amd64)$")
  set(MODULE_BINARY_DIR "${MODULE_BINARY_DIR}/x86_64")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^(arm64|aarch64)$")
  set(MODULE_BINARY_DIR "${MODULE_BINARY_DIR}/arm64")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^(i386|i686)$")
  set(MODULE_BINARY_DIR "${MODULE_BINARY_DIR}/x86")
else()
  message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()


get_filename_component(lib_file $<TARGET_FILE:${PROJECT_NAME}> NAME)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  $<TARGET_FILE:${PROJECT_NAME}>
  $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}/${MODULE_BINARY_DIR}/${PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_SOURCE_DIR}/module.json
  $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}/module.json
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_SOURCE_DIR}/README.md
  $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}/README.md
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E tar cfv ${CMAKE_CURRENT_BINARY_DIR}/../${PROJECT_NAME}.flowmod --format=zip ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
)
