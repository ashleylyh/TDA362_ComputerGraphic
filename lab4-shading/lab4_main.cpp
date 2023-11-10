
#include <GL/glew.h>
#include <stb_image.h>
#include <chrono>
#include <iostream>
#include <map>
#include <labhelper.h>
#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <Model.h>


using namespace glm;
using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Various globals
///////////////////////////////////////////////////////////////////////////////
SDL_Window* g_window = nullptr;
static float currentTime = 0.0f;
static float deltaTime = 0.0f;
static int windowWidth, windowHeight;
bool showUI = true;

// Mouse input
ivec2 g_prevMouseCoords = { -1, -1 };
bool g_isMouseDragging = false;
bool g_isMouseRightDragging = false;

///////////////////////////////////////////////////////////////////////////////
// Shader programs
///////////////////////////////////////////////////////////////////////////////
GLuint shaderProgram;
GLuint backgroundProgram;
GLuint simpleShaderProgram;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
GLuint fullScreenQuadVAO = 0;
float environment_multiplier = 1.0f;
GLuint environmentMap;
GLuint irradianceMap;
GLuint reflectionMap;
const std::string envmap_base_name = "001";

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
vec3 lightPosition;
float lightRotation = 0.f;
bool lightManualOnly = true;
float point_light_intensity_multiplier = 1000.0f;
vec3 point_light_color = vec3(1.f, 1.f, 1.f);


///////////////////////////////////////////////////////////////////////////////
// Models
///////////////////////////////////////////////////////////////////////////////
vec3 worldUp(0.0f, 1.0f, 0.0f);

struct camera_t
{
	vec3 position;
	vec3 direction;
};

struct scene_t
{
	std::vector<labhelper::Model*> models;

	camera_t camera;
};

std::map<std::string, scene_t> scenes;
std::string currentScene;
camera_t camera;

size_t selectedModelIdxInScene = 0;
int selectedMeshInModel = 0;
int selectedMaterialIdx = 0;

void changeScene(std::string sceneName)
{
	currentScene = sceneName;
	selectedModelIdxInScene = 0;
	selectedMeshInModel = 0;
	selectedMaterialIdx =
	    scenes[currentScene].models[selectedModelIdxInScene]->m_meshes[selectedMeshInModel].m_material_idx;
	camera = scenes[currentScene].camera;
}

void cleanupScenes()
{
	for(auto& it : scenes)
	{
		for(auto m : it.second.models)
		{
			labhelper::freeModel(m);
		}
	}
}

void loadScenes()
{
	scenes["Ship"] = { {
		                   // Models
		                   labhelper::loadModelFromOBJ("../scenes/space-ship.obj"),
		               },
		               {
		                   // Camera
		                   vec3(-30, 10, 30),
		                   normalize(-vec3(-30, 5, 30)),
		               } };
	scenes["Material Test"] = { {
		                            // Models
		                            labhelper::loadModelFromOBJ("../scenes/materialtest.obj"),
		                        },
		                        {
		                            // Camera
		                            vec3(0, 30, 30),
		                            normalize(-vec3(0, 30, 30)),
		                        } };
	scenes["Cube"] = { {
		                   // Models
		                   labhelper::loadModelFromOBJ("../scenes/cube.obj"),
		               },
		               {
		                   // Camera
		                   vec3(2, 2, 2),
		                   normalize(-vec3(2, 2, 2)),
		               } };
}

///////////////////////////////////////////////////////////////////////////////
/// The load shaders function is called once from initialize() and then
/// whenever you press the Reload Shaders button
///////////////////////////////////////////////////////////////////////////////
void loadShaders(bool is_reload)
{
	GLuint shader = labhelper::loadShaderProgram("../lab4-shading/shading.vert",
	                                             "../lab4-shading/shading.frag", is_reload);
	if(shader != 0)
		shaderProgram = shader;

	shader = labhelper::loadShaderProgram("../lab4-shading/background.vert",
	                                      "../lab4-shading/background.frag", is_reload);
	if(shader != 0)
		backgroundProgram = shader;

	shader = labhelper::loadShaderProgram("../lab4-shading/simple.vert", "../lab4-shading/simple.frag",
	                                      is_reload);
	if(shader != 0)
		simpleShaderProgram = shader;
}

