$input v_normal, v_lightDir, v_color0

#include "../bgfx/examples/common/common.sh"

void main()
{
	vec3 lightDir = normalize(v_lightDir);

	float ndotl = dot(normalize(v_normal), lightDir);
	float diffuse = ndotl * 0.5 + 0.5;

	gl_FragColor = vec4(vec3_splat(diffuse), 1.0);
}
