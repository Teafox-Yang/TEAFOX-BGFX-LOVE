/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bx/uint32_t.h>
#include "common.h"
#include "bgfx_utils.h"
#include "bgfx_logo.h"
#include "imgui/imgui.h"
#include "Camera.h"

namespace
{
	#define RENDER_SHADOW_PASS_ID 0
	#define RENDER_SKY_PASS_ID 1
	#define RENDER_SCENE_PASS_ID  2


	//UI parameters
	struct Settings
	{
		Settings()
		{
			m_lightOrbit = true;
			m_gloss = 32.0f;
			m_lightIntensity = 500.0f;
			m_ambientIntensity = 1.0f;
			m_shaderSelection = 0;
			m_shadowSoft = 1.0f;
			m_baseColor[0] = 0.1;
			m_baseColor[1] = 0.5;
			m_baseColor[2] = 1.0;
			m_baseColor[3] = 1.0;
			m_ambientColor[0] = 0.5;
			m_ambientColor[1] = 0.3;
			m_ambientColor[2] = 0.7;
			m_ambientColor[3] = 1.0;
			m_lightColor[0] = 1.0;
			m_lightColor[1] = 1.0;
			m_lightColor[2] = 1.0;
			m_iblSetting[0] = 1.0;
			m_iblSetting[1] = 1.0;
			m_iblSetting[2] = 0.0;
			m_iblSetting[3] = 0.0;
		}
		bool m_lightOrbit;
		float m_gloss;
		float m_lightIntensity;
		float m_ambientIntensity;
		float m_baseColor[4];
		float m_lightColor[3];
		float m_ambientColor[4];
		float m_iblSetting[4];
		float m_shadowSoft;
		int32_t m_shaderSelection;
	};

	//Cull clockwise triangles for SkyBox CUBE
	uint64_t skyBoxState = 0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
		| BGFX_STATE_CULL_CW
		| BGFX_STATE_MSAA
		;

	struct LightProbe
	{
		enum Enum
		{
			Bolonga,
			Kyoto,

			Count
		};

		void load(const char* _name)
		{
			char filePath[512];

			bx::snprintf(filePath, BX_COUNTOF(filePath), "../resource/env_maps/%s_lod.dds", _name);
			m_tex = loadTexture(filePath, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP);

			bx::snprintf(filePath, BX_COUNTOF(filePath), "../resource/env_maps/%s_lod.dds", _name);
			m_texIrr = loadTexture(filePath, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP);
		}

		void destroy()
		{
			bgfx::destroy(m_tex);
			bgfx::destroy(m_texIrr);
		}

		bgfx::TextureHandle m_tex;
		bgfx::TextureHandle m_texIrr;
	};

class EStarHomework : public entry::AppI
{
public:
	EStarHomework(const char* _name, const char* _description, const char* _url)
		: entry::AppI(_name, _description, _url)
	{
		m_width = 0;
		m_height = 0;
		m_debug = BGFX_DEBUG_NONE;
		m_reset = BGFX_RESET_NONE;
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		m_width  = _width;
		m_height = _height;
		m_debug  = BGFX_DEBUG_TEXT;
		m_reset  = BGFX_RESET_VSYNC;

		bgfx::Init init;
		//init.type = bgfx::RendererType::OpenGLES;
		init.type = args.m_type;
		init.vendorId = args.m_pciId;
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		bgfx::init(init);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view clear state.
		bgfx::setViewClear(RENDER_SHADOW_PASS_ID
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
		);

		bgfx::setViewClear(RENDER_SKY_PASS_ID
			, BGFX_CLEAR_DEPTH | BGFX_CLEAR_COLOR
			, 0x303030ff
			, 1.0f
			, 0
			);

		bgfx::setViewClear(RENDER_SCENE_PASS_ID
			, BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
		);

		s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
		s_texNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
		s_texAORM = bgfx::createUniform("s_AORM", bgfx::UniformType::Sampler);
		s_texCubeLod = bgfx::createUniform("s_texCubeLod", bgfx::UniformType::Sampler);
		s_texCubeIrr = bgfx::createUniform("s_texCubeIrr", bgfx::UniformType::Sampler);
		s_brdfLut = bgfx::createUniform("s_brdfLut", bgfx::UniformType::Sampler);
		s_shadowMap = bgfx::createUniform("s_shadowMap", bgfx::UniformType::Sampler);

		u_lightPos = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4);
		u_lightCol = bgfx::createUniform("u_lightCol", bgfx::UniformType::Vec4);
		u_lightMtx = bgfx::createUniform("u_lightMtx", bgfx::UniformType::Mat4);

