# Force Engine Runtime Executable (Shipping build - no editor code)

add_compile_definitions(FORCE_RUNTIME_BUILD)

set(RUNTIME_SOURCES
    Source/RuntimeApp.cpp
)

set(RUNTIME_HEADERS
    Include/RuntimeApp.h
)

add_executable(ForceRuntime ${RUNTIME_SOURCES} ${RUNTIME_HEADERS})

target_include_directories(ForceRuntime PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_link_libraries(ForceRuntime PRIVATE ForceEngine)

# Runtime uses cooked data only
add_custom_command(TARGET ForceRuntime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/CookedContent
    $<TARGET_FILE_DIR:ForceRuntime>/Content
)
