#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

uniform vec3 material_color = vec3(0.0);
uniform vec3 material_emission = vec3(0.0);

uniform int has_color_texture = 0;
layout(binding = 0) uniform sampler2D color_texture;

uniform int has_emission_texture = 0;
layout(binding = 5) uniform sampler2D emission_texture;

in vec2 texCoord;
in vec3 ws_normal;

layout(location = 0) out vec4 fragmentColor;

void main()
{
	vec3 n = normalize(ws_normal);

	vec4 color = vec4(material_color, 1.0);
	if(has_color_texture == 1)
	{
		color = texture(color_texture, texCoord.xy);
	}

	vec4 emission = vec4(material_emission, 0);
	if(has_emission_texture == 1)
	{
		emission = texture(emission_texture, texCoord.xy);
	}

	const vec3 lightDir = normalize(vec3(-0.74, -1, 0.68));
	fragmentColor = color * max(dot(n, -lightDir), 0.3) + emission;
}
