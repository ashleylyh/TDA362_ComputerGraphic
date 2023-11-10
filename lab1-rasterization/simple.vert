#version 420

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

// Task 3: Add an output variable to pass colors to the fragment shader

void main()
{
	gl_Position = vec4(in_position, 1.0);

	// Task 3: Set the color variable to the vertex color
}
