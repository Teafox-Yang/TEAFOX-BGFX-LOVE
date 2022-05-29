$input v_position

#include "../bgfx/examples/common/common.sh"

void main()
{
	gl_FragColor = vec4_splat(v_position.z);
}
