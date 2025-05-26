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

	int g_current_leaf = -1;
	int g_current_area = -1;

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
		const auto current_leaf = game::get_leaf_from_position(*game::get_current_view_origin());
		//g_player_leaf_update = g_current_leaf != current_leaf;
		g_current_leaf = current_leaf;

		// CM_LeafArea :: get current area the camera is in
		g_current_area = shared::utils::hook::call<int(__cdecl)(int leafnum)>(ENGINE_BASE + 0x185A10)(current_leaf);

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
		game::cvar_uncheat_and_set_int("r_dopixelvisibility", 0);

		game::cvar_uncheat_and_set_int("r_WaterDrawRefraction", 0);
		game::cvar_uncheat_and_set_int("r_WaterDrawReflection", 0);
		game::cvar_uncheat_and_set_int("r_threaded_particles", 0);
		game::cvar_uncheat_and_set_int("r_entityclips", 0);
		game::cvar_uncheat_and_set_int("mat_queue_mode", 0);
		game::cvar_uncheat_and_set_int("mat_fastspecular", 0);
		game::cvar_uncheat_and_set_int("mat_softwarelighting", 0);
		game::cvar_uncheat_and_set_int("mat_parallaxmap", 0);
		game::cvar_uncheat_and_set_int("mat_normalmaps", 0);
		game::cvar_uncheat_and_set_int("r_3dsky", 0);
		game::cvar_uncheat_and_set_int("r_flashlightrender", 0);
		game::cvar_uncheat_and_set_int("r_occlusion", 0);

		game::cvar_uncheat_and_set_int("r_shadows", 0);
		game::cvar_uncheat_and_set_int("mat_vsync", 0);
		game::cvar_uncheat_and_set_int("r_waterforcereflectentities", 0);
		game::cvar_uncheat_and_set_int("mat_motion_blur_enabled", 0);
		//game::cvar_uncheat_and_set_int("r_flashlightdepthtexture", 0); // breaks font rendering?
		//game::cvar_uncheat_and_set_int("r_waterforceexpensive", 0); // breaks font rendering?
		game::cvar_uncheat_and_set_int("mat_supports_d3d9ex", 0);
		game::cvar_uncheat_and_set_int("cl_csm_enabled", 0);

		game::cvar_uncheat_and_set_int("r_staticprop_lod", 0);
		game::cvar_uncheat_and_set_int("r_lod", 0);
		game::cvar_uncheat_and_set_int("r_staticprop_lod", 0);

		game::cvar_uncheat_and_set_int("mat_fullbright", 1);
		game::cvar_uncheat_and_set_int("mat_softwareskin", 1);
		game::cvar_uncheat_and_set_int("mat_phong", 1);
		game::cvar_uncheat_and_set_int("mat_disable_ps_patch", 1);
		game::cvar_uncheat_and_set_int("r_gbuffer_disable", 1);
		game::cvar_uncheat_and_set_int("mat_disable_bloom", 1);
		game::cvar_uncheat_and_set_int("mat_drawflat", 1); // was 0?
		game::cvar_uncheat_and_set_int("mat_fastnobump", 1);
	}

	// check if a boundingbox is within a specified radius around the player
	bool is_aabb_within_distance(const VectorAligned& center, const VectorAligned& half_diagonal, const Vector& player_origin, const float radius)
	{
		const Vector min_bounds = center - half_diagonal;
		const Vector max_bounds = center + half_diagonal;

		auto sq_dist = 0.0f;
		for (auto i = 0; i < 3; ++i)
		{
			if (player_origin[i] < min_bounds[i])
			{
				const auto d = min_bounds[i] - player_origin[i];
				sq_dist += d * d;
			}
			else if (player_origin[i] > max_bounds[i])
			{
				const auto d = player_origin[i] - max_bounds[i];
				sq_dist += d * d;
			}

			// return false if distance exceeds radius sqr
			if (sq_dist > radius * radius) {
				return false;
			}
		}

		return true;
	}

	// Stub before calling 'R_CullNode' in 'R_RecursiveWorldNode'
	// Return 0 to NOT cull the node
	int r_cullnode_wrapper(game::Frustum_t* frustum, game::mnode_t* node, int unkown_flag)
	{
		// "global" nocull distance if area has no overrides
		float nocull_dist = 6000.0f; 

		// if no area override or if cull mode is distance based
		{
			if (is_aabb_within_distance(node->m_vecCenter, node->m_vecHalfDiagonal, *game::get_current_view_origin(), nocull_dist)) {
				return 0;
			}

			// if forcing current area + distance
			{
				if ((int)node->area == g_current_area) {
					return 0;
				}
			}
		}

		// R_CullNode - uses area frustums if avail. and not in a solid - uses player frustum otherwise
		if (!shared::utils::hook::call<bool(__cdecl)(game::Frustum_t*, game::mnode_t*, int)>(ENGINE_BASE + 0x1379C0)(frustum, node, unkown_flag)) { // #OFFS
			return 0;
		}

		// cull node
		return 1;
	}

	//HOOK_RETN_PLACE_DEF(r_cullnode_cull_retn);
	//HOOK_RETN_PLACE_DEF(r_cullnode_skip_retn);
	//__declspec(naked) void r_cullnode_stub()
	//{
	//	__asm
	//	{
	//		pushad;
	//		push	ebx;
	//		call	r_cullnode_wrapper; // return 0 to not jump
	//		add		esp, 4;
	//		test	eax, eax;
	//		jz		SKIP; // jump if eax = 0
	//		popad;

	//		add     esp, 4; // og
	//		jmp		r_cullnode_cull_retn;

	//	SKIP:
	//		popad;
	//		add     esp, 4; // og
	//		jmp		r_cullnode_skip_retn;
	//	}
	//}

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


		// ^ :: while( ... !R_CullNode) - wrapper function to impl. additional culling control (force areas/leafs + use frustum culling when needed)
		//shared::utils::hook(ENGINE_BASE + 0xE68FB, r_cullnode_stub, HOOK_JUMP).install()->quick();
		//HOOK_RETN_PLACE(r_cullnode_cull_retn, ENGINE_BASE + 0xE6A42);
		//HOOK_RETN_PLACE(r_cullnode_skip_retn, ENGINE_BASE + 0xE690B);
		shared::utils::hook(ENGINE_BASE + 0x108133, r_cullnode_wrapper, HOOK_CALL).install()->quick();

		MH_EnableHook(MH_ALL_HOOKS);
	}
}
