#include "ParticleSystem.h"
ParticleSystem::ParticleSystem(int capacity) : max_size(capacity)
{
	gl_data_temp_buffer.resize(max_size);
}

ParticleSystem::~ParticleSystem()
{
}

void ParticleSystem::init_gpu_data()
{
}

void ParticleSystem::process_particles(float dt)
{
}

void ParticleSystem::submit_to_gpu(const glm::mat4& viewMat)
{
}

void ParticleSystem::spawn(Particle particle){};

void ParticleSystem::kill(int id){};
