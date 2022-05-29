#include <bx/uint32_t.h>
#include "common.h"
#include "entry/cmd.h"
#include "entry/input.h"

#define CAMERA_KEY_FORWARD   UINT8_C(0x01)
#define CAMERA_KEY_BACKWARD  UINT8_C(0x02)
#define CAMERA_KEY_LEFT      UINT8_C(0x04)
#define CAMERA_KEY_RIGHT     UINT8_C(0x08)


void cameraSetKeyState(uint8_t _key, bool _down);


static void cmd(const void* _userData)
{
	cmdExec((const char*)_userData);
}

int cmdCamMove(CmdContext* /*_context*/, void* /*_userData*/, int _argc, char const* const* _argv)
{
	if (_argc > 1)
	{
		if (0 == bx::strCmp(_argv[1], "forward"))
		{
			cameraSetKeyState(CAMERA_KEY_FORWARD, true);
			return 0;
		}
		else if (0 == bx::strCmp(_argv[1], "left"))
		{
			cameraSetKeyState(CAMERA_KEY_LEFT, true);
			return 0;
		}
		else if (0 == bx::strCmp(_argv[1], "right"))
		{
			cameraSetKeyState(CAMERA_KEY_RIGHT, true);
			return 0;
		}
		else if (0 == bx::strCmp(_argv[1], "backward"))
		{
			cameraSetKeyState(CAMERA_KEY_BACKWARD, true);
			return 0;
		}
	}
	return 1;
}

static const InputBinding s_bindingCamera[] =
{
	{ entry::Key::KeyW,      entry::Modifier::None,       0, cmd, "move forward"         },
	{ entry::Key::KeyS,      entry::Modifier::None,       0, cmd, "move backward"         },
	{ entry::Key::KeyA,      entry::Modifier::None,       0, cmd, "move left"         },
	{ entry::Key::KeyD,      entry::Modifier::None,       0, cmd, "move right"         },

	INPUT_BINDING_END
};



struct Mouse
{
	Mouse()
	{
		m_dx = 0.0f;
		m_dy = 0.0f;
		m_prevMx = 0.0f;
		m_prevMy = 0.0f;
		m_scroll = 0;
		m_scrollPrev = 0;
	}

	void update(float _mx, float _my, int32_t _mz, uint32_t _width, uint32_t _height)
	{
		const float widthf = float(int32_t(_width));
		const float heightf = float(int32_t(_height));

		// Delta movement.
		m_dx = float(_mx - m_prevMx) / widthf;
		m_dy = float(_my - m_prevMy) / heightf;

		m_prevMx = _mx;
		m_prevMy = _my;

		// Scroll.
		m_scroll = _mz - m_scrollPrev;
		m_scrollPrev = _mz;
	}

	float m_dx; // Screen space.
	float m_dy;
	float m_prevMx;
	float m_prevMy;
	int32_t m_scroll;
	int32_t m_scrollPrev;
};

class Camera
{
public:
	Camera(const bx::Vec3& _center = bx::init::Zero, float _distance = 3.0f, float _near = 0.01f, float _far = 100.0f)
	{
		init(bx::init::Zero, 3.0f, 0.01f, 100.0f);
		cmdAdd("move", cmdCamMove);
		inputAddBindings("camBindings", s_bindingCamera);
	}

	~Camera()
	{
		cmdRemove("move");
		inputRemoveBindings("camBindings");
	}

	void setKeyState(uint8_t _key, bool _down)
	{
		m_keys &= ~_key;
		m_keys |= _down ? _key : 0;
	}

	void init(const bx::Vec3& _center = bx::init::Zero, float _distance = 3.0f, float _near = 0.01f, float _far = 100.0f)
	{
		m_target.curr = _center;
		m_target.dest = _center;

		m_pos.curr = _center;
		m_pos.curr.z += _distance;
		m_pos.dest = _center;
		m_pos.dest.z += _distance;

		m_orbit[0] = 0.0f;
		m_orbit[1] = 0.0f;

		m_near = _near;
		m_far = _far;
	}
	void mtxLookAt(float* _outViewMtx)
	{
		bx::mtxLookAt(_outViewMtx, m_pos.curr, m_target.curr);
	}
	//the move of the mouse
	void orbit(float _dx, float _dy)
	{
		m_orbit[0] += _dx;
		m_orbit[1] += _dy;
	}