///////////////////////////////////////////////////////////////////////////////
/// Create buffer to render a full screen quad
///////////////////////////////////////////////////////////////////////////////
void initFullScreenQuad()
{
	///////////////////////////////////////////////////////////////////////////
	// initialize the fullScreenQuadVAO for drawFullScreenQuad
	///////////////////////////////////////////////////////////////////////////
	if(fullScreenQuadVAO == 0)
	{
		// Task 4.1
		// ...
	}
}

///////////////////////////////////////////////////////////////////////////////
/// Draw a full screen quad to the screen
///////////////////////////////////////////////////////////////////////////////
void drawFullScreenQuad()
{
	///////////////////////////////////////////////////////////////////////////
	// draw a quad at full screen
	///////////////////////////////////////////////////////////////////////////
	// Task 4.2
	// ...
}


///////////////////////////////////////////////////////////////////////////////
/// This function is called once at the start of the program and never again
///////////////////////////////////////////////////////////////////////////////
void initialize()
{
	ENSURE_INITIALIZE_ONLY_ONCE();

	///////////////////////////////////////////////////////////////////////////
	// Load shaders first time. Do not allow errors.
	///////////////////////////////////////////////////////////////////////////
	loadShaders(false);

	///////////////////////////////////////////////////////////////////////////
	// Local helper struct for loading HDR images
	///////////////////////////////////////////////////////////////////////////
	struct HDRImage
	{
		int width, height, components;
		float* data = nullptr;
		// Constructor
		HDRImage(const string& filename)
		{
			stbi_set_flip_vertically_on_load(true);
			data = stbi_loadf(filename.c_str(), &width, &height, &components, 3);
			if(data == NULL)
			{
				std::cout << "Failed to load image: " << filename << ".\n";
				exit(1);
			}
		};
		// Destructor
		~HDRImage()
		{
			stbi_image_free(data);
		};
	};

	///////////////////////////////////////////////////////////////////////////
	// Load environment map
	// NOTE: You can safely ignore this until you start Task 4.
	///////////////////////////////////////////////////////////////////////////
	initFullScreenQuad();
	{ // Environment map
		HDRImage image("../scenes/envmaps/" + envmap_base_name + ".hdr");
		glGenTextures(1, &environmentMap);
		glBindTexture(GL_TEXTURE_2D, environmentMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, image.width, image.height, 0, GL_RGB, GL_FLOAT, image.data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	{ // Irradiance map
		HDRImage image("../scenes/envmaps/" + envmap_base_name + "_irradiance.hdr");
		glGenTextures(1, &irradianceMap);
		glBindTexture(GL_TEXTURE_2D, irradianceMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, image.width, image.height, 0, GL_RGB, GL_FLOAT, image.data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	{ // Reflection map
		glGenTextures(1, &reflectionMap);
		glBindTexture(GL_TEXTURE_2D, reflectionMap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		HDRImage image("../scenes/envmaps/" + envmap_base_name + "_dl_0.hdr");
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, image.width, image.height, 0, GL_RGB, GL_FLOAT, image.data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// We call this again because AMD drivers have some weird issue in the GenerateMipmap function that
		// breaks the first level of the image.
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, image.width, image.height, 0, GL_RGB, GL_FLOAT, image.data);

		const int roughnesses = 8;
		for(int i = 1; i < roughnesses; i++)
		{
			HDRImage image("../scenes/envmaps/" + envmap_base_name + "_dl_" + std::to_string(i) + ".hdr");
			glTexImage2D(GL_TEXTURE_2D, i, GL_RGB32F, image.width, image.height, 0, GL_RGB, GL_FLOAT,
			             image.data);
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Load .obj models
	///////////////////////////////////////////////////////////////////////////
	loadScenes();

	// You can find the valid values for this in `loadScenes`: "Ship", "Material Test" and "Cube"
	changeScene("Ship");
	//changeScene("Material Test");
	//changeScene("Cube");
}

void debugDrawLight(const glm::mat4& viewMatrix,
                    const glm::mat4& projectionMatrix,
                    const glm::vec3& worldSpaceLightPos)
{
	glUseProgram(simpleShaderProgram);
	mat4 modelMatrix = glm::translate(worldSpaceLightPos);
	labhelper::setUniformSlow(simpleShaderProgram, "modelViewProjectionMatrix",
	                          projectionMatrix * viewMatrix * modelMatrix);
	labhelper::setUniformSlow(simpleShaderProgram, "material_color", vec3(1, 1, 1));
	labhelper::debugDrawSphere();
}


///////////////////////////////////////////////////////////////////////////////
/// This function will be called once per frame, so the code to set up
/// the scene for rendering should go here
///////////////////////////////////////////////////////////////////////////////
void display(void)
{
	///////////////////////////////////////////////////////////////////////////
	// Set up OpenGL stuff
	///////////////////////////////////////////////////////////////////////////
	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(0.1f, 0.1f, 0.6f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	SDL_GetWindowSize(g_window, &windowWidth, &windowHeight);

	///////////////////////////////////////////////////////////////////////////
	// Bind the environment map(s) to unused texture units
	///////////////////////////////////////////////////////////////////////////
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, environmentMap);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, irradianceMap);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, reflectionMap);

	///////////////////////////////////////////////////////////////////////////
	// Set up the view and projection matrix for the camera
	///////////////////////////////////////////////////////////////////////////
	mat4 viewMatrix, projectionMatrix;
	{
		vec3 cameraRight = normalize(cross(camera.direction, worldUp));
		vec3 cameraUp = normalize(cross(cameraRight, camera.direction));
		mat3 cameraBaseVectorsWorldSpace(cameraRight, cameraUp, -camera.direction);
		mat4 cameraRotation = mat4(transpose(cameraBaseVectorsWorldSpace));
		viewMatrix = cameraRotation * translate(-camera.position);
		projectionMatrix = perspective(radians(45.0f), float(windowWidth) / float(windowHeight), 0.01f,
		                               300.0f);
	}

	///////////////////////////////////////////////////////////////////////////
	// Task 4.3 - Render a fullscreen quad, to generate the background from the
	//            environment map.
	///////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////
	// Render the .obj models
	///////////////////////////////////////////////////////////////////////////
	glUseProgram(shaderProgram);
	// Light source
	vec4 lightStartPosition = vec4(0.0f, 20.0f, 20.0f, 1.0f);
	float light_rotation_speed = 1.f;
	if(!lightManualOnly && !g_isMouseRightDragging)
	{
		lightRotation += deltaTime * light_rotation_speed;
	}
	lightPosition = vec3(rotate(lightRotation + float(M_PI / 4), worldUp) * lightStartPosition);
	vec4 viewSpaceLightPosition = viewMatrix * vec4(lightPosition, 1);
	labhelper::setUniformSlow(shaderProgram, "point_light_color", point_light_color);
	labhelper::setUniformSlow(shaderProgram, "point_light_intensity_multiplier",
	                          point_light_intensity_multiplier);
	labhelper::setUniformSlow(shaderProgram, "viewSpaceLightPosition", vec3(viewSpaceLightPosition));
	// Environment
	labhelper::setUniformSlow(shaderProgram, "environment_multiplier", environment_multiplier);

	// The model matrix is just identity
	mat4 modelMatrix = mat4(1);
	// Matrices
	labhelper::setUniformSlow(shaderProgram, "modelViewProjectionMatrix",
	                          projectionMatrix * viewMatrix * modelMatrix);
	labhelper::setUniformSlow(shaderProgram, "viewInverse", inverse(viewMatrix));
	labhelper::setUniformSlow(shaderProgram, "modelViewMatrix", viewMatrix * modelMatrix);
	labhelper::setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(viewMatrix * modelMatrix)));

	// Render the scene objects
	for(labhelper::Model* m : scenes[currentScene].models)
	{
		labhelper::render(m);
	}
	// Render the light source
	debugDrawLight(viewMatrix, projectionMatrix, vec3(lightPosition));

	glUseProgram(0);
}


///////////////////////////////////////////////////////////////////////////////
/// This function is used to update the scene according to user input
///////////////////////////////////////////////////////////////////////////////
bool handleEvents()
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
				lightRotation += delta_x * rotation_speed;
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
		const float speed = 10.f;
		if(state[SDL_SCANCODE_W])
		{
			camera.position += deltaTime * speed * camera.direction;
		}
		if(state[SDL_SCANCODE_S])
		{
			camera.position -= deltaTime * speed * camera.direction;
		}
		if(state[SDL_SCANCODE_A])
		{
			camera.position -= deltaTime * speed * cameraRight;
		}
		if(state[SDL_SCANCODE_D])
		{
			camera.position += deltaTime * speed * cameraRight;
		}
		if(state[SDL_SCANCODE_Q])
		{
			camera.position -= deltaTime * speed * worldUp;
		}
		if(state[SDL_SCANCODE_E])
		{
			camera.position += deltaTime * speed * worldUp;
		}
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

	///////////////////////////////////////////////////////////////////////////
	// Helpers for getting lists of materials and meshes into widgets
	///////////////////////////////////////////////////////////////////////////
	static auto mesh_getter = [](void* vec, int idx, const char** text)
	{
		auto& vector = *static_cast<std::vector<labhelper::Mesh>*>(vec);
		if(idx < 0 || idx >= static_cast<int>(vector.size()))
		{
			return false;
		}
		*text = vector[idx].m_name.c_str();
		return true;
	};

	///////////////////////////////////////////////////////////////////////////
	// List all meshes in the model and show properties for the selected
	///////////////////////////////////////////////////////////////////////////
	labhelper::Model* selected_model = scenes[currentScene].models[selectedModelIdxInScene];

	ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	{
		if(ImGui::CollapsingHeader("Meshes", "meshes_ch", true, true))
		{
			if(ImGui::ListBox("Meshes", &selectedMeshInModel, mesh_getter, (void*)&selected_model->m_meshes,
			                  int(selected_model->m_meshes.size()), 6))
			{
				selectedMaterialIdx = selected_model->m_meshes[selectedMeshInModel].m_material_idx;
			}

			labhelper::Mesh& mesh = selected_model->m_meshes[selectedMeshInModel];
			ImGui::LabelText("Mesh Name", "%s", mesh.m_name.c_str());
		}

		///////////////////////////////////////////////////////////////////////////
		// Show properties for the selected material
		///////////////////////////////////////////////////////////////////////////
		if(ImGui::CollapsingHeader("Material", "materials_ch", true, true))
		{
			labhelper::Material& material = selected_model->m_materials[selectedMaterialIdx];
			ImGui::LabelText("Material Name", "%s", material.m_name.c_str());
			ImGui::ColorEdit3("Color", &material.m_color.x);
			ImGui::SliderFloat("Metalness", &material.m_metalness, 0.0f, 1.0f);
			ImGui::SliderFloat("Fresnel", &material.m_fresnel, 0.0f, 1.0f);
			ImGui::SliderFloat("Shininess", &material.m_shininess, 0.0f, 5000.0f, "%.3f", 2);
			ImGui::ColorEdit3("Emission", &material.m_emission.x);
		}

		///////////////////////////////////////////////////////////////////////////
		// Light and environment map
		///////////////////////////////////////////////////////////////////////////
		if(ImGui::CollapsingHeader("Light sources", "lights_ch", true, true))
		{
			ImGui::SliderFloat("Environment multiplier", &environment_multiplier, 0.0f, 10.0f);
			ImGui::ColorEdit3("Point light color", &point_light_color.x);
			ImGui::SliderFloat("Point light intensity multiplier", &point_light_intensity_multiplier, 0.0f,
			                   10000.0f, "%.3f", 2.f);
			ImGui::Checkbox("Manual light only (right-click drag to move)", &lightManualOnly);
		}

#if ALLOW_SAVE_MATERIALS
		if(ImGui::Button("Save Materials"))
		{
			labhelper::saveModelMaterialsToMTL(selected_model,
			                                   labhelper::file::change_extension(selected_model->m_filename,
			                                                                     ".mtl"));
		}
#endif

		///////////////////////////////////////////////////////////////////////////
		// A button for reloading the shaders
		///////////////////////////////////////////////////////////////////////////
		if(ImGui::Button("Reload Shaders"))
		{
			loadShaders(true);
		}
	}
	ImGui::End(); // Control Panel
}

int main(int argc, char* argv[])
{
	g_window = labhelper::init_window_SDL("OpenGL Lab 4", 1280, 720);

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

		// Then render overlay GUI.
		if(showUI)
		{
			gui();
		}

		// Render the GUI.
		ImGui::Render();

		// Swap front and back buffer. This frame will now be displayed.
		SDL_GL_SwapWindow(g_window);
	}

	// Delete Models
	cleanupScenes();
	// Shut down everything. This includes the window and all other subsystems.
	labhelper::shutDown(g_window);
	return 0;
}
