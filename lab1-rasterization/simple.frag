#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

// Task 3: Add an input variable for colors from the vertex shader

layout(location = 0) out vec4 fragmentColor;

// Task 5: Add a uniform to tint all the pixels by some color

void main()
{
// Task 3: Set the output color to be the incoming interpolated color received
	fragmentColor = vec4(1.0, 1.0, 1.0, 1.0);
}
