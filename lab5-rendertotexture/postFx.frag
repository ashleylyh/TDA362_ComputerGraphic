#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

layout(binding = 0) uniform sampler2D frameBufferTexture;
layout(binding = 1) uniform sampler2D blurredFrameBufferTexture;
uniform float time = 0.f;
uniform int currentEffect = 1; // 1 as default, to know when the framebuffers are properly set
uniform int filterSize = 1;
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
 * Perturps the sampling coordinates of the pixel and returns the new coordinates
 * these can then be used to sample the frame buffer. The effect uses a sine wave to make us
 * feel woozy.
 */
vec2 mushrooms(vec2 inCoord);

/**
 * Samples a region of the frame buffer using gaussian filter weights to blur the image
 * as the kernel width is not that large, it doesnt produce a very large effect. Making it larger
 * is both tedious and expensive, for real time purposes a separable blur is preferable, but this
 * requires several passes.
 * takes as input the centre coordinate to sample around.
 */
vec3 blur(vec2 coord);

/**
 * Simply returns the luminance of the input sample color.
 */
vec3 grayscale(vec3 rgbSample);

/**
 * Converts the color sample to sepia tone (by transformation to the yiq color space).
 */
vec3 toSepiaTone(vec3 rgbSample);




void main()
{
	switch(currentEffect)
	{
	case 0:
		fragmentColor = textureRect(frameBufferTexture, gl_FragCoord.xy);
		break;
	case 1:
		fragmentColor = vec4(toSepiaTone(textureRect(frameBufferTexture, gl_FragCoord.xy).rgb), 1.0);
		break;
	case 2:
		fragmentColor = textureRect(frameBufferTexture, mushrooms(gl_FragCoord.xy));
		break;
	case 3:
		fragmentColor = vec4(blur(gl_FragCoord.xy), 1.0);
		break;
	case 4:
		fragmentColor = vec4(grayscale(textureRect(frameBufferTexture, gl_FragCoord.xy).rgb), 1.0);
		break;
	case 5:
		// all at once
		fragmentColor = vec4(toSepiaTone(blur(mushrooms(gl_FragCoord.xy))), 1.0);
		break;
	case 6:
		fragmentColor = vec4(0.0); // place holder
		break;
	case 7:
		fragmentColor = vec4(0.0); // place holder
		break;
	case 8:
		fragmentColor = vec4(0.0); // place holder
		break;
	case 9:
		fragmentColor = vec4(0.0); // place holder
		break;
	}
}


vec3 toSepiaTone(vec3 rgbSample)
{
	//-----------------------------------------------------------------
	// Variables used for YIQ/RGB color space conversion.
	//-----------------------------------------------------------------
	vec3 yiqTransform0 = vec3(0.299, 0.587, 0.144);
	vec3 yiqTransform1 = vec3(0.596, -0.275, -0.321);
	vec3 yiqTransform2 = vec3(0.212, -0.523, 0.311);

	vec3 yiqInverseTransform0 = vec3(1, 0.956, 0.621);
	vec3 yiqInverseTransform1 = vec3(1, -0.272, -0.647);
	vec3 yiqInverseTransform2 = vec3(1, -1.105, 1.702);

	// transform to YIQ color space and set color information to sepia tone
	vec3 yiq = vec3(dot(yiqTransform0, rgbSample), 0.2, 0.0);

	// inverse transform to RGB color space
	vec3 result = vec3(dot(yiqInverseTransform0, yiq), dot(yiqInverseTransform1, yiq),
	                   dot(yiqInverseTransform2, yiq));
	return result;
}

vec2 mushrooms(vec2 inCoord)
{
	return inCoord + vec2(sin(time * 4.3127 + inCoord.y / 9.0) * 15.0, 0.0);
}

vec3 blur(vec2 coord)
{
	vec3 result = vec3(0.0);
	float weight = 1.0 / (filterSize * filterSize);

	for(float i = -filterSize / 2; i <= filterSize / 2; i += 1.0)
	{
		for(float j = -filterSize / 2; j <= filterSize / 2; j += 1.0)
		{
			result += weight * textureRect(frameBufferTexture, coord + vec2(i, j)).rgb;
		}
	}

	return result;
}

vec3 grayscale(vec3 rgbSample)
{
	return vec3(rgbSample.r * 0.2126 + rgbSample.g * 0.7152 + rgbSample.b * 0.0722);
}
