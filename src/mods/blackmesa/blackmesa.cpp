#include "std_include.hpp"

#include <windowsx.h>

#include "modules/imgui.hpp"
#include "modules/renderer.hpp"
#include "shared/common/flags.hpp"
#include "shared/common/remix_api.hpp"
#include "shared/common/remix_vars.hpp"

// -setup

namespace mods::blackmesa
{
	namespace tex_addons
	{
		LPDIRECT3DTEXTURE9 white;
		LPDIRECT3DTEXTURE9 black;
	}

	void init_texture_addons([[maybe_unused]] bool release)
	{
		const auto dev = shared::globals::d3d_device;
		D3DXCreateTextureFromFileA(dev, "rtx_comp\\textures\\white.dds", &tex_addons::white);
		D3DXCreateTextureFromFileA(dev, "rtx_comp\\textures\\black.dds", &tex_addons::black);
	}

	void on_begin_scene_cb()
	{
		
		if (static bool initiated_vars_once = false; !initiated_vars_once)
		{
			init_texture_addons(false);
			initiated_vars_once = true;
		}
	}

	void on_renderview()
	{
		auto enginerender = game::get_engine_renderer();
		const auto dev = game::get_d3d_device();

		// resets
		//renderer::get()->m_unbake_transforms_on_next_static_prop = false;
		//renderer::get()->m_unbake_transforms_p2w_transform = game::IDENTITY;

		// setup main camera (currently req. for nocull markers)
		{
			float colView[4][4] = {};
			shared::utils::transpose_float4x4(enginerender->m_matrixView.m[0], colView[0]);

			float colProj[4][4] = {};
			shared::utils::transpose_float4x4(enginerender->m_matrixProjection.m[0], colProj[0]);

			dev->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
			dev->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(colView));
			dev->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(colProj));
		}

		{
			// set a default material with diffuse set to a warm white
			// so that add light to texture works and does not require rtx.effectLightPlasmaBall (animated)
			D3DMATERIAL9 dmat = {};
			dmat.Diffuse.r = 1.0f;
			dmat.Diffuse.g = 0.8f;
			dmat.Diffuse.b = 0.8f;
			dev->SetMaterial(&dmat);
		}

		//if (!game::is_paused()) {
		//	main_module::framecount++; // used for debug anim
		//}

		// ----
		// ----

		//choreo_events::on_client_frame();
		//remix_vars::on_client_frame();
		//remix_lights::on_client_frame();

		force_cvars();


		// TODO - find better spot to call this
		//map_settings::spawn_markers_once();
		//model_render::draw_nocull_markers();

		// CM_PointLeafnum :: get current leaf
		//const auto current_leaf = game::get_leaf_from_position(*game::get_current_view_origin());
		//g_player_leaf_update = g_current_leaf != current_leaf;
		//g_current_leaf = current_leaf;

		// CM_LeafArea :: get current area the camera is in
		//g_current_area = utils::hook::call<int(__cdecl)(int leafnum)>(ENGINE_BASE + USE_OFFSET(0x15ACE0, 0x159470))(current_leaf); // 0125

		//remix_api::get()->on_renderview();
	}

	HOOK_RETN_PLACE_DEF(cviewrenderer_renderview_retn);
	__declspec(naked) void cviewrenderer_renderview_stub()
	{
		__asm
		{
			pushad;
			call	on_renderview;
			popad;

			push    0x178; // og
			jmp		cviewrenderer_renderview_retn;
		}
	}

	void force_cvars()
	{
		//game::cvar_uncheat_and_set_int("r_dopixelvisibility", 0);
	}

	void install_signature_patches()
	{
		std::uint32_t install_counter = 0u;
		std::uint32_t total_patch_amount = 0u;

		// ------------------
		std::cout << "[SIG] Installed " << std::to_string(install_counter) << "/" << std::to_string(total_patch_amount) << " signature patches.\n";
	}

	void main()
	{
		game::init_game_addresses();

		// init remix api
		shared::common::remix_api::initialize(on_begin_scene_cb, nullptr, nullptr, false);

		// init remix variable system
		//shared::common::remix_vars::initialize(game::g_is_paused, &shared::globals::frame_time_ms);

		shared::common::loader::module_loader::register_module(std::make_unique<imgui>());
		shared::common::loader::module_loader::register_module(std::make_unique<mods::blackmesa::renderer>());

		// CViewRender::RenderView :: "start" of current frame (after CViewRender::DrawMonitors)
		shared::utils::hook(CLIENT_BASE + 0x1FE0F3, cviewrenderer_renderview_stub).install()->quick();
		HOOK_RETN_PLACE(cviewrenderer_renderview_retn, CLIENT_BASE + 0x1FE0F8);

		MH_EnableHook(MH_ALL_HOOKS);
	}
}
