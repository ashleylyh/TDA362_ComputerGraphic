#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

// Task 1: Receive the texture coordinates

// Task 3.4: Receive the texture as a uniform

layout(location = 0) out vec4 fragmentColor;

void main()
{
	// Task 1: Use the texture coordinates for the x,y of the color output
	// Task 3.5: Sample the texture with the texture coordinates and use that for the color
	fragmentColor = vec4(1.0, 1.0, 1.0, 1.0);
}
