$input a_position, a_normal
$output v_viewDir, v_worldPos, v_lightDir, v_normal, v_color0
#include "../bgfx/examples/common/common.sh"

uniform vec4 u_lightPos;
uniform vec4 u_viewPos;
uniform vec4 u_baseColorBlinn;

void main()
{
	vec3 worldPos = mul(u_model[0], vec4(a_position, 1.0)).xyz;
	//vec3 normal = a_normal * 2.0 - 1.0;

	v_normal = mul(u_model[0], vec4(a_normal,1.0)).xyz;

	//viewDir in world space
	v_viewDir = u_viewPos.xyz- worldPos;
	
	//lightDir in world space
	v_lightDir = u_lightPos.xyz - worldPos;

	gl_Position = mul(u_viewProj, vec4(worldPos, 1.0));
	v_color0 = u_baseColorBlinn;
}