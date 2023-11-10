#version 420

layout(location = 0) in vec3 in_position;

// Task 1: Add input and output variables for the texture coordinates

uniform mat4 projectionMatrix;
uniform vec3 cameraPosition;


void main()
{
	// This is a hardcoded rotation matrix we use to have the camera look slightly downward
	// In lab 3 you will build and use a proper view matrix with helper functions
	mat3 V = mat3(
	1.0, 0.0, 0.0,
	0.0, 0.97, 0.26,
	0.0, -0.26, 0.97);

	vec4 pos = vec4((V * in_position.xyz) - cameraPosition.xyz, 1);
	gl_Position = projectionMatrix * pos;

	// Task 1: Copy the value received for the texcoord to the out variable sent to the fragment shader
}
