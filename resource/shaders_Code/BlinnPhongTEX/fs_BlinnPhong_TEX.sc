$input v_worldPos, v_viewDir, v_lightDir, v_normal, v_tangent, v_bitangent, v_texcoord0


#include "../bgfx/examples/common/common.sh"


SAMPLER2D(s_texColor,  0);
SAMPLER2D(s_texNormal, 1);

uniform vec4 u_ambient;
uniform vec4 u_glossBlinn;
uniform vec4 u_lightCol;
uniform vec4 u_lightPos;

vec3 ACESToneMapping(vec3 color)
{
    float A = 2.51;
    float B = 0.03;
    float C = 2.43;
    float D = 0.59;
    float E = 0.14;
    return (color * (A * color + B)) / (color * (C * color + D) + E);
}

void main()
{
	mat3 tbn = mtxFromCols(v_tangent, v_bitangent, v_normal);
	vec3 specColor = vec3(1.0, 1.0, 1.0);
	float gloss = u_glossBlinn.x;

	//caculate radience
    float distance    = length(u_lightPos.xyz - v_worldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = u_lightCol.xyz * attenuation; 

    vec2 v_texcoord = vec2(v_texcoord0.x, 1.0 - v_texcoord0.y);

	//caculate vectors
	vec3 normal;
	normal.xy = texture2D(s_texNormal, v_texcoord).xy * 2.0 - 1.0;
	normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
	vec3 viewDir = normalize(v_viewDir);
	vec3 lightDir = normalize(v_lightDir);
	vec3 halfDir = normalize(lightDir + viewDir);

	//caculate light model
	vec3 albedo = toLinear(texture2D(s_texColor, v_texcoord)).xyz;

	vec3 ambient = u_ambient.xyz * 0.05;
	vec3 diffuse = albedo * max(0, dot(normal, lightDir));
	vec3 specular = specColor * pow(max(0, dot(normal,halfDir)), gloss);
	vec3 color = (diffuse + specular) * radiance + ambient;
	
	gl_FragColor.xyz = ACESToneMapping(color);
	gl_FragColor.w = 1.0;
	gl_FragColor = toGamma(gl_FragColor);
}
