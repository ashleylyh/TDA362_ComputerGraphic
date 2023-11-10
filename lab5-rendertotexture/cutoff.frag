#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

layout(binding = 0) uniform sampler2D frameBufferTexture;
uniform float time;
layout(location = 0) out vec4 fragmentColor;

/**
* Helper function to sample with pixel coordinates, e.g., (511.5, 12.75)
* This functionality is similar to using sampler2DRect.
* TexelFetch only work with integer coordinates and do not perform bilinerar filtering.
*/
vec4 textureRect(in sampler2D tex, vec2 rectangleCoord)
{
	return texture(tex, rectangleCoord / textureSize(tex, 0));
}

/**
 * Implements cutoff to ensure only bright parts of the screen gets blurred to produce bloom.
 */
void main()
{
	float cutAt = 0.80;
	vec4 rgbSample = textureRect(frameBufferTexture, gl_FragCoord.xy);
	if(rgbSample.r > cutAt || rgbSample.g > cutAt || rgbSample.b > cutAt)
	{
		fragmentColor = rgbSample;
	}
	else
	{
		fragmentColor = vec4(0.0);
	}
}
