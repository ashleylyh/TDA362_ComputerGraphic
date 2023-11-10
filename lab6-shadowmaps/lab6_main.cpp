

#include <GL/glew.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <map>

#include <labhelper.h>
#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
using namespace glm;

#include <Model.h>
#include "hdr.h"
#include "fbo.h"

using std::min;
using std::max;

///////////////////////////////////////////////////////////////////////////////
// Various globals
///////////////////////////////////////////////////////////////////////////////
SDL_Window* g_window = nullptr;
static float currentTime = 0.0f;
static float deltaTime = 0.0f;
bool showUI = true;

// Mouse input
ivec2 g_prevMouseCoords = { -1, -1 };
bool g_isMouseDragging = false;
bool g_isMouseRightDragging = false;

///////////////////////////////////////////////////////////////////////////////
// Shader programs
///////////////////////////////////////////////////////////////////////////////
GLuint shaderProgram; // Shader for rendering the final image
GLuint depthProgram;  // Shader used to draw the shadow map
GLuint simpleShaderProgram;
GLuint backgroundProgram;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
float environment_multiplier = 0.9f;
GLuint environmentMap, irradianceMap, reflectionMap;
const std::string envmap_base_name = "001";

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
vec3 lightPosition;
float lightAzimuth = 0.f;
float lightZenith = 45.f;
float lightDistance = 55.f;
bool animateLight = true;
vec3 point_light_color = vec3(1.f, 1.f, 1.f);
bool useSpotLight = false;
float innerSpotlightAngle = 17.5f;
float outerSpotlightAngle = 22.5f;
float point_light_intensity_multiplier = 10000.0f;


///////////////////////////////////////////////////////////////////////////////
// Shadow map
///////////////////////////////////////////////////////////////////////////////
enum ClampMode
{
	Edge = 1,
	Border = 2
};

FboInfo shadowMapFB;
int shadowMapResolution = 1024;
int shadowMapClampMode = ClampMode::Edge;
bool shadowMapClampBorderShadowed = false;
bool usePolygonOffset = false;
bool useSoftFalloff = false;
bool useHardwarePCF = false;
float polygonOffset_factor = .25f;
float polygonOffset_units = 1.0f;


///////////////////////////////////////////////////////////////////////////////
// Camera parameters.
///////////////////////////////////////////////////////////////////////////////
float cameraSpeed = 10.f;

vec3 worldUp(0.0f, 1.0f, 0.0f);

///////////////////////////////////////////////////////////////////////////////
// Models
///////////////////////////////////////////////////////////////////////////////
labhelper::Model* landingpadModel = nullptr;

struct camera_t
{
	vec3 position;
	vec3 direction;
};

struct scene_t
{
	struct scene_object_t
	{
		labhelper::Model* model;
		mat4 modelMat;
	};
	std::vector<scene_object_t> models;

	camera_t camera;
};

std::map<std::string, scene_t> scenes;
std::string currentScene;
camera_t camera;

void changeScene(std::string sceneName)
{
	currentScene = sceneName;
	camera = scenes[currentScene].camera;
}

void cleanupScenes()
{
	for(auto& it : scenes)
	{
		for(auto& m : it.second.models)
		{
			labhelper::freeModel(m.model);
		}
	}
}

void loadScenes()
{
	scenes["Ship"] = { {
		                   // Models
		                   { labhelper::loadModelFromOBJ("../scenes/space-ship.obj"),
		                     translate(8.0f * worldUp) },
		               },
		               {
		                   // Camera
		                   vec3(-70, 50, 70),
		                   normalize(-vec3(-70, 50, 70)),
		               } };
	scenes["Peter Panning"] = { {
		                            // Models
		                            { labhelper::loadModelFromOBJ("../scenes/peter-panning-plane.obj"),
		                              mat4(1) },
		                        },
		                        {
		                            // Camera
		                            vec3(-13, 10, 17),
		                            normalize(-vec3(-10, 7, 10)),
		                        } };
}


