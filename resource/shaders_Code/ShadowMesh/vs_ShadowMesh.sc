$input a_position, a_normal
$output v_view, v_normal, v_light, v_shadowcoord

#include "../bgfx/examples/common/common.sh"

uniform mat4 u_lightMtx;
uniform vec4 u_lightPos;
uniform vec4 u_viewPos;
uniform vec4 u_unpackNormal;

void main()
{
	vec3 unpackedNormal = a_normal.xyz * 2.0 - 1.0;
	vec3 normal = unpackedNormal * u_unpackNormal[0] + a_normal.xyz * (1.0 - u_unpackNormal[0]);
	vec3 worldPos = mul(u_model[0], vec4(a_position, 1.0)).xyz;

	v_normal = normalize(mul(u_model[0], vec4(normal,1.0)).xyz);

	//viewDir in world space
	v_view = u_viewPos.xyz- worldPos;
	
	//lightDir in world space
	v_light = u_lightPos.xyz - worldPos;

	gl_Position = mul(u_viewProj, vec4(worldPos, 1.0));

	const float shadowMapOffset = 0.001;
	vec3 posOffset = a_position + normal * shadowMapOffset;
	v_shadowcoord = mul(u_lightMtx, vec4(posOffset, 1.0) );
}
