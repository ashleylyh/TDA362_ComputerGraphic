#version 410
///////////////////////////////////////////////////////////////////////////////
// Input vertex attributes
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec2 position;
layout(location = 2) in vec2 texCoordIn;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////

uniform mat4 normalMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform sampler2D heightField;
uniform int tesselation;
uniform float scale;

///////////////////////////////////////////////////////////////////////////////
// Output to fragment shader
///////////////////////////////////////////////////////////////////////////////
out vec2 texCoord;
out vec3 viewSpacePosition;
out vec3 viewSpaceNormal;

void main()
{
	float height = texture(heightField, texCoordIn).r * scale;
	vec3 mappedPos = vec3(position.x, height, position.y);
	
	// Estimate normal.
	float delta = 1.0 / tesselation;
	float du = texture(heightField, texCoordIn + delta*vec2(-1,0)).r
		     - texture(heightField, texCoordIn + delta*vec2(1,0)).r;
	float dv = texture(heightField, texCoordIn + delta*vec2(0,-1)).r
		     - texture(heightField, texCoordIn + delta*vec2(0,1)).r;
	// Small term to avoid y-only normals that lead to ugly bands of fresnel.
	float fudge = 0.01;
	vec3 normalIn = normalize(vec3(
		fudge + scale * du / (delta * 2),
		1.0,
		fudge + scale * dv / (delta * 2)));

	gl_Position = modelViewProjectionMatrix * vec4(mappedPos, 1.0);
	texCoord = texCoordIn;
	viewSpaceNormal = (normalMatrix * vec4(normalIn, 0.0)).xyz;
	viewSpacePosition = (modelViewMatrix * vec4(mappedPos, 1.0)).xyz;
}
