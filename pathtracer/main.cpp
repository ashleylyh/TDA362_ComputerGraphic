
#include <GL/glew.h>
#include <stb_image.h>
#include <chrono>
#include <iostream>
#include <labhelper.h>
#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <Model.h>
#include <string>
#include "Pathtracer.h"
#include "embree.h"
#include "sampling.h"


using namespace glm;
using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Various globals
///////////////////////////////////////////////////////////////////////////////
SDL_Window* g_window = nullptr;
int windowWidth = 0, windowHeight = 0;

static float currentTime = 0.0f;
static float deltaTime = 0.0f;

bool showUI = true;

// Mouse input
ivec2 g_prevMouseCoords = { -1, -1 };
bool g_isMouseDragging = false;

bool showLightSources = false;

///////////////////////////////////////////////////////////////////////////////
// Shader programs
///////////////////////////////////////////////////////////////////////////////
GLuint shaderProgram;
GLuint simpleShaderProgram;

///////////////////////////////////////////////////////////////////////////////
// GL texture to put pathtracing result into
///////////////////////////////////////////////////////////////////////////////
uint32_t pathtracer_result_txt_id;

///////////////////////////////////////////////////////////////////////////////
// Scene
///////////////////////////////////////////////////////////////////////////////
vec3 worldUp(0.0f, 1.0f, 0.0f);

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

int selected_model_index = 0;
int selected_mesh_index = 0;
int selected_material_index = 0;


void loadScenes()
{
	scenes["Sphere"] = { {
		                     // Models
		                     { labhelper::loadModelFromOBJ("../scenes/sphere.obj"), mat4(1.f) },
		                 },
		                 {
		                     // Camera
		                     vec3(-15, 0, 15),
		                     normalize(-vec3(-15, 0, 15)),
		                 } };
	scenes["Ship"] = { {
		                   // Models
		                   { labhelper::loadModelFromOBJ("../scenes/space-ship.obj"),
		                     translate(vec3(0.f, 8.f, 0.f)) },
		                   { labhelper::loadModelFromOBJ("../scenes/landingpad.obj"), mat4(1.f) },
		               },
		               {
		                   // Camera
		                   vec3(-30, 15, 30),
		                   normalize(-vec3(-30, 8, 30)),
		               } };
	// Modify the landingpad screen's color
	scenes["Ship"].models[1].model->m_materials[8].m_color = glm::vec3(0.380392, 0.588235, 0.266667);

	scenes["Refractions"] = { {
		                          // Models
		                          { labhelper::loadModelFromOBJ("../scenes/refractions.obj"), mat4(1.f) },
		                      },
		                      {
		                          // Camera
		                          vec3(7.3, 3.2, 7.2),
		                          normalize(vec3(-0.43, -0.27, -0.85)),
		                      } };
}

void changeScene(std::string sceneName)
{
	currentScene = sceneName;
	camera = scenes[currentScene].camera;

	selected_model_index = 0;
	selected_mesh_index = 0;
	selected_material_index = scenes[currentScene].models[0].model->m_meshes[0].m_material_idx;


	pathtracer::reinitScene();

	// Add models to pathtracer scene
	for(auto& o : scenes[currentScene].models)
	{
		pathtracer::addModel(o.model, o.modelMat);
	}
	pathtracer::buildBVH();

	pathtracer::restart();
}