///////////////////////////////////////////////////////////////////////////////
/// This function is called once at the start of the program and never again
///////////////////////////////////////////////////////////////////////////////
void initialize()
{
	ENSURE_INITIALIZE_ONLY_ONCE();

	///////////////////////////////////////////////////////////////////////
	// Sanity Check
	static int _initialized = 0;
	if(_initialized++)
	{
		labhelper::fatal_error("You must not call the initialize() function more than once!",
		                       "Already initialized!");
	}

	///////////////////////////////////////////////////////////////////////
	//		Load Shaders
	///////////////////////////////////////////////////////////////////////
	backgroundProgram = labhelper::loadShaderProgram("../lab6-shadowmaps/background.vert",
	                                                 "../lab6-shadowmaps/background.frag");
	shaderProgram = labhelper::loadShaderProgram("../lab6-shadowmaps/shading.vert",
	                                             "../lab6-shadowmaps/shading.frag");
	depthProgram = labhelper::loadShaderProgram("../lab6-shadowmaps/depth.vert",
	                                            "../lab6-shadowmaps/depth.frag");
	simpleShaderProgram = labhelper::loadShaderProgram("../lab6-shadowmaps/simple.vert",
	                                                   "../lab6-shadowmaps/simple.frag");

	///////////////////////////////////////////////////////////////////////
	// Load models and set up model matrices
	///////////////////////////////////////////////////////////////////////

	loadScenes();

	// You can find the valid values for this in `loadScenes`: "Ship", "Material Test" and "Cube"
	changeScene("Ship");
	//changeScene("Material Test");
	//changeScene("Cube");

	landingpadModel = labhelper::loadModelFromOBJ("../scenes/landingpad.obj");

	///////////////////////////////////////////////////////////////////////
	// Load environment map
	///////////////////////////////////////////////////////////////////////
	const int roughnesses = 8;
	std::vector<std::string> filenames;
	for(int i = 0; i < roughnesses; i++)
		filenames.push_back("../scenes/envmaps/" + envmap_base_name + "_dl_" + std::to_string(i) + ".hdr");

	reflectionMap = labhelper::loadHdrMipmapTexture(filenames);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	environmentMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + ".hdr");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	irradianceMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + "_irradiance.hdr");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);


	///////////////////////////////////////////////////////////////////////
	// Setup Framebuffer for shadow map rendering
	///////////////////////////////////////////////////////////////////////
	shadowMapFB.resize(shadowMapResolution, shadowMapResolution);


	glEnable(GL_DEPTH_TEST); // enable Z-buffering
	glEnable(GL_CULL_FACE);  // enables backface culling
}

void debugDrawLight(const glm::mat4& viewMatrix,
                    const glm::mat4& projectionMatrix,
                    const glm::vec3& worldSpaceLightPos)
{
	mat4 modelMatrix = glm::translate(worldSpaceLightPos);
	glUseProgram(simpleShaderProgram);
	labhelper::setUniformSlow(simpleShaderProgram, "modelViewProjectionMatrix",
	                          projectionMatrix * viewMatrix * modelMatrix);
	labhelper::setUniformSlow(simpleShaderProgram, "material_color", vec3(1, 1, 1));
	labhelper::debugDrawSphere();
}


void drawBackground(const mat4& viewMatrix, const mat4& projectionMatrix)
{
	glUseProgram(backgroundProgram);
	labhelper::setUniformSlow(backgroundProgram, "environment_multiplier", environment_multiplier);
	labhelper::setUniformSlow(backgroundProgram, "inv_PV", inverse(projectionMatrix * viewMatrix));
	labhelper::setUniformSlow(backgroundProgram, "camera_pos", camera.position);
	labhelper::drawFullScreenQuad();
}


///////////////////////////////////////////////////////////////////////////////
/// This function is used to draw the main objects on the scene
///////////////////////////////////////////////////////////////////////////////
void drawScene(GLuint currentShaderProgram,
               const mat4& viewMatrix,
               const mat4& projectionMatrix,
               const mat4& lightViewMatrix,
               const mat4& lightProjectionMatrix)
{
	glUseProgram(currentShaderProgram);
	// Light source
	vec4 viewSpaceLightPosition = viewMatrix * vec4(lightPosition, 1.0f);
	labhelper::setUniformSlow(currentShaderProgram, "point_light_color", point_light_color);
	labhelper::setUniformSlow(currentShaderProgram, "point_light_intensity_multiplier",
	                          point_light_intensity_multiplier);
	labhelper::setUniformSlow(currentShaderProgram, "viewSpaceLightPosition", vec3(viewSpaceLightPosition));
	labhelper::setUniformSlow(currentShaderProgram, "viewSpaceLightDir",
	                          normalize(vec3(viewMatrix * vec4(-lightPosition, 0.0f))));
	labhelper::setUniformSlow(currentShaderProgram, "spotOuterAngle", std::cos(radians(outerSpotlightAngle)));



	// Environment
	labhelper::setUniformSlow(currentShaderProgram, "environment_multiplier", environment_multiplier);

	// camera
	labhelper::setUniformSlow(currentShaderProgram, "viewInverse", inverse(viewMatrix));

	// landing pad
	mat4 modelMatrix(1.0f);
	labhelper::setUniformSlow(currentShaderProgram, "modelViewProjectionMatrix",
	                          projectionMatrix * viewMatrix * modelMatrix);
	labhelper::setUniformSlow(currentShaderProgram, "modelViewMatrix", viewMatrix * modelMatrix);
	labhelper::setUniformSlow(currentShaderProgram, "normalMatrix",
	                          inverse(transpose(viewMatrix * modelMatrix)));

	labhelper::render(landingpadModel);

	// scene objects
	for(auto& m : scenes[currentScene].models)
	{
		labhelper::setUniformSlow(currentShaderProgram, "modelViewProjectionMatrix",
		                          projectionMatrix * viewMatrix * m.modelMat);
		labhelper::setUniformSlow(currentShaderProgram, "modelViewMatrix", viewMatrix * m.modelMat);
		labhelper::setUniformSlow(currentShaderProgram, "normalMatrix",
		                          inverse(transpose(viewMatrix * m.modelMat)));
		labhelper::render(m.model);
	}
}


