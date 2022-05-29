$input a_position
$output v_dir

#include "../bgfx/examples/common/common.sh"

void main()
{
	mat4 view = u_view;
	view[0][3] = 0.0;
	view[1][3] = 0.0;
	view[2][3] = 0.0;
	mat4 proj = u_proj;
	mat4 viewProj = mul(proj, view);
	vec4 pos = mul(viewProj, vec4(a_position, 1.0) );
	gl_Position = pos;

	v_dir = normalize(a_position.xyz);
}