		u_viewPos = bgfx::createUniform("u_viewPos", bgfx::UniformType::Vec4);
		u_ambient = bgfx::createUniform("u_ambient", bgfx::UniformType::Vec4);
		u_baseColorBlinn = bgfx::createUniform("u_baseColorBlinn", bgfx::UniformType::Vec4);
		u_glossBlinn = bgfx::createUniform("u_glossBlinn", bgfx::UniformType::Vec4);
		u_iblSetting = bgfx::createUniform("u_iblSetting", bgfx::UniformType::Vec4);
		u_unpackNormal = bgfx::createUniform("u_unpackNormal", bgfx::UniformType::Vec4);
		u_shadowSoft = bgfx::createUniform("u_shadowSoft", bgfx::UniformType::Vec4);

		bgfx::touch(0);

		m_LightPoint = loadProgram("vs_LightPoint", "fs_LightPoint");
		m_BlinnPhong = loadProgram("vs_BlinnPhong", "fs_BlinnPhong");
		m_BlinnPhongTEX = loadProgram("vs_BlinnPhong_TEX", "fs_BlinnPhong_TEX");
		m_Diffuse = loadProgram("vs_Diffuse", "fs_Diffuse");
		m_PBR = loadProgram("vs_PBR", "fs_PBR");
		m_SkyBox = loadProgram("vs_Skybox", "fs_Skybox");
		m_IBL = loadProgram("vs_IBL", "fs_IBL");
		m_ShadowMap = loadProgram("vs_Shadow", "fs_Shadow");
		m_ShadowMesh = loadProgram("vs_ShadowMesh", "fs_ShadowMesh");

		m_orb = meshLoad("../resource/basic_meshes/orb.bin");
		m_stone = meshLoad("../resource/pbr_stone/pbr_stone_mes.bin");
		m_plane = meshLoad("../resource/basic_meshes/platform.bin");
		m_cube = meshLoad("../resource/basic_meshes/cube.bin");
		
		m_textureColor = loadTexture("../resource/pbr_stone/pbr_stone_base_color.dds");
		m_textureNormal = loadTexture("../resource/pbr_stone/pbr_stone_normal.dds");
		m_textureAORM = loadTexture("../resource/pbr_stone/pbr_stone_aorm.dds");
		m_brdfLut = loadTexture("../resource/pbr_stone/ibl_brdf_lut.dds");

		m_lightProbes[LightProbe::Bolonga].load("bolonga");
		m_lightProbes[LightProbe::Kyoto].load("kyoto");
		m_currentLightProbe = LightProbe::Kyoto;

		m_timeOffset = bx::getHPCounter();
		m_shadowMapSize = 512;

		m_shadowMapFB = BGFX_INVALID_HANDLE;

		m_stateShadow[0] = meshStateCreate();
		m_stateShadow[0]->m_state = 0
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_DEPTH_TEST_LESS
			| BGFX_STATE_CULL_CCW
			| BGFX_STATE_MSAA
			;
		m_stateShadow[0]->m_program = m_ShadowMap;
		m_stateShadow[0]->m_viewId = RENDER_SHADOW_PASS_ID;
		m_stateShadow[0]->m_numTextures = 0;