///////////////////////////////////////////////////////////////////////////////
/// This function will be called once per frame, so the code to set up
/// the scene for rendering should go here
///////////////////////////////////////////////////////////////////////////////
void display(void)
{
	int w, h;
	SDL_GetWindowSize(g_window, &w, &h);

	///////////////////////////////////////////////////////////////////////////
	// setup matrices
	///////////////////////////////////////////////////////////////////////////
	mat4 projMatrix = perspective(radians(45.0f), float(w) / float(h), 5.0f, 500.0f);
	mat4 viewMatrix = lookAt(camera.position, camera.position + camera.direction, worldUp);

	mat3 lightRot = rotate(radians(lightAzimuth), vec3(0, 1, 0)) * rotate(radians(lightZenith), vec3(0, 0, 1));
	lightPosition = lightRot * vec3(lightDistance, 0, 0);
	mat4 lightViewMatrix = lookAt(lightPosition, vec3(0.0f), lightRot * worldUp);
	mat4 lightProjMatrix = perspective(radians(45.0f), 1.0f, 25.0f, 100.0f);

	///////////////////////////////////////////////////////////////////////////
	// Bind the environment map(s) to unused texture units
	///////////////////////////////////////////////////////////////////////////
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, environmentMap);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, irradianceMap);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, reflectionMap);
	glActiveTexture(GL_TEXTURE0);

	///////////////////////////////////////////////////////////////////////////
	// Set up shadow map parameters
	///////////////////////////////////////////////////////////////////////////
	// Task 1

	///////////////////////////////////////////////////////////////////////////
	// Draw Shadow Map
	///////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////
	// Draw from camera
	///////////////////////////////////////////////////////////////////////////
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);
	glClearColor(0.2f, 0.2f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawBackground(viewMatrix, projMatrix);
	drawScene(shaderProgram, viewMatrix, projMatrix, lightViewMatrix, lightProjMatrix);
	debugDrawLight(viewMatrix, projMatrix, vec3(lightPosition));


	CHECK_GL_ERROR();
}


///////////////////////////////////////////////////////////////////////////////
/// This function is used to update the scene according to user input
///////////////////////////////////////////////////////////////////////////////
bool handleEvents(void)
{
	// check events (keyboard among other)
	SDL_Event event;
	bool quitEvent = false;

	// Allow ImGui to capture events.
	ImGuiIO& io = ImGui::GetIO();

	while(SDL_PollEvent(&event))
	{
		ImGui_ImplSdlGL3_ProcessEvent(&event);

		if(event.type == SDL_QUIT || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE))
		{
			quitEvent = true;
		}
		else if(event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_g)
		{
			showUI = !showUI;
		}
		else if(event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_PRINTSCREEN)
		{
			labhelper::saveScreenshot();
		}
		else if(event.type == SDL_MOUSEBUTTONDOWN && (!showUI || !ImGui::GetIO().WantCaptureMouse)
		        && (event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT)
		        && !(g_isMouseDragging || g_isMouseRightDragging))
		{
			if(event.button.button == SDL_BUTTON_LEFT)
			{
				g_isMouseDragging = true;
			}
			else if(event.button.button == SDL_BUTTON_RIGHT)
			{
				g_isMouseRightDragging = true;
			}
			int x;
			int y;
			SDL_GetMouseState(&x, &y);
			g_prevMouseCoords.x = x;
			g_prevMouseCoords.y = y;
		}

		if(!(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{
			g_isMouseDragging = false;
		}
		if(!(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT)))
		{
			g_isMouseRightDragging = false;
		}

		if(event.type == SDL_MOUSEMOTION)
		{
			// More info at https://wiki.libsdl.org/SDL_MouseMotionEvent
			int delta_x = event.motion.x - g_prevMouseCoords.x;
			int delta_y = event.motion.y - g_prevMouseCoords.y;
			if(g_isMouseDragging)
			{
				float rotation_speed = 0.005f;
				mat4 yaw = rotate(rotation_speed * -delta_x, worldUp);
				mat4 pitch = rotate(rotation_speed * -delta_y, normalize(cross(camera.direction, worldUp)));
				camera.direction = vec3(pitch * yaw * vec4(camera.direction, 0.0f));
			}
			else if(g_isMouseRightDragging)
			{
				const float rotation_speed = 0.01f;
				lightAzimuth += delta_x * rotation_speed;
			}
			g_prevMouseCoords.x = event.motion.x;
			g_prevMouseCoords.y = event.motion.y;
		}
	}

	if(!io.WantCaptureKeyboard)
	{
		// check keyboard state (which keys are still pressed)
		const uint8_t* state = SDL_GetKeyboardState(nullptr);
		vec3 cameraRight = cross(camera.direction, worldUp);
		if(state[SDL_SCANCODE_W])
		{
			camera.position += deltaTime * cameraSpeed * camera.direction;
		}
		if(state[SDL_SCANCODE_S])
		{
			camera.position -= deltaTime * cameraSpeed * camera.direction;
		}
		if(state[SDL_SCANCODE_A])
		{
			camera.position -= deltaTime * cameraSpeed * cameraRight;
		}
		if(state[SDL_SCANCODE_D])
		{
			camera.position += deltaTime * cameraSpeed * cameraRight;
		}
		if(state[SDL_SCANCODE_Q])
		{
			camera.position -= deltaTime * cameraSpeed * worldUp;
		}
		if(state[SDL_SCANCODE_E])
		{
			camera.position += deltaTime * cameraSpeed * worldUp;
		}
	}

	const float light_rotation_speed = 90.f;
	if(animateLight)
	{
		lightAzimuth += deltaTime * light_rotation_speed;
		lightAzimuth = fmodf(lightAzimuth, 360.f);
	}

	return quitEvent;
}


