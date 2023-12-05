#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

layout(location = 0) out vec4 fragmentColor;

layout(binding = 0) uniform sampler2D environmentMap;

in vec2 texCoord;

uniform uint num_samples;

uniform uint samples_taken;

uniform float roughness;


const float M_PI = 3.1415926538;

uvec3 pcg3d(uvec3 v)
{
	v = v * 1664525u + 1013904223u;

	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;

	v = v ^ (v >> 16u);

	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;

	return v;
}

vec3 pcg3df(vec3 v)
{
	uvec3 uv = floatBitsToUint(v);

	uvec3 m = pcg3d(uv);

	// Construct a float with half-open range [0:1] using low 23 bits.
	// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
	// From https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
	const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
	const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

	m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
	m |= ieeeOne;                          // Add fractional part to 1.0

	vec3 f = uintBitsToFloat( m );       // Range [1:2]
	return f - 1.0;                        // Range [0:1]
}

vec3 rand3f(float idx)
{
	return pcg3df(vec3(gl_FragCoord.xy, idx));
}

mat3 tangentSpace(vec3 n)
{
	float s = 1.0;
	if (n.z < 0)
	{
		s = -1.0;
	}
	const float a = -1.0 / (s + n.z);
	const float b = n.x * n.y * a;
	mat3 r;
	r[0] = vec3( 1.0 + s * n.x * n.x * a, s * b, -s * n.x );
	r[1] = vec3( b, s + n.y * n.y * a, -n.y );
	r[2] = n;
	return r;
}


void main()
{
	fragmentColor.xyz = rand3f(float(samples_taken));
}
