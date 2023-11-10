
#include <GL/glew.h>

// STB_IMAGE for loading images of many filetypes
#include <stb_image.h>

#include <cstdlib>

#include <labhelper.h>

#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
using namespace glm;

///////////////////////////////////////////////////////////////////////////////
// Various globals
///////////////////////////////////////////////////////////////////////////////

// The window we'll be rendering to
SDL_Window* g_window = nullptr;

int mag = 1;
int mini = 5;
float anisotropy = 16.0f;
float camera_pan = 0.f;
bool showUI = false;


///////////////////////////////////////////////////////////////////////////////
// Shader programs
///////////////////////////////////////////////////////////////////////////////

// The shaderProgram holds the vertexShader and fragmentShader
GLuint shaderProgram;


///////////////////////////////////////////////////////////////////////////////
// Scene objects
///////////////////////////////////////////////////////////////////////////////

GLuint vaoRoad, textureRoad;
GLuint vaoExplosion, textureExplosion;


///////////////////////////////////////////////////////////////////////////////
/// This function is called once at the start of the program and never again
///////////////////////////////////////////////////////////////////////////////
void initRoadVAO();
void initExplosionVAO();
void initialize()
{
	ENSURE_INITIALIZE_ONLY_ONCE();

	// Init the first VAO - The road.
	initRoadVAO();
	initExplosionVAO();

	// The loadShaderProgram and linkShaderProgam functions are defined in glutil.cpp and
	// do exactly what we did in lab1 but are hidden for convenience
	shaderProgram = labhelper::loadShaderProgram("../lab2-textures/simple.vert",
	                                             "../lab2-textures/simple.frag");
}


void initRoadVAO()
{
	// Create a handle for the vertex array object
	glGenVertexArrays(1, &vaoRoad);
	// Set it as current, i.e., related calls will affect this object
	glBindVertexArray(vaoRoad);

	GLuint positionBuffer;
	const float positions[] = {
		// X      Y       Z
		-10.0f, 0.0f, -10.0f,  // v0
		-10.0f, 0.0f, -330.0f, // v1
		10.0f,  0.0f, -330.0f, // v2
		10.0f,  0.0f, -10.0f   // v3
	};
	// Create a handle for the vertex position buffer
	glGenBuffers(1, &positionBuffer);
	// Set the newly created buffer as the current one
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	// Send the vetex position data to the current buffer
	glBufferData(GL_ARRAY_BUFFER, labhelper::array_length(positions) * sizeof(float), positions,
	             GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, false /*normalized*/, 0 /*stride*/, 0 /*offset*/);
	// Enable the attribute
	glEnableVertexAttribArray(0);

	float texcoords[] = {
		0.0f, 0.0f,  // (u,v) for v0
		0.0f, 15.0f, // (u,v) for v1
		1.0f, 15.0f, // (u,v) for v2
		1.0f, 0.0f   // (u,v) for v3
	};
	GLuint texCoordsBuffer;
	glGenBuffers(1, &texCoordsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texCoordsBuffer);
	glBufferData(GL_ARRAY_BUFFER, labhelper::array_length(texcoords) * sizeof(float), texcoords,
	             GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(1);

	const int indices[] = {
		0, 3, 1, // Triangle 1
		1, 3, 2  // Triangle 2
	};
	GLuint indexBuffer;
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, labhelper::array_length(indices) * sizeof(float), indices,
	             GL_STATIC_DRAW);

	// Load the texture.
	int w, h, comp;
	unsigned char* image = stbi_load("../scenes/textures/asphalt.jpg", &w, &h, &comp, STBI_rgb_alpha);

	// Allocate an OpenGL texture.
	glGenTextures(1, &textureRoad);
	glBindTexture(GL_TEXTURE_2D, textureRoad);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	free(image);

	// Configure the texture.
	// Texture wrapping behaviour.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glGenerateMipmap(GL_TEXTURE_2D);
}

void initExplosionVAO()
{
	glGenVertexArrays(1, &vaoExplosion);
	glBindVertexArray(vaoExplosion);

	// Verticies.
	const float vertexPos[] = {
		// X   Y       Z
		1.0f,  0.0f,  -20.0f, // lower left
		11.0f, 0.0f,  -20.0f, // lower right
		1.0f,  10.0f, -20.0f, // upper left
		11.0f, 10.0f, -20.0f  // upper right
	};
	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, labhelper::array_length(vertexPos) * sizeof(float), vertexPos,
	             GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(0);
	CHECK_GL_ERROR();

	// Texture.
	float texcoords[] = {
		0.0f, 0.0f, // lower left
		0.0f, 1.0f, // lower right
		1.0f, 0.0f, // upper left
		1.0f, 1.0f  // upper right
	};
	GLuint texCoordsBuffer;
	glGenBuffers(1, &texCoordsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texCoordsBuffer);
	glBufferData(GL_ARRAY_BUFFER, labhelper::array_length(texcoords) * sizeof(float), texcoords,
	             GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(1);
	CHECK_GL_ERROR();

	// Indices.
	const int indices[] = {
		0, 1, 2, // Triangle 1
		1, 3, 2  // Triangle 2
	};
	GLuint indexBuffer;
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, labhelper::array_length(indices) * sizeof(float), indices,
	             GL_STATIC_DRAW);
	CHECK_GL_ERROR();

	// Load texture.
	int w, h, comp;
	unsigned char* image = stbi_load("../scenes/textures/explosion.png", &w, &h, &comp, STBI_rgb_alpha);
	glGenTextures(1, &textureExplosion);
	glBindTexture(GL_TEXTURE_2D, textureExplosion);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	free(image);
	glGenerateMipmap(GL_TEXTURE_2D);
	CHECK_GL_ERROR();
}