///////////////////////////////////////////////////////////////////////////////
/// This function is to hold the general GUI logic
///////////////////////////////////////////////////////////////////////////////
void gui()
{
	if(ImGui::BeginMainMenuBar())
	{
		if(ImGui::BeginMenu("Scene"))
		{
			for(auto it : scenes)
			{
				if(ImGui::MenuItem(it.first.c_str(), nullptr, it.first == currentScene))
				{
					changeScene(it.first);
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	// ----------------- Set variables --------------------------
	ImGui::SliderInt("Shadow Map Resolution", &shadowMapResolution, 32, 2048);
	ImGui::Text("Polygon Offset");
	ImGui::Checkbox("Use polygon offset", &usePolygonOffset);
	ImGui::SliderFloat("Factor", &polygonOffset_factor, 0.0f, 10.0f);
	ImGui::SliderFloat("Units", &polygonOffset_units, 0.0f, 100000.0f);
	ImGui::Text("Clamp Mode");
	ImGui::RadioButton("Clamp to edge", &shadowMapClampMode, ClampMode::Edge);
	ImGui::RadioButton("Clamp to border", &shadowMapClampMode, ClampMode::Border);
	ImGui::Checkbox("Border as shadow", &shadowMapClampBorderShadowed);
	ImGui::Checkbox("Use spot light", &useSpotLight);
	ImGui::Checkbox("Use soft falloff", &useSoftFalloff);
	ImGui::SliderFloat("Inner Deg.", &innerSpotlightAngle, 0.0f, 90.0f);
	ImGui::SliderFloat("Outer Deg.", &outerSpotlightAngle, 0.0f, 90.0f);
	ImGui::Checkbox("Use hardware PCF", &useHardwarePCF);
	ImGui::Checkbox("Animate light", &animateLight);
	ImGui::SliderFloat("Light Azimuth", &lightAzimuth, 0.0f, 360.0f);
	ImGui::SliderFloat("Light Zenith", &lightZenith, 0.0f, 90.0f);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
	            ImGui::GetIO().Framerate);
	// ----------------------------------------------------------
}

int main(int argc, char* argv[])
{
	g_window = labhelper::init_window_SDL("OpenGL Lab 6");

	initialize();

	bool stopRendering = false;
	auto startTime = std::chrono::system_clock::now();

	while(!stopRendering)
	{
		//update currentTime
		std::chrono::duration<float> timeSinceStart = std::chrono::system_clock::now() - startTime;
		deltaTime = timeSinceStart.count() - currentTime;
		currentTime = timeSinceStart.count();

		// Inform imgui of new frame
		ImGui_ImplSdlGL3_NewFrame(g_window);

		// check events (keyboard among other)
		stopRendering = handleEvents();

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
	// Free Models
	cleanupScenes();
	labhelper::freeModel(landingpadModel);

	// Shut down everything. This includes the window and all other subsystems.
	labhelper::shutDown(g_window);
	return 0;
}
