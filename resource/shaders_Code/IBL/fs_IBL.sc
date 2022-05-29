$input v_worldPos, v_viewDir, v_lightDir, v_normal, v_tangent, v_bitangent, v_texcoord0


#include "../bgfx/examples/common/common.sh"


SAMPLER2D(s_texColor,  0);
SAMPLER2D(s_texNormal, 1);
SAMPLER2D(s_texAORM, 2);
SAMPLERCUBE(s_texCubeIrr, 3);
SAMPLERCUBE(s_texCubeLod, 4);
SAMPLER2D(s_brdfLut, 5);

uniform vec4 u_ambient;
uniform vec4 u_lightCol;
uniform vec4 u_lightPos;
uniform vec4 u_iblSetting;

vec3 Fresnel(float vdoth, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - vdoth, 5.0);
}

float GGX_NDF(float ndoth, float a)
{
	float a2 = a * a;
	float temp = ndoth * ndoth * (a2 - 1.0) + 1.0;
	temp = 3.1415 * temp * temp;
	return a2/temp;
}

float GGX_Geometry(float dotResult, float k)
{
	return dotResult/(dotResult * (1.0 - k) + k);
}

vec3 ACESToneMapping(vec3 color)
{
    float A = 2.51;
    float B = 0.03;
    float C = 2.43;
    float D = 0.59;
    float E = 0.14;
    return (color * (A * color + B)) / (color * (C * color + D) + E);
}

vec3 FresnelSchlickRoughness(float ndotv, vec3 F0, float roughness)
{
    return F0 + (max(vec3_splat(1.0 - roughness), F0) - F0) * pow(1.0 - ndotv, 5.0);
}  

void main()
{
	mat3 tbn = mtxFromCols(v_tangent, v_bitangent, v_normal);

	vec2 v_texcoord = vec2(v_texcoord0.x, 1.0 - v_texcoord0.y);

	//sample textures
	vec3 normal;
	normal.xy = texture2D(s_texNormal, v_texcoord).xy * 2.0 - 1.0;
	normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));

	//calculate worldNormal for sample
	vec3 worldNormal = mul(tbn, normal);

	vec3 albedo = toLinear(texture2D(s_texColor, v_texcoord)).xyz;
	vec3 AORM = texture2D(s_texAORM, v_texcoord).xyz;
	float ao = AORM.x;
	float roughness = AORM.y;
	float metallic = AORM.z;

	//caculate directlight radience
    float distance    = length(u_lightPos.xyz - v_worldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = u_lightCol.xyz * attenuation; 

	//calculate vectors
	vec3 viewDir = normalize(v_viewDir);
	vec3 lightDir = normalize(v_lightDir);
	vec3 halfDir = normalize(lightDir + viewDir);
	vec3 reflectDir = normalize(reflect(-lightDir, normal));
	vec3 reflectDirWorld = mul(tbn, reflectDir);

	float ndotl = max(dot(lightDir, normal), 0.0);
	float ndoth = max(dot(halfDir, normal), 0.0);
	float ndotv = max(dot(viewDir, normal), 0.0);
	float vdoth = max(dot(halfDir, viewDir), 0.0);

	//calculate fresnel 
	vec3 F0 = vec3_splat(0.04);
	F0 = mix(F0, albedo, metallic);
	vec3 fresnelDirect = Fresnel(vdoth, F0);
	vec3 fresnelAmbient = FresnelSchlickRoughness(ndotv, F0, roughness);

	//IBL ks,ka
	vec3 KSA = fresnelAmbient;
	vec3 KDA = 1.0 - KSA;
	KDA *= 1.0 - metallic;

	//PBR ks,ka
	vec3 KS = fresnelDirect;
	vec3 KD = 1.0 - KS;
	KD *= 1.0 - metallic;

	//IBL Lighting
	//diffuse
	vec3 irr = toLinear(textureCube(s_texCubeIrr, worldNormal)).xyz * exp2(u_ambient.w);
	vec3 diffAmbient = KD * albedo * irr;

	//specular
	float MAX_REFLECTION_LOD = 7.0;
	float lod = roughness * MAX_REFLECTION_LOD;
	reflectDirWorld = fixCubeLookup(reflectDirWorld, lod, 256.0);
	vec3 specAmbient = toLinear(textureCubeLod(s_texCubeLod, reflectDirWorld, lod)).xyz * exp2(u_ambient.w);
	vec2 envBRDF  = texture2D(s_brdfLut, vec2(ndotv, roughness)).xy;
	specAmbient = specAmbient * (KS * envBRDF.x + envBRDF.y);

	//PBR lighting

	//calculate NDF
	float ndf = GGX_NDF(ndoth, roughness);

	//calculate Geometry
	float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
	float geometry = GGX_Geometry(ndotv, k) * GGX_Geometry(ndotl, k);
	float vis = geometry/(4.0 * ndotl * ndotv);

	vec3 diffuse = KD * albedo / 3.1415;
	vec3 specular = vis * ndf * fresnelDirect;
	vec3 Lo = (diffuse + specular) * radiance * ndotl;
   

	vec3 color = (diffAmbient * u_iblSetting[0] + specAmbient * u_iblSetting[1]) * ao + (diffuse * u_iblSetting[2] + specular * u_iblSetting[3]) * radiance * ndotl;
	gl_FragColor.xyz = ACESToneMapping(color);
	gl_FragColor.w = 1.0;
	gl_FragColor = toGamma(gl_FragColor);
}