	void distance(float _z)
	{
		_z = bx::clamp(_z, m_near, m_far);

		bx::Vec3 toTarget = bx::sub(m_target.dest, m_pos.dest);
		bx::Vec3 toTargetNorm = bx::normalize(toTarget);

		m_pos.dest = bx::mad(toTargetNorm, -_z, m_target.dest);
	}
	//caculate the distance move when middle rolls
	void dolly(float _dz)
	{
		const bx::Vec3 toTarget = bx::sub(m_target.dest, m_pos.dest);
		const float toTargetLen = bx::length(toTarget);
		const float invToTargetLen = 1.0f / (toTargetLen + bx::kFloatMin);
		const bx::Vec3 toTargetNorm = bx::mul(toTarget, invToTargetLen);

		float delta = toTargetLen * _dz;
		float newLen = toTargetLen - delta;

		if ((m_near < newLen || _dz < 0.0f)
			&& (newLen < m_far || _dz > 0.0f))
		{
			m_pos.dest = bx::mad(toTargetNorm, delta, m_pos.dest);
		}
	}
	//caculate position when orbit happen
	void consumeOrbit(float _amount)
	{
		float consume[2];
		consume[0] = m_orbit[0] * _amount;
		consume[1] = m_orbit[1] * _amount;
		m_orbit[0] -= consume[0];
		m_orbit[1] -= consume[1];

		const bx::Vec3 toPos = bx::sub(m_pos.curr, m_target.curr);
		const float toPosLen = bx::length(toPos);
		const float invToPosLen = 1.0f / (toPosLen + bx::kFloatMin);
		const bx::Vec3 toPosNorm = bx::mul(toPos, invToPosLen);

		float ll[2];
		bx::toLatLong(&ll[0], &ll[1], toPosNorm);
		ll[0] += consume[0];
		ll[1] -= consume[1];
		ll[1] = bx::clamp(ll[1], 0.02f, 0.98f);

		const bx::Vec3 tmp = bx::fromLatLong(ll[0], ll[1]);
		const bx::Vec3 diff = bx::mul(bx::sub(tmp, toPosNorm), toPosLen);

		m_pos.curr = bx::add(m_pos.curr, diff);
		m_pos.dest = bx::add(m_pos.dest, diff);
	}

	void update(float _dt, const entry::MouseState& m_mouseState, uint32_t m_width, uint32_t m_height, bool mouse_OverGui)
	{
		if (m_keys & CAMERA_KEY_FORWARD)
		{
			orbit(0.0f, -0.01f);
			setKeyState(CAMERA_KEY_FORWARD, false);
		}

		if (m_keys & CAMERA_KEY_BACKWARD)
		{
			orbit(0.0f, +0.01f);
			setKeyState(CAMERA_KEY_BACKWARD, false);
		}

		if (m_keys & CAMERA_KEY_LEFT)
		{
			orbit(+0.01f, 0.0f);
			setKeyState(CAMERA_KEY_LEFT, false);
		}

		if (m_keys & CAMERA_KEY_RIGHT)
		{
			orbit(-0.01f, 0.0f);
			setKeyState(CAMERA_KEY_RIGHT, false);
		}
		m_mouse.update(float(m_mouseState.m_mx), float(m_mouseState.m_my), m_mouseState.m_mz, m_width, m_height);
		if (!mouse_OverGui)
		{
			if (m_mouseState.m_buttons[entry::MouseButton::Left])
			{
				orbit(m_mouse.m_dx, m_mouse.m_dy);
			}
			else if (m_mouseState.m_buttons[entry::MouseButton::Right])
			{
				orbit(m_mouse.m_dx, m_mouse.m_dy);
			}
			else if (0 != m_mouse.m_scroll)
			{
				dolly(float(m_mouse.m_scroll) * 0.1f);
			}
		}
		const float amount = bx::min(_dt / 0.12f, 1.0f);

		consumeOrbit(amount);

		m_target.curr = bx::lerp(m_target.curr, m_target.dest, amount);
		m_pos.curr = bx::lerp(m_pos.curr, m_pos.dest, amount);
	}

	struct Interp3f
	{
		bx::Vec3 curr = bx::init::None;
		bx::Vec3 dest = bx::init::None;
	};

	Interp3f m_target;
	Interp3f m_pos;
	float m_orbit[2];
	float m_near, m_far;
	Mouse m_mouse;
	uint8_t m_keys;
};

static Camera* s_camera = NULL;

void cameraCreate(const bx::Vec3& _center = bx::init::Zero, float _distance = 3.0f, float _near = 0.01f, float _far = 100.0f)
{
	s_camera = BX_NEW(entry::getAllocator(), Camera);
	s_camera->init(_center, _distance, _near, _far);
}

void cameraDestroy()
{
	BX_DELETE(entry::getAllocator(), s_camera);
	s_camera = NULL;
}

void cameraSetKeyState(uint8_t _key, bool _down)
{
	s_camera->setKeyState(_key, _down);
}

void cameraGetViewMtx(float* _viewMtx)
{
	s_camera->mtxLookAt(_viewMtx);
}

bx::Vec3 cameraGetPosition()
{
	return s_camera->m_pos.curr;
}

void cameraUpdate(float _deltaTime, const entry::MouseState& _mouseState, uint32_t _width, uint32_t _height, bool _mouseOverGui)
{
	s_camera->update(_deltaTime, _mouseState, _width, _height, _mouseOverGui);
}
