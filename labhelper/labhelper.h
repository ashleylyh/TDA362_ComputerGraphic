#pragma once

// This file contains utility functions to be used for the labs in computer
// graphics at chalmers, they are not covered by any particular license...

#include <glm/glm.hpp>

#include <string>
#include <cassert>

#include <SDL.h>
#undef main
#include <GL/glew.h>

// Sometimes it exists, sometimes not...
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define ENSURE_INITIALIZE_ONLY_ONCE()                                                                        \
	do                                                                                                       \
	{                                                                                                        \
		static bool initialized = false;                                                                     \
		if(initialized)                                                                                      \
		{                                                                                                    \
			labhelper::fatal_error("initialize must only be called once when the application starts!", "");  \
		}                                                                                                    \
		initialized = true;                                                                                  \
	} while(0)


///////////////////////////////////////////////////////////////////////////
/// This macro checks for GL errors using glGetError().
///
/// If you're unsure where to put this, place it after every call to GL. If a
/// debugger is attached, CHECK_GL_ERROR() will break on the offending line, and
/// print the file and line location in an MSVC compatible format on the debug
/// output and console.
///
/// Note: CHECK_GL_ERROR() will report any errors since the last call to
/// glGetError()! If CHECK_GL_ERROR() reports an error, you *must* consider all
/// calls to GL since the last CHECK_GL_ERROR() (or call to glGetError())!
///
/// Note: the macro _cannot_ be used between glBegin() and glEnd(), as stated in
/// the OpenGL standard.
///
/// Example (we're looking for an error at either glClearColor() or glClear()):
///	CHECK_GL_ERROR(); // catch previous errors
///	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
///	CHECK_GL_ERROR(); // see if glClearColor() generated an error
///	glClear(GL_COLOR_BUFFER_BIT);
///	CHECK_GL_ERROR(); // see if glClear() generated an error
///////////////////////////////////////////////////////////////////////////
#define CHECK_GL_ERROR()                                                                                     \
	{                                                                                                        \
		labhelper::checkGLError(__FILE__, __LINE__) && (__debugbreak(), 1);                                  \
	}

