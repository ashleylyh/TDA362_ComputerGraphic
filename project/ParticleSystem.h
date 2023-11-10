#pragma once

#include <GL/glew.h>
#include <vector>
#include <glm/detail/type_vec3.hpp>
#include <glm/mat4x4.hpp>

struct Particle
{
	glm::vec3 pos;
	glm::vec3 velocity;
	float lifetime;
	float life_length;
};

class ParticleSystem
{
public:
	/// Allocates the gpu buffer to hold up to `capacity` particles and the corresponding vao
	explicit ParticleSystem(int capacity);

	/// Clean up the gpu structures created in the constructor
	~ParticleSystem();

	void init_gpu_data();

	/// Creates a new particle if there less than `max_size` elements in the particles array buffer
	void spawn(Particle particle);

	/// Updates all the particles' positions depending on their speed, their lifetimes, and kills any
	/// that are past their life_length
	void process_particles(float dt);

	/// Updates the vertex buffer with the current particle properties, and renders them
	void submit_to_gpu(const glm::mat4& viewMat);

private:
	/// Deletes a particle at position `id` by swapping it with the last
	/// particle in the array and reducing the size by 1.
	/// (Care with indexes, as this can change the particle an index refers to)
	void kill(int id);

	// Members
	std::vector<Particle> particles;
	int max_size;

	GLuint gl_vao = 0;
	GLuint gl_buffer = 0;
	std::vector<glm::vec4> gl_data_temp_buffer;
};
