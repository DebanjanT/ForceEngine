# Force Engine Editor Executable

add_compile_definitions(FORCE_EDITOR_BUILD)

set(EDITOR_SOURCES
    Source/EditorApp.cpp
    Source/EditorLayer.cpp
)

set(EDITOR_HEADERS
    Include/EditorApp.h
    Include/EditorLayer.h
)

add_executable(ForceEditor ${EDITOR_SOURCES} ${EDITOR_HEADERS})

target_include_directories(ForceEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_link_libraries(ForceEditor PRIVATE ForceEngine)

# Set as startup project in Visual Studio
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ForceEditor)

# Copy assets to output directory
add_custom_command(TARGET ForceEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Assets
    $<TARGET_FILE_DIR:ForceEditor>/Assets
)
