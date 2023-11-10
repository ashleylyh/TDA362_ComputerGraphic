cmake_minimum_required ( VERSION 3.5 )

project ( computer-graphics-labs )
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)

if(UNIX)
    set(OpenGL_GL_PREFERENCE "GLVND")
endif(UNIX)

# For windows we use our bundled binaries.
if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/external/embree2")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/external/sdl2")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/external/glew")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/external/glm")
endif(WIN32)

macro(config_build_output)
    if(MSVC)
        set(DLL_DIRECTORIES "${CMAKE_SOURCE_DIR}/external/bin")
        set(MSVC_RUNTIME_DIR "${CMAKE_SOURCE_DIR}/bin")
        set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY                "${MSVC_RUNTIME_DIR}" )
        set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${MSVC_RUNTIME_DIR}" )
        set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${MSVC_RUNTIME_DIR}" )
        set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${MSVC_RUNTIME_DIR}" )
        set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${MSVC_RUNTIME_DIR}" )
        set(vs_user_file "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.vcxproj.user")
        string(REGEX REPLACE "v([0-9][0-9])([0-9])" "\\1.\\2" "VS_TOOLSET_VERSION" "${CMAKE_VS_PLATFORM_TOOLSET}")
        configure_file("${CMAKE_SOURCE_DIR}/VSUserTemplate.user" "${vs_user_file}" @ONLY)
    endif(MSVC)
endmacro(config_build_output)



add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
set ( CMAKE_EXPORT_COMPILE_COMMANDS ON )
set ( CMAKE_CXX_STANDARD 11 )
if( NOT CMAKE_BUILD_TYPE )
  set ( CMAKE_BUILD_TYPE DEBUG )
endif()

add_subdirectory ( labhelper )
add_subdirectory ( lab1-rasterization )
add_subdirectory ( lab2-textures )
add_subdirectory ( lab3-camera )
add_subdirectory ( lab4-shading )
add_subdirectory ( lab5-rendertotexture )
add_subdirectory ( lab6-shadowmaps )
add_subdirectory ( pathtracer )
add_subdirectory ( project )
