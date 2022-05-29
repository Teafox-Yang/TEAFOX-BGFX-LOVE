$input v_dir

#include "../bgfx/examples/common/common.sh"

SAMPLERCUBE(s_texCubeLod, 0);

uniform vec4 u_ambient;

void main()
{
	vec3 dir = normalize(v_dir);

	vec4 color;
	float lod = 0.0;
	dir = fixCubeLookup(dir, lod, 256.0);
	color = toLinear(textureCubeLod(s_texCubeLod, dir, lod));
	color *= exp2(u_ambient.w);
	gl_FragColor = toFilmic(color);
}
