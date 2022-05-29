$input v_view, v_normal, v_light, v_shadowcoord

#include "../bgfx/examples/common/common.sh"

uniform vec4 u_baseColorBlinn;
uniform vec4 u_ambient;
uniform vec4 u_glossBlinn;
uniform vec4 u_shadowSoft;

SAMPLER2DSHADOW(s_shadowMap, 0);

vec3 ACESToneMapping(vec3 color)
{
    float A = 2.51;
    float B = 0.03;
    float C = 2.43;
    float D = 0.59;
    float E = 0.14;
    return (color * (A * color + B)) / (color * (C * color + D) + E);
}

float hardShadow(sampler2DShadow _sampler, vec4 _shadowCoord, float _bias)
{
	vec3 texCoord = _shadowCoord.xyz/_shadowCoord.w;
	return shadow2D(_sampler, vec3(texCoord.xy, texCoord.z-_bias) );
}

float PCF(sampler2DShadow _sampler, vec4 _shadowCoord, float _bias, vec2 _texelSize)
{
	vec2 texCoord = _shadowCoord.xy/_shadowCoord.w;

	bool outside = any(greaterThan(texCoord, vec2_splat(1.0)))
				|| any(lessThan   (texCoord, vec2_splat(0.0)))
				 ;

	if (outside)
	{
		return 1.0;
	}

	float result = 0.0;
	vec2 offset = _texelSize * _shadowCoord.w;

	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-1.5, -1.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-1.5, -0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-1.5,  0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-1.5,  1.5) * offset, 0.0, 0.0), _bias);

	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-0.5, -1.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-0.5, -0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-0.5,  0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-0.5,  1.5) * offset, 0.0, 0.0), _bias);

	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(0.5, -1.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(0.5, -0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(0.5,  0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(0.5,  1.5) * offset, 0.0, 0.0), _bias);

	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(1.5, -1.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(1.5, -0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(1.5,  0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(1.5,  1.5) * offset, 0.0, 0.0), _bias);

	return result / 16.0;
}

void main()
{
	//params
	float shadowMapBias = 0.005;
	vec3 baseColor = u_baseColorBlinn.xyz;
	float gloss = u_glossBlinn.x;
	float shadowSoft = u_shadowSoft[0];

	//vectors
	vec3 viewDir  = normalize(v_view);
	vec3 normalDir  = v_normal;
	vec3 lightDir  = normalize(v_light);
	vec3 halfDir = normalize(viewDir + lightDir);

	//dot products
	float ndotl = max(dot(normalDir, lightDir), 0.0);
	float ndoth = max(dot(normalDir, halfDir), 0.0);

	//lighting
	vec3 diffuse = ndotl * baseColor;
	vec3 specular = vec3_splat(step(0.0, ndotl) * pow(max(0.0, ndoth), gloss) * (2.0 + gloss)/8.0);
	vec3 ambient = u_ambient.xyz * 0.1;

	//visibility
	vec2 texelSize = vec2_splat(1.0/512.0);
	float hard = hardShadow(s_shadowMap, v_shadowcoord, shadowMapBias);
	float soft = PCF(s_shadowMap, v_shadowcoord, shadowMapBias, texelSize);

	float visibility = hard * (1.0 - shadowSoft) + soft * shadowSoft;

	vec3 brdf = (diffuse + specular) * visibility + ambient;
	gl_FragColor.xyz = ACESToneMapping(brdf);
	gl_FragColor.w = 1.0;
}
