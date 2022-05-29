$input a_position, a_normal, a_tangent, a_texcoord0
$output v_worldPos, v_viewDir, v_lightDir, v_normal, v_tangent, v_bitangent, v_texcoord0

#include "../bgfx/examples/common/common.sh"

uniform vec4 u_lightPos;
uniform vec4 u_viewPos;

void main()
{
	v_worldPos = mul(u_model[0], vec4(a_position, 1.0)).xyz;
	//vec4 normal = a_normal * 2.0 - 1.0;

	vec3 worldNormal = mul(u_model[0], vec4(a_normal.xyz, 0.0)).xyz;
	vec3 worldTangent = mul(u_model[0], vec4(a_tangent.xyz, 0.0)).xyz;

	v_normal = normalize(worldNormal);
	v_tangent = normalize(worldTangent);
	v_bitangent = cross(v_normal, v_tangent) * a_tangent.w;
	mat3 tbn = mtxFromCols(v_tangent, v_bitangent, v_normal);

	//viewDir in tangent space
	v_viewDir = mul(u_viewPos.xyz - v_worldPos, tbn);
	//lightDir in tangent space
	v_lightDir = mul(u_lightPos.xyz - v_worldPos, tbn);
	//lightDir in tangent space
	gl_Position = mul(u_viewProj, vec4(v_worldPos, 1.0));
	v_texcoord0 = a_texcoord0;

}