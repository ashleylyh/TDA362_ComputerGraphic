#version 420
///////////////////////////////////////////////////////////////////////////////
// Input vertex attributes
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texCoordIn;
layout(binding = 0) uniform sampler2D hf;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////

uniform mat4 normalMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 lightMatrix;
uniform float displaceNormal;

///////////////////////////////////////////////////////////////////////////////
// Output to fragment shader
///////////////////////////////////////////////////////////////////////////////
out vec2 texCoord;
out vec3 viewSpacePosition;
out vec3 viewSpaceNormal;
out vec4 shadowMapCoord;

vec2 uvCoords(vec2 xzCoords)
{
	return (xzCoords + 1) / 2;
}

vec4 normalCalc()
{
	float off = 0.01f;
	float hX = texture2D(hf, uvCoords(position.xz + vec2(off, 0))).r;
	float hZ = texture2D(hf, uvCoords(position.xz + vec2(0, off))).r;
	float currentHeight = 3*  texture2D(hf, texCoordIn.xy).r;

	vec3 heightPos = vec3(position.x, currentHeight*10, position.z);

	vec3 slopeX = vec3(position.x + off, hX, position.z) - heightPos/2;
	vec3 slopeZ = vec3(position.x, hZ, position.z + off) - heightPos/2;

	vec3 normal = cross(normalize(slopeX), normalize(slopeZ));
	return -normalize(vec4(normal,0));
}

void main() 
{
	viewSpaceNormal = (normalMatrix * normalCalc()).xyz;
	viewSpacePosition = (modelViewMatrix * vec4(position,1.0f)).xyz;
	float height = texture2D(hf, texCoordIn.xy).r * 3;
	gl_Position = modelViewProjectionMatrix * vec4(position.x, height, position.z, 1.0) + normalize(vec4(viewSpaceNormal,0)) * displaceNormal;
	texCoord = texCoordIn;
	shadowMapCoord = lightMatrix * vec4(viewSpacePosition, 1.0f);
}