		m_stateShadow[1] = meshStateCreate();
		m_stateShadow[1]->m_state = 0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_DEPTH_TEST_LESS
			| BGFX_STATE_CULL_CCW
			| BGFX_STATE_MSAA
			;
		m_stateShadow[1]->m_viewId = RENDER_SCENE_PASS_ID;
		m_stateShadow[1]->m_program = m_ShadowMesh;
		m_stateShadow[1]->m_numTextures = 1;
		m_stateShadow[1]->m_textures[0].m_flags = UINT32_MAX;
		m_stateShadow[1]->m_textures[0].m_stage = 0;
		m_stateShadow[1]->m_textures[0].m_sampler = s_shadowMap;
		m_stateShadow[1]->m_textures[0].m_texture = BGFX_INVALID_HANDLE;


		imguiCreate();

		//get suitable Camera parameters for stone
		bx::Aabb boundingBox = {};
		for (GroupArray::const_iterator it = m_stone->m_groups.begin(), itEnd = m_stone->m_groups.end(); it != itEnd; ++it)
		{
			if (it == m_stone->m_groups.begin())
			{
				boundingBox = it->m_aabb;
			}
			else
			{
				aabbExpand(boundingBox, it->m_aabb.min);
				aabbExpand(boundingBox, it->m_aabb.max);
			}
		}

		bx::Vec3 m_meshCenter = bx::getCenter(boundingBox);
		float m_meshRadius = bx::length(bx::getExtents(boundingBox));
		cameraCreate(m_meshCenter, m_meshRadius * 5.0f, 0.01f, m_meshRadius * 1000.0f);
		//m_camera.init(m_meshCenter, m_meshRadius * 5.0f, 0.01f, m_meshRadius * 10.0f);
	}

