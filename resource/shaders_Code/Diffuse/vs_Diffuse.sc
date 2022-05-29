$input a_position, a_normal
$output v_normal, v_lightDir, v_color0

#include "../bgfx/examples/common/common.sh"

uniform vec4 u_lightPos;

void main()
{
	vec3 worldPos = mul(u_model[0], vec4(a_position, 1.0)).xyz;
	v_lightDir = u_lightPos.xyz - worldPos;

	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0) );

	v_normal = a_normal * 2.0 - 1.0;
	v_normal = mul(u_model[0], vec4(v_normal, 0.0) ).xyz;
}