void cleanupScenes()
{
	for(auto& it : scenes)
	{
		for(auto m : it.second.models)
		{
			labhelper::freeModel(m.model);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Load shaders, environment maps, models and so on
///////////////////////////////////////////////////////////////////////////////
void initialize()
{
	///////////////////////////////////////////////////////////////////////////
	// Load shader program
	///////////////////////////////////////////////////////////////////////////
	shaderProgram = labhelper::loadShaderProgram("../pathtracer/copyTexture.vert",
	                                             "../pathtracer/copyTexture.frag");
	simpleShaderProgram = labhelper::loadShaderProgram("../pathtracer/simple.vert",
	                                                   "../pathtracer/simple.frag");

	///////////////////////////////////////////////////////////////////////////
	// Generate result texture
	///////////////////////////////////////////////////////////////////////////
	glGenTextures(1, &pathtracer_result_txt_id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pathtracer_result_txt_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	///////////////////////////////////////////////////////////////////////////
	// Initial path-tracer settings
	///////////////////////////////////////////////////////////////////////////
	pathtracer::settings.max_bounces = 8;
	pathtracer::settings.max_paths_per_pixel = 0; // 0 = Infinite
#ifdef _DEBUG
	pathtracer::settings.subsampling = 16;
#else
	pathtracer::settings.subsampling = 4;
#endif

	///////////////////////////////////////////////////////////////////////////
	// Set up light sources
	///////////////////////////////////////////////////////////////////////////
	pathtracer::point_light.intensity_multiplier = 2500.0f;
	pathtracer::point_light.color = vec3(1.f, 1.f, 1.f);
	pathtracer::point_light.position = vec3(10.0f, 25.0f, 20.0f);

	// float intensity_multiplier;
	// vec3 color;
	// vec3 position;
	// vec3 direction;
	// float radius;
	/*
	pathtracer::disc_lights.push_back( pathtracer::DiscLight{
									   1000,
									   {1, 0.8, 0},
									   {-8, 10, 8},
									   glm::normalize(glm::vec3(10, -2, 10)),
									   8.0 } );
	pathtracer::disc_lights.push_back( pathtracer::DiscLight{
									   1000,
									   {0.1, 0.3, 1},
									   {-10, 20, -5},
									   glm::normalize(-glm::vec3(-10, 20, -5)),
									   10.0 } );
	*/

	///////////////////////////////////////////////////////////////////////////
	// Load environment map
	///////////////////////////////////////////////////////////////////////////
	pathtracer::environment.map.load("../scenes/envmaps/001.hdr");
	pathtracer::environment.multiplier = 1.0f;

	///////////////////////////////////////////////////////////////////////////
	// Load .obj models to scene
	///////////////////////////////////////////////////////////////////////////
	loadScenes();
	changeScene("Ship");
	//changeScene("Sphere");
	//changeScene("Refractions");


	///////////////////////////////////////////////////////////////////////////
	// This is INCORRECT! But an easy way to get us a brighter image that
	// just looks a little better...
	///////////////////////////////////////////////////////////////////////////
	//glEnable(GL_FRAMEBUFFER_SRGB);
}

void display(void)
{
	{ ///////////////////////////////////////////////////////////////////////
		// If first frame, or window resized, or subsampling changes,
		// inform the pathtracer
		///////////////////////////////////////////////////////////////////////
		int w, h;
		SDL_GetWindowSize(g_window, &w, &h);
		static int old_subsampling;
		if(windowWidth != w || windowHeight != h || old_subsampling != pathtracer::settings.subsampling)
		{
			pathtracer::resize(w, h);
			windowWidth = w;
			windowWidth = h;
			old_subsampling = pathtracer::settings.subsampling;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Trace one path per pixel
	///////////////////////////////////////////////////////////////////////////
	mat4 viewMatrix = lookAt(camera.position, camera.position + camera.direction, worldUp);
	mat4 projMatrix = perspective(radians(45.0f),
	                              float(pathtracer::rendered_image.width)
	                                  / float(pathtracer::rendered_image.height),
	                              0.1f, 100.0f);
	pathtracer::tracePaths(viewMatrix, projMatrix);

	///////////////////////////////////////////////////////////////////////////
	// Copy pathtraced image to texture for display
	///////////////////////////////////////////////////////////////////////////
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pathtracer_result_txt_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, pathtracer::rendered_image.width,
	             pathtracer::rendered_image.height, 0, GL_RGB, GL_FLOAT, pathtracer::rendered_image.getPtr());

	///////////////////////////////////////////////////////////////////////////
	// Render a fullscreen quad, textured with our pathtraced image.
	///////////////////////////////////////////////////////////////////////////
	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(0.1f, 0.1f, 0.6f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	SDL_GetWindowSize(g_window, &windowWidth, &windowHeight);
	glUseProgram(shaderProgram);
	labhelper::drawFullScreenQuad();

	if(showLightSources)
	{
		glUseProgram(simpleShaderProgram);

		mat4 modelMatrix = glm::translate(pathtracer::point_light.position);
		glUseProgram(simpleShaderProgram);
		labhelper::setUniformSlow(simpleShaderProgram, "modelViewProjectionMatrix",
		                          projMatrix * viewMatrix * modelMatrix);
		labhelper::setUniformSlow(simpleShaderProgram, "material_color", pathtracer::point_light.color);

		labhelper::debugDrawSphere();

		for(int i = 0; i < pathtracer::disc_lights.size(); ++i)
		{
			mat3 tbn = labhelper::tangentSpace(pathtracer::disc_lights[i].direction);
			tbn = mat3(tbn[0], tbn[2], tbn[1]);
			mat4 modelMatrix = glm::translate(pathtracer::disc_lights[i].position) * mat4(tbn)
			                   * glm::scale(vec3(pathtracer::disc_lights[i].radius));
			glUseProgram(simpleShaderProgram);
			labhelper::setUniformSlow(simpleShaderProgram, "modelViewProjectionMatrix",
			                          projMatrix * viewMatrix * modelMatrix);
			labhelper::setUniformSlow(simpleShaderProgram, "material_color", pathtracer::disc_lights[i].color);

			labhelper::debugDrawDisc();

			labhelper::debugDrawArrow(viewMatrix, projMatrix, pathtracer::disc_lights[i].position,
			                          pathtracer::disc_lights[i].position
			                              + 2.f * pathtracer::disc_lights[i].direction);
		}
	}
}

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
		else if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT
		        && !io.WantCaptureMouse)
		{
			g_isMouseDragging = true;
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

		if(event.type == SDL_MOUSEMOTION && g_isMouseDragging)
		{
			// More info at https://wiki.libsdl.org/SDL_MouseMotionEvent
			int delta_x = event.motion.x - g_prevMouseCoords.x;
			int delta_y = event.motion.y - g_prevMouseCoords.y;
			float rotationSpeed = 0.1f;
			mat4 yaw = rotate(rotationSpeed * deltaTime * -delta_x, worldUp);
			mat4 pitch = rotate(rotationSpeed * deltaTime * -delta_y,
			                    normalize(cross(camera.direction, worldUp)));
			camera.direction = vec3(pitch * yaw * vec4(camera.direction, 0.0f));
			g_prevMouseCoords.x = event.motion.x;
			g_prevMouseCoords.y = event.motion.y;
			pathtracer::restart();
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
			pathtracer::restart();
		}
		if(state[SDL_SCANCODE_S])
		{
			camera.position -= deltaTime * speed * camera.direction;
			pathtracer::restart();
		}
		if(state[SDL_SCANCODE_A])
		{
			camera.position -= deltaTime * speed * cameraRight;
			pathtracer::restart();
		}
		if(state[SDL_SCANCODE_D])
		{
			camera.position += deltaTime * speed * cameraRight;
			pathtracer::restart();
		}
		if(state[SDL_SCANCODE_Q])
		{
			camera.position -= deltaTime * speed * worldUp;
			pathtracer::restart();
		}
		if(state[SDL_SCANCODE_E])
		{
			camera.position += deltaTime * speed * worldUp;
			pathtracer::restart();
		}
	}

	return quitEvent;
}

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
	static auto model_getter = [](void* scene, int idx, const char** text)
	{
		auto& s = *(static_cast<scene_t*>(scene));
		if(idx < 0 || idx >= static_cast<int>(s.models.size()))
		{
			return false;
		}
		*text = s.models[idx].model->m_name.c_str();
		return true;
	};


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

	static auto material_getter = [](void* vec, int idx, const char** text)
	{
		auto& vector = *static_cast<std::vector<labhelper::Material>*>(vec);
		if(idx < 0 || idx >= static_cast<int>(vector.size()))
		{
			return false;
		}
		*text = vector[idx].m_name.c_str();
		return true;
	};

	ImGui::SetNextWindowSizeConstraints({ 0, 0 }, { -1, float(windowHeight) - 20 });
	ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	///////////////////////////////////////////////////////////////////////////
	// Pathtracer settings
	///////////////////////////////////////////////////////////////////////////
	if(ImGui::CollapsingHeader("Pathtracer", "pathtracer_ch", true, true))
	{
		ImGui::SliderInt("Subsampling", &pathtracer::settings.subsampling, 1, 16);
		ImGui::SliderInt("Max Bounces", &pathtracer::settings.max_bounces, 0, 16);
		ImGui::SliderInt("Max Paths Per Pixel", &pathtracer::settings.max_paths_per_pixel, 0, 1024);
		if(ImGui::Button("Restart Pathtracing"))
		{
			pathtracer::restart();
		}
		ImGui::Text("Num. samples: %d", pathtracer::getSampleCount());
	}

	///////////////////////////////////////////////////////////////////////////
	// Choose a model to modify
	///////////////////////////////////////////////////////////////////////////
	labhelper::Model* selected_model = scenes[currentScene].models[selected_model_index].model;
	scene_t* selected_scene = &scenes[currentScene];

	if(ImGui::CollapsingHeader("Models", "meshes_ch", true, true))
	{
		if(ImGui::Combo("Model", &selected_model_index, model_getter, (void*)selected_scene,
		                int(selected_scene->models.size())))
		{
			selected_model = selected_scene->models[selected_model_index].model;
			selected_mesh_index = 0;
			selected_material_index = selected_model->m_meshes[selected_mesh_index].m_material_idx;
		}

		///////////////////////////////////////////////////////////////////////////
		// List all meshes in the model and show properties for the selected
		///////////////////////////////////////////////////////////////////////////

		if(ImGui::CollapsingHeader("Meshes", "meshes_ch", true, true))
		{
			if(ImGui::ListBox("Meshes", &selected_mesh_index, mesh_getter, (void*)&selected_model->m_meshes,
			                  int(selected_model->m_meshes.size()), 5))
			{
				selected_material_index = selected_model->m_meshes[selected_mesh_index].m_material_idx;
			}

			labhelper::Mesh& mesh = selected_model->m_meshes[selected_mesh_index];
			ImGui::LabelText("Mesh Name", "%s", mesh.m_name.c_str());
		}

		///////////////////////////////////////////////////////////////////////////
		// List all materials in the model and show properties for the selected
		///////////////////////////////////////////////////////////////////////////
		if(ImGui::CollapsingHeader("Material", "materials_ch", true, true))
		{
			labhelper::Material& material = selected_model->m_materials[selected_material_index];
			ImGui::LabelText("Material Name", "%s", material.m_name.c_str());
			ImGui::ColorEdit3("Color", &material.m_color.x);
			ImGui::SliderFloat("Metalness", &material.m_metalness, 0.0f, 1.0f);
			ImGui::SliderFloat("Fresnel", &material.m_fresnel, 0.0f, 1.0f);
			ImGui::SliderFloat("Shininess", &material.m_shininess, 0.0f, 5000.0f, "%.3f", 2);
			ImGui::ColorEdit3("Emission", &material.m_emission.x);
			ImGui::SliderFloat("Transparency", &material.m_transparency, 0.0f, 1.0f);
			//ImGui::SliderFloat("IoR", &material.m_ior, 0.1f, 3.0f);
		}

#if ALLOW_SAVE_MATERIALS
		if(ImGui::Button("Save Materials"))
		{
			labhelper::saveModelMaterialsToMTL(selected_model,
			                                   labhelper::file::change_extension(selected_model->m_filename,
			                                                                     ".mtl"));
		}
#endif
	}

	///////////////////////////////////////////////////////////////////////////
	// Light and environment map
	///////////////////////////////////////////////////////////////////////////
	if(ImGui::CollapsingHeader("Light sources", "lights_ch", true, true))
	{
		ImGui::Checkbox("Show Light Overlays", &showLightSources);
		ImGui::SliderFloat("Environment multiplier", &pathtracer::environment.multiplier, 0.0f, 10.0f);
		ImGui::Separator();
		ImGui::Text("Point Light");
		ImGui::ColorEdit3("Point light color", &pathtracer::point_light.color.x);
		ImGui::SliderFloat("Point light intensity multiplier", &pathtracer::point_light.intensity_multiplier,
		                   0.0f, 10000.0f);
		ImGui::DragFloat3("Position", &pathtracer::point_light.position.x, 0.1);

		for(int i = 0; i < pathtracer::disc_lights.size(); ++i)
		{
			ImGui::PushID(i);
			ImGui::Separator();
			auto& l = pathtracer::disc_lights[i];
			ImGui::Text("Disc Light %d", i);
			ImGui::ColorEdit3("Color", &l.color.x);
			ImGui::SliderFloat("Intensity", &l.intensity_multiplier, 0.0f, 10000.0f, "%.3f", 3);
			ImGui::DragFloat3("Position", &l.position.x, 0.1);

			glm::vec2 dir(atan2(l.direction.z, l.direction.x) / (2 * M_PI) + 0.5, acos(l.direction.y) / M_PI);
			ImGui::DragFloat2("Direction", &dir.x, 0.01, 0, 1);
			dir.x -= 0.5;
			dir.x *= 2 * M_PI;
			dir.y *= M_PI;
			l.direction = vec3(cos(dir.x) * sin(dir.y), cos(dir.y), sin(dir.x) * sin(dir.y));

			ImGui::DragFloat("Radius", &l.radius, 1, 0, 100);
			ImGui::PopID();
		}
	}

	ImGui::End(); // Control Panel
}

int main(int argc, char* argv[])
{
	g_window = labhelper::init_window_SDL("Pathtracer", 1280, 720);

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