	virtual int shutdown() override
	{
		imguiDestroy();
		meshUnload(m_stone);
		meshUnload(m_orb);
		meshUnload(m_plane);
		meshUnload(m_cube);

		bgfx::destroy(m_BlinnPhong);
		bgfx::destroy(m_Diffuse);
		bgfx::destroy(m_BlinnPhongTEX);
		bgfx::destroy(m_LightPoint);
		bgfx::destroy(m_PBR);
		bgfx::destroy(m_SkyBox);
		bgfx::destroy(m_IBL);
		bgfx::destroy(m_ShadowMap);
		bgfx::destroy(m_ShadowMesh);

		bgfx::destroy(m_textureColor);
		bgfx::destroy(m_textureNormal);
		bgfx::destroy(m_textureAORM);
		bgfx::destroy(m_brdfLut);

		bgfx::destroy(s_texColor);
		bgfx::destroy(s_texNormal);
		bgfx::destroy(s_texAORM);
		bgfx::destroy(s_brdfLut);
		bgfx::destroy(s_shadowMap);

		bgfx::destroy(u_lightPos);
		bgfx::destroy(u_lightCol);
		bgfx::destroy(u_lightMtx);
		bgfx::destroy(u_viewPos);
		bgfx::destroy(u_ambient);
		bgfx::destroy(u_baseColorBlinn);
		bgfx::destroy(u_glossBlinn);
		bgfx::destroy(u_iblSetting);
		bgfx::destroy(u_unpackNormal);
		bgfx::destroy(u_shadowSoft);
		

		for (uint8_t ii = 0; ii < LightProbe::Count; ++ii)
		{
			m_lightProbes[ii].destroy();
		}

		cameraDestroy();

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	bool update() override
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState))
		{
			//GUI CONFIGS
			imguiBeginFrame(m_mouseState.m_mx
				, m_mouseState.m_my
				, (m_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
				, m_mouseState.m_mz
				, uint16_t(m_width)
				, uint16_t(m_height)
			);

			showExampleDialog(this);

			ImGui::SetNextWindowPos(
				ImVec2(m_width - m_width / 5.0f - 10.0f, 10.0f)
				, ImGuiCond_FirstUseEver
			);
			ImGui::SetNextWindowSize(
				ImVec2(m_width / 4.0f, m_height - 20.0f)
				, ImGuiCond_FirstUseEver
			);

			const bool isBlinn = (0 == m_settings.m_shaderSelection);
			const bool isBlinnTex = (1 == m_settings.m_shaderSelection);
			const bool isPBR = (2 == m_settings.m_shaderSelection);
			const bool isIBL = (3 == m_settings.m_shaderSelection);
			const bool isShadowMap = (4 == m_settings.m_shaderSelection);

			ImGui::Begin("Scene Settings"
				, NULL
				, 0
			);
			ImGui::PushItemWidth(180.0f);
			ImGui::Text("Point Light:");
			ImGui::Indent();
			ImGui::Checkbox("Orbit", &m_settings.m_lightOrbit);
			ImGui::Unindent();
			ImGui::Separator();
			ImGui::Text("SkyBox:");
			if (ImGui::BeginTabBar("Cubemap", ImGuiTabBarFlags_None))
			{
				if (ImGui::BeginTabItem("Bolonga"))
				{
					m_currentLightProbe = LightProbe::Bolonga;
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Kyoto"))
				{
					m_currentLightProbe = LightProbe::Kyoto;
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
			ImGui::Separator();
			ImGui::Text("SkyBox Settings:");
			ImGui::SliderFloat("exposure", &m_settings.m_ambientColor[3], -4.0f, 4.0f);
			if (isIBL)
			{
				ImGui::Separator();
				ImGui::Text("IBL Settings:");
				ImGui::SliderFloat("DiffEnv", &m_settings.m_iblSetting[0], 0.0f, 1.0f);
				ImGui::SliderFloat("SpecEnv", &m_settings.m_iblSetting[1], 0.0f, 1.0f);
				ImGui::SliderFloat("DiffDirect", &m_settings.m_iblSetting[2], 0.0f, 1.0f);
				ImGui::SliderFloat("SpecDirect", &m_settings.m_iblSetting[3], 0.0f, 1.0f);
			}
			if (isShadowMap)
			{
				ImGui::Separator();
				ImGui::Text("Shadow Settings:");
				ImGui::SliderFloat("SoftShadow", &m_settings.m_shadowSoft, 0.0f, 1.0f);
			}

			ImGui::End();

			ImGui::SetNextWindowPos(
				ImVec2(10.0f, 260.0f)
				, ImGuiCond_FirstUseEver
			);
			ImGui::SetNextWindowSize(
				ImVec2(m_width / 4.0f, 450.0f)
				, ImGuiCond_FirstUseEver
			);
			ImGui::Begin("Render Settings"
				, NULL
				, 0
			);

			ImGui::Text("Shader:");
			ImGui::RadioButton("BlinnPhong", &m_settings.m_shaderSelection, 0);
			ImGui::RadioButton("BlinnPhongWithNormalAndColor", &m_settings.m_shaderSelection, 1);
			ImGui::RadioButton("PBR", &m_settings.m_shaderSelection, 2);
			ImGui::RadioButton("IBL", &m_settings.m_shaderSelection, 3);
			ImGui::RadioButton("ShadowMap", &m_settings.m_shaderSelection, 4);

			ImGui::Separator();
			ImGui::Text("LightSettings:");
			ImGui::SliderFloat("LightPower", &m_settings.m_lightIntensity, 10.0f, 5000.0f);
			if (!isIBL)
			{
				ImGui::SliderFloat("AmbientPower", &m_settings.m_ambientIntensity, 0.0f, 1.0f);
				ImGui::ColorWheel("AmbientColor", &m_settings.m_ambientColor[0], 0.5f);
			}
			ImGui::ColorWheel("LightColor", &m_settings.m_lightColor[0], 0.5f);
			if (isBlinn || isBlinnTex || isShadowMap)
			{
				ImGui::Separator();
				ImGui::Text("BlinnPhongSettings:");
				ImGui::SliderFloat("BlinnGloss", &m_settings.m_gloss, 5.0f, 50.0f);
				if(!isBlinnTex)
					ImGui::ColorWheel("BaseCol", &m_settings.m_baseColor[0], 0.5f);
			}
			ImGui::End();

			imguiEndFrame();

			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			//bgfx::touch(0);
			if (!bgfx::isValid(m_shadowMapFB))
			{
				bgfx::TextureHandle shadowMapTexture = BGFX_INVALID_HANDLE;
				bgfx::TextureHandle fbtextures[] =
				{
					bgfx::createTexture2D(
						  m_shadowMapSize
						, m_shadowMapSize
						, false
						, 1
						, bgfx::TextureFormat::D16
						, BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LEQUAL
						),
				};

				shadowMapTexture = fbtextures[0];
				m_shadowMapFB = bgfx::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);
				m_stateShadow[1]->m_textures[0].m_texture = shadowMapTexture;
			}

			int64_t now = bx::getHPCounter();
			static int64_t last = now;
			const int64_t frameTime = now - last;
			last = now;
			const double freq = double(bx::getHPFrequency());
			const float deltaTime = float(frameTime / freq);
			float time = (float)((bx::getHPCounter() - m_timeOffset) / double(bx::getHPFrequency()));

			if (m_settings.m_lightOrbit)
			{
				lightPos[0] = 15.0f * bx::sin(time);
				lightPos[2] = 15.0f * bx::cos(time);
			}
			bgfx::setUniform(u_lightPos, lightPos);

			float ambientColor[4] = { m_settings.m_ambientColor[0] * m_settings.m_ambientIntensity, m_settings.m_ambientColor[1] * m_settings.m_ambientIntensity, m_settings.m_ambientColor[2] * m_settings.m_ambientIntensity, m_settings.m_ambientColor[3] };
			bgfx::setUniform(u_ambient, ambientColor);

			bgfx::setUniform(u_baseColorBlinn, m_settings.m_baseColor);

			float glossBlinn[4] = { m_settings.m_gloss, 0.0f, 0.0f, 0.0f };
			bgfx::setUniform(u_glossBlinn, glossBlinn);

			float lightCol[4] = { m_settings.m_lightColor[0] * m_settings.m_lightIntensity, m_settings.m_lightColor[1] * m_settings.m_lightIntensity, m_settings.m_lightColor[2] * m_settings.m_lightIntensity, 1.0f };
			bgfx::setUniform(u_lightCol, lightCol);

			bgfx::setUniform(u_iblSetting, m_settings.m_iblSetting);


			const bool mouseOverGui = ImGui::MouseOverArea();

			
			cameraUpdate(deltaTime, m_mouseState, m_width, m_height, mouseOverGui);
			bx::Vec3 eye = cameraGetPosition();
			float viewPos[4] = { eye.x, eye.y, eye.z, 1.0f };
			bgfx::setUniform(u_viewPos, viewPos);

			// Set view and projection matrix for light
			float lightView[16];
			float lightProj[16];

			const bx::Vec3 at = { 0.0f,  0.0f,   0.0f };
			const bx::Vec3 light = { lightPos[0], lightPos[1], lightPos[2] };
			//const bx::Vec3 light = { bx::cos(time), 1.0f, bx::sin(time)};
			bx::mtxLookAt(lightView, light, at);

			const bgfx::Caps* caps = bgfx::getCaps();
			const float area = 30.0f;
			bx::mtxOrtho(lightProj, -area, area, -area, area, -100.0f, 100.0f, 0.0f, caps->homogeneousDepth);

			bgfx::setViewRect(RENDER_SHADOW_PASS_ID, 0, 0, m_shadowMapSize, m_shadowMapSize);
			bgfx::setViewFrameBuffer(RENDER_SHADOW_PASS_ID, m_shadowMapFB);
			bgfx::setViewTransform(RENDER_SHADOW_PASS_ID, lightView, lightProj);

			// Set view and projection matrix for scene
			float view[16];
			float proj[16];

			cameraGetViewMtx(view);

			bx::mtxProj(proj, 60.0f, float(m_width) / float(m_height), 0.1f, 1000.0f, caps->homogeneousDepth);
			bgfx::setViewTransform(RENDER_SKY_PASS_ID, view, proj);
			bgfx::setViewTransform(RENDER_SCENE_PASS_ID, view, proj);

			bgfx::setViewRect(RENDER_SKY_PASS_ID, 0, 0, uint16_t(m_width), uint16_t(m_height));
			bgfx::setViewRect(RENDER_SCENE_PASS_ID, 0, 0, uint16_t(m_width), uint16_t(m_height));
			
			// Set Matrix
			float mtxShadow[16];
			float lightMtx[16];

			const float sy = caps->originBottomLeft ? 0.5f : -0.5f;
			const float sz = caps->homogeneousDepth ? 0.5f : 1.0f;
			const float tz = caps->homogeneousDepth ? 0.5f : 0.0f;
			const float mtxCrop[16] =
			{
				0.5f, 0.0f, 0.0f, 0.0f,
				0.0f,   sy, 0.0f, 0.0f,
				0.0f, 0.0f, sz,   0.0f,
				0.5f, 0.5f, tz,   1.0f,
			};

			float mtxTmp[16];
			bx::mtxMul(mtxTmp, lightProj, mtxCrop);
			bx::mtxMul(mtxShadow, lightView, mtxTmp);

			float mtxI[16];
			bx::mtxIdentity(mtxI);

			float mtxStone[16];
			bx::mtxIdentity(mtxStone);
			
			float mtxOrb[16];
			bx::mtxScale(mtxOrb, 4.0f);
			mtxOrb[12] = 4.0f;

			float mtxPlane[16];
			bx::mtxScale(mtxPlane, 15);
			mtxPlane[13] = -2.0f;

			float mtxLight[16];
			bx::mtxIdentity(mtxLight);
			bx::mtxTranslate(mtxLight, lightPos[0], lightPos[1], lightPos[2]);
			mtxLight[0] = 0.2f;
			mtxLight[5] = 0.2f;
			mtxLight[10] = 0.2f;
			float unpackNormal[4] = {0.0, 0.0 ,0.0, 0.0};
			if (isShadowMap)
			{
				float soft[4] = { m_settings.m_shadowSoft, 0.0, 0.0, 0.0 };
				bgfx::setUniform(u_shadowSoft, soft);
				bgfx::setTexture(m_stateShadow[1]->m_textures[0].m_stage,
					m_stateShadow[1]->m_textures[0].m_sampler,
					m_stateShadow[1]->m_textures[0].m_texture,
					m_stateShadow[1]->m_textures[0].m_flags
					);
				bx::mtxMul(lightMtx, mtxPlane, mtxShadow);
				bgfx::setUniform(u_lightMtx, lightMtx);
				unpackNormal[0] = 1.0;
				bgfx::setUniform(u_unpackNormal, unpackNormal);
				meshSubmit(m_plane, &m_stateShadow[0], 1, mtxPlane);
				meshSubmit(m_plane, &m_stateShadow[1], 1, mtxPlane);

				bx::mtxMul(lightMtx, mtxStone, mtxShadow);
				bgfx::setUniform(u_lightMtx, lightMtx);
				unpackNormal[0] = 0.0;
				bgfx::setUniform(u_unpackNormal, unpackNormal);
				meshSubmit(m_stone, &m_stateShadow[0], 1, mtxStone);
				meshSubmit(m_stone, &m_stateShadow[1], 1, mtxStone);
			}
			else
				meshSubmit(m_plane, RENDER_SCENE_PASS_ID, m_Diffuse, mtxPlane);



			bgfx::setTexture(0, s_texCubeLod, m_lightProbes[m_currentLightProbe].m_tex);
			meshSubmit(m_cube, RENDER_SKY_PASS_ID, m_SkyBox, mtxI, skyBoxState);
			
			bgfx::setTexture(0, s_texColor, m_textureColor);
			bgfx::setTexture(1, s_texNormal, m_textureNormal);
			bgfx::setTexture(2, s_texAORM, m_textureAORM);
			bgfx::setTexture(3, s_texCubeIrr, m_lightProbes[m_currentLightProbe].m_texIrr);
			bgfx::setTexture(4, s_texCubeLod, m_lightProbes[m_currentLightProbe].m_tex);
			bgfx::setTexture(5, s_brdfLut, m_brdfLut);

			if (isBlinn)
				meshSubmit(m_stone, RENDER_SCENE_PASS_ID, m_BlinnPhong, mtxStone);
			else if(isBlinnTex)
				meshSubmit(m_stone, RENDER_SCENE_PASS_ID, m_BlinnPhongTEX, mtxStone);
			else if(isPBR)
				meshSubmit(m_stone, RENDER_SCENE_PASS_ID, m_PBR, mtxStone);
			else if(isIBL)
				meshSubmit(m_stone, RENDER_SCENE_PASS_ID, m_IBL, mtxStone);

			meshSubmit(m_cube, RENDER_SCENE_PASS_ID, m_LightPoint, mtxLight);
			

			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();

			return true;
		}

		return false;
	}

	entry::MouseState m_mouseState;

	bgfx::UniformHandle s_texColor;
	bgfx::UniformHandle s_texNormal;
	bgfx::UniformHandle s_texAORM;
	bgfx::UniformHandle s_texCubeLod;
	bgfx::UniformHandle s_texCubeIrr;
	bgfx::UniformHandle s_brdfLut;
	bgfx::UniformHandle s_shadowMap;

	bgfx::FrameBufferHandle m_shadowMapFB;

	bgfx::UniformHandle u_lightPos;
	bgfx::UniformHandle u_lightCol;
	bgfx::UniformHandle u_lightMtx;

	bgfx::UniformHandle u_viewPos;
	bgfx::UniformHandle u_ambient;
	bgfx::UniformHandle u_baseColorBlinn;
	bgfx::UniformHandle u_exposure;
	bgfx::UniformHandle u_glossBlinn;
	bgfx::UniformHandle u_iblSetting;
	bgfx::UniformHandle u_unpackNormal;
	bgfx::UniformHandle u_shadowSoft;


	bgfx::TextureHandle m_textureColor;
	bgfx::TextureHandle m_textureNormal;
	bgfx::TextureHandle m_textureAORM;
	bgfx::TextureHandle m_texLod;
	bgfx::TextureHandle m_texIrr;
	bgfx::TextureHandle m_brdfLut;


	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;

	uint16_t m_shadowMapSize;
	
	int64_t m_timeOffset;
	Mesh* m_stone;
	Mesh* m_orb;
	Mesh* m_plane;
	Mesh* m_cube;

	MeshState* m_stateShadow[2];

	bgfx::ProgramHandle m_BlinnPhong;
	bgfx::ProgramHandle m_BlinnPhongTEX;
	bgfx::ProgramHandle m_Diffuse;
	bgfx::ProgramHandle m_LightPoint;
	bgfx::ProgramHandle m_PBR;
	bgfx::ProgramHandle m_SkyBox;
	bgfx::ProgramHandle m_IBL;
	bgfx::ProgramHandle m_ShadowMap;
	bgfx::ProgramHandle m_ShadowMesh;


	LightProbe m_lightProbes[LightProbe::Count];
	LightProbe::Enum m_currentLightProbe;

	Settings m_settings;
	float lightPos[4] = { 0.0f, 15.0f, 0.0f, 1.0f };
};	

} // namespace

int _main_(int _argc, char** _argv)
{
	EStarHomework app("e-star-homework", "", "");
	return entry::runApp(&app, _argc, _argv);
}

