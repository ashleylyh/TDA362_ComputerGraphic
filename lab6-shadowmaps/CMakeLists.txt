cmake_minimum_required ( VERSION 3.5 )

project ( lab6-shadowmaps )

# Find *all* shaders.
file(GLOB_RECURSE SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
)
# Separate filter for shaders.
source_group("Shaders" FILES ${SHADERS})

# Build and link executable.
add_executable ( ${PROJECT_NAME}
    lab6_main.cpp
    fbo.cpp
    ${SHADERS}
)

target_link_libraries ( ${PROJECT_NAME} labhelper )
config_build_output()
