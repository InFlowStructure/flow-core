function(CreateFlowModule module_name)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      set(MODULE_BINARY_DIR "linux")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      set(MODULE_BINARY_DIR "macos")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
      set(MODULE_BINARY_DIR "windows")
    else()
      message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
    endif()

    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^(x86_64|amd64|AMD64)$")
      set(MODULE_BINARY_DIR "${MODULE_BINARY_DIR}/x86_64")
    elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^(arm64|aarch64)$")
      set(MODULE_BINARY_DIR "${MODULE_BINARY_DIR}/arm64")
    elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^(i386|i686)$")
      set(MODULE_BINARY_DIR "${MODULE_BINARY_DIR}/x86")
    else()
      message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    add_custom_command(TARGET ${module_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      $<TARGET_FILE:${module_name}>
      $<TARGET_FILE_DIR:${module_name}>/${module_name}/${MODULE_BINARY_DIR}/${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
    )

    add_custom_command(TARGET ${module_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_SOURCE_DIR}/module.json
      $<TARGET_FILE_DIR:${module_name}>/${module_name}/module.json
    )

    add_custom_command(TARGET ${module_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_SOURCE_DIR}/README.md
      $<TARGET_FILE_DIR:${module_name}>/${module_name}/README.md
    )

    add_custom_command(TARGET ${module_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
      $<TARGET_FILE_DIR:${module_name}>/${module_name}/LICENSE
    )

    add_custom_command(TARGET ${module_name} POST_BUILD
      WORKING_DIRECTORY $<TARGET_FILE_DIR:${module_name}>
      COMMAND ${CMAKE_COMMAND} -E tar -cfv ${CMAKE_CURRENT_BINARY_DIR}/${module_name}.fmod --format=zip ${module_name}/
    )
endfunction()