///////////////////////////////////////////////////////////////////////////////
/// This function will be called once per frame, so the code to set up
/// the scene for rendering should go here
///////////////////////////////////////////////////////////////////////////////
void display(void)
{
	// The viewport determines how many pixels we are rasterizing to
	int w, h;
	SDL_GL_GetDrawableSize(g_window, &w, &h);
	// Set viewport
	glViewport(0, 0, w, h);

	// Set clear color
	glClearColor(0.2f, 0.2f, 0.8f, 1.0f);
	// Clears the color buffer and the z-buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// We disable backface culling for this tutorial, otherwise care must be taken with the winding order
	// of the vertices. It is however a lot faster to enable culling when drawing large scenes.
	glEnable(GL_CULL_FACE);
	// Disable depth testing
	glDisable(GL_DEPTH_TEST);
	// Set the shader program to use for this draw call
	glUseProgram(shaderProgram);

	// Set up a projection matrix
	float fovy = radians(45.0f);
	float aspectRatio = float(w) / float(h);
	float nearPlane = 0.01f;
	float farPlane = 400.0f;
	mat4 projectionMatrix = perspective(fovy, aspectRatio, nearPlane, farPlane);
	// Send it to the vertex shader
	int loc = glGetUniformLocation(shaderProgram, "projectionMatrix");
	glUniformMatrix4fv(loc, 1, false, &projectionMatrix[0].x);

	loc = glGetUniformLocation(shaderProgram, "cameraPosition");
	glUniform3f(loc, camera_pan, 10, 0);

	// Bind the road texture so we may change its parameters. Interstingly, the
	// tutorial says these always need to be set, but the docs seem to point to
	// well defined initial values.
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureRoad);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mini);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);

	// Draw the road.
	glBindVertexArray(vaoRoad);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	// Configure blending.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Draw the explosion.
	glBindVertexArray(vaoExplosion);
	glBindTexture(GL_TEXTURE_2D, textureExplosion);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glUseProgram(0); // "unsets" the current shader program. Not really necessary.
}


///////////////////////////////////////////////////////////////////////////////
/// This function is to hold the general GUI logic
///////////////////////////////////////////////////////////////////////////////
void gui()
{
	// ----------------- Set variables --------------------------
	ImGui::PushID("mag");
	ImGui::Text("Magnification");
	ImGui::RadioButton("GL_NEAREST", &mag, GL_NEAREST);
	ImGui::RadioButton("GL_LINEAR", &mag, GL_LINEAR);
	ImGui::PopID();

	ImGui::PushID("mini");
	ImGui::Text("Minification");
	ImGui::RadioButton("GL_NEAREST", &mini, GL_NEAREST);
	ImGui::RadioButton("GL_LINEAR", &mini, GL_LINEAR);
	ImGui::RadioButton("GL_NEAREST_MIPMAP_NEAREST", &mini, GL_NEAREST_MIPMAP_NEAREST);
	ImGui::RadioButton("GL_NEAREST_MIPMAP_LINEAR", &mini, GL_NEAREST_MIPMAP_LINEAR);
	ImGui::RadioButton("GL_LINEAR_MIPMAP_NEAREST", &mini, GL_LINEAR_MIPMAP_NEAREST);
	ImGui::RadioButton("GL_LINEAR_MIPMAP_LINEAR", &mini, GL_LINEAR_MIPMAP_LINEAR);
	ImGui::PopID();

	ImGui::SliderFloat("Anisotropic filtering", &anisotropy, 1.0, 16.0, "Number of samples: %.0f");
	ImGui::Dummy({ 0, 20 });
	ImGui::SliderFloat("Camera Panning", &camera_pan, -1.0, 1.0);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
	            ImGui::GetIO().Framerate);
	// ----------------------------------------------------------
}

int main(int argc, char* argv[])
{
	g_window = labhelper::init_window_SDL("OpenGL Lab 2");

	initialize();

	// render-loop
	bool stopRendering = false;
	while(!stopRendering)
	{
		// Inform imgui of new frame
		ImGui_ImplSdlGL3_NewFrame(g_window);

		// check events (keyboard among other)
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			// Allow ImGui to capture events.
			ImGui_ImplSdlGL3_ProcessEvent(&event);

			if(event.type == SDL_QUIT || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE))
			{
				stopRendering = true;
			}
			else if(event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_g)
			{
				showUI = !showUI;
			}
			else if(event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_PRINTSCREEN)
			{
				labhelper::saveScreenshot();
			}
		}

		// render to window
		display();

		// Render overlay GUI.
		if(showUI)
		{
			gui();
		}

		// Render the GUI.
		ImGui::Render();

		// Swap front and back buffer. This frame will now been displayed.
		SDL_GL_SwapWindow(g_window);
	}

	// Shut down everything. This includes the window and all other subsystems.
	labhelper::shutDown(g_window);
	return 0;
}