namespace labhelper
{
#if !defined(_WIN32)
#define __debugbreak() assert(false)
#endif

///////////////////////////////////////////////////////////////////////////
/// Internal function used by macro CHECK_GL_ERROR, use that instead.
bool checkGLError(const char* file, int line);

///////////////////////////////////////////////////////////////////////////
/// Print GL version/vendor and renderer.
/// Ensure that we've got OpenGL 3.0 (GLEW_VERSION_3_0). Bail if we don't.
///////////////////////////////////////////////////////////////////////////
void startupGLDiagnostics();

///////////////////////////////////////////////////////////////////////////
/// Initialize OpenGL debug messages.
///////////////////////////////////////////////////////////////////////////
void setupGLDebugMessages();

///////////////////////////////////////////////////////////////////////////
/// Reports error and aborts execution
///////////////////////////////////////////////////////////////////////////
void fatal_error(std::string errorString, std::string title = std::string());

///////////////////////////////////////////////////////////////////////////
/// Reports error but continues execution
///////////////////////////////////////////////////////////////////////////
void non_fatal_error(std::string errorString, std::string title = std::string());

///////////////////////////////////////////////////////////////////////////
/// Initialize a window, an openGL context, and initiate async debug output.
///////////////////////////////////////////////////////////////////////////
SDL_Window* init_window_SDL(std::string caption, int width = 1280, int height = 720);

///////////////////////////////////////////////////////////////////////////
/// Destroys that which have been initialized.
///////////////////////////////////////////////////////////////////////////
void shutDown(SDL_Window* window);

///////////////////////////////////////////////////////////////////////////
/// Creates a cube map using the files specified for each face.
///////////////////////////////////////////////////////////////////////////
GLuint loadCubeMap(const char* facePosX,
                   const char* faceNegX,
                   const char* facePosY,
                   const char* faceNegY,
                   const char* facePosZ,
                   const char* faceNegZ);

///////////////////////////////////////////////////////////////////////////
/// Helper function used to get log info (such as errors) about a shader object or shader program
///////////////////////////////////////////////////////////////////////////
std::string GetShaderInfoLog(GLuint obj);

///////////////////////////////////////////////////////////////////////////
/// Loads and compiles a fragment and vertex shader. Then creates a shader program
/// and attaches the shaders. Does NOT link the program, this is done with  linkShaderProgram()
/// The reason for this is that before linking we need to bind attribute locations, using
/// glBindAttribLocation and fragment data lications, using glBindFragDataLocation.
///////////////////////////////////////////////////////////////////////////
GLuint loadShaderProgram(const std::string& vertexShader,
                         const std::string& fragmentShader,
                         bool allow_errors = false);

///////////////////////////////////////////////////////////////////////////
/// Call to link a shader program prevoiusly loaded using loadShaderProgram.
///////////////////////////////////////////////////////////////////////////
bool linkShaderProgram(GLuint shaderProgram, bool allow_errors = false);

///////////////////////////////////////////////////////////////////////////
/// Creates a GL buffer, uploads the given data to it, and attaches it to the VAO.
/// returns the handle of the GL buffer.
///////////////////////////////////////////////////////////////////////////
GLuint createAddAttribBuffer(GLuint vertexArrayObject,
                             const void* data,
                             const size_t dataSize,
                             GLuint attributeIndex,
                             GLsizei attributeSize,
                             GLenum type,
                             GLenum bufferUsage = GL_STATIC_DRAW);

///////////////////////////////////////////////////////////////////////////
/// Creates a GL index buffer, uploads the given data to it, and attaches it ot the VAO.
/// returns the handle of the GL buffer.
///////////////////////////////////////////////////////////////////////////
GLuint createAddIndexBuffer(GLuint vertexArrayObject,
                            const void* data,
                            const size_t dataSize,
                            GLenum bufferUsage = GL_STATIC_DRAW);


///////////////////////////////////////////////////////////////////////////
/// Helper to set uniform variables in shaders, labeled SLOW because they find the location from string each time.
/// In OpenGL (and similarly in other APIs) it is much more efficient (in terms of CPU time) to keep the uniform
/// location, and use that. Or even better, use uniform buffers!
/// However, in the simple tutorial samples, performance is not an issue.
/// Overloaded to set many types.
///////////////////////////////////////////////////////////////////////////
void setUniformSlow(GLuint shaderProgram, const char* name, const glm::mat4& matrix);
void setUniformSlow(GLuint shaderProgram, const char* name, const float value);
void setUniformSlow(GLuint shaderProgram, const char* name, const GLint value);
void setUniformSlow(GLuint shaderProgram, const char* name, const GLuint value);
void setUniformSlow(GLuint shaderProgram, const char* name, const bool value);
void setUniformSlow(GLuint shaderProgram, const char* name, const glm::vec3& value);
void setUniformSlow(GLuint shaderProgram, const char* name, const uint32_t nof_values, const glm::vec3* values);

///////////////////////////////////////////////////////////////////////////
/// Draws a single quad (two triangles) that cover the entire screen
///////////////////////////////////////////////////////////////////////////
void drawFullScreenQuad();

///////////////////////////////////////////////////////////////////////////
/// Draws an arrow going from start to the point
///////////////////////////////////////////////////////////////////////////
void debugDrawArrow(const glm::mat4& viewMat, const glm::mat4& projMat, glm::vec3 start, glm::vec3 point);

///////////////////////////////////////////////////////////////////////////
/// Draws a sphere
///////////////////////////////////////////////////////////////////////////
void debugDrawSphere();

///////////////////////////////////////////////////////////////////////////
/// Draws a disc
///////////////////////////////////////////////////////////////////////////
void debugDrawDisc();


///////////////////////////////////////////////////////////////////////////
/// Takes the image in the default framebuffer and stores it in a file
///////////////////////////////////////////////////////////////////////////
void saveScreenshot();

///////////////////////////////////////////////////////////////////////////
/// Generates random, uniformly distributed floating point
/// numbers in the interval [from, to].
///////////////////////////////////////////////////////////////////////////
float uniform_randf(const float from, const float to);

///////////////////////////////////////////////////////////////////////////
/// Generates random, uniformly distributed floating point
/// numbers in the interval [0, 1].
///////////////////////////////////////////////////////////////////////////
float randf();

///////////////////////////////////////////////////////////////////////////
/// Generates uniform points on a disc
///////////////////////////////////////////////////////////////////////////
glm::vec2 concentricSampleDisk();

///////////////////////////////////////////////////////////////////////////
/// Generates points with a cosine distribution on the hemisphere
///////////////////////////////////////////////////////////////////////////
glm::vec3 cosineSampleHemisphere();

///////////////////////////////////////////////////////////////////////////
/// Generate a vector that is perpendicular to another
///////////////////////////////////////////////////////////////////////////
glm::vec3 perpendicular(const glm::vec3& v);

///////////////////////////////////////////////////////////////////////////
/// Creates a TBN matrix for the tangent space orthonormal to N
///////////////////////////////////////////////////////////////////////////
glm::mat3 tangentSpace(glm::vec3 n);

///////////////////////////////////////////////////////////////////////////
/// Used to obtain the number of elements of a C-style array
///////////////////////////////////////////////////////////////////////////
template<typename _T, size_t _Sz>
inline size_t array_length(const _T (&arr)[_Sz])
{
	return _Sz;
}


namespace file
{
	std::string normalise(const std::string& file_name);
	std::string parent_path(const std::string& file_name);
	std::string file_stem(const std::string& file_name);
	std::string file_extension(const std::string& file_name);
	std::string change_extension(const std::string& file_name, const std::string& ext);
} // namespace file
} // namespace labhelper
