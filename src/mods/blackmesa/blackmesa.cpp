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

	/**
	 * Force visibility of a specific node
	 * @param node_index	The node to force vis for
	 * @param player_node	The node the player is currently in
	 */
	void force_node_vis(int node_index, bool hide = false)
	{
		const auto world = game::get_hoststate_worldbrush_data();
		const auto root_node = &world->nodes[0];

		int next_node_index = node_index;
		while (next_node_index >= 0)
		{
			const auto node = &world->nodes[next_node_index];

			if (!hide)
			{
				// node was already set to current visframe, do not continue
				if (node->visframe == game::get_visframecount()) {
					break;
				}

				// force node vis
				node->visframe = game::get_visframecount();
			}
			else
			{
				// nodes already hidden
				if (node->visframe == 0) {
					break;
				}

				node->visframe = 0;
			}

			// we only need to traverse to the root node
			if (node == root_node) {
				break;
			}

			next_node_index = &node->parent[0] - root_node;
		}
	}

	/**
	 * Force visibility of a specific leaf
	 * @param leaf_index   The leaf to force vis for
	 * @param player_node  The node the player is currently in
	 */
	void force_leaf_vis(int leaf_index, bool hide = false)
	{
		const auto world = game::get_hoststate_worldbrush_data();
		auto leaf_node = &world->leafs[leaf_index];
		auto parent_node_index = &leaf_node->parent[0] - &world->nodes[0];

		if (!hide)
		{
			// force leaf vis
			leaf_node->visframe = game::get_visframecount();
		}
		else
		{
			leaf_node->visframe = 0;
		}

		// force nodes
		force_node_vis(parent_node_index, hide);
	}

	// Called once before 'R_RecursiveWorldNode' is getting called for the first time
	void pre_recursive_world_node()
	{
		/*if (*game::get_current_view_id() == VIEW_3DSKY || *game::get_current_view_id() == VIEW_MONITOR) {
			return;
		}*/

		const auto world = game::get_hoststate_worldbrush_data();
		//auto& map_settings = map_settings::get_map_settings();

		float nocull_dist = 6000.0f;
		if (imgui::is_initialized()) {
			nocull_dist = imgui::get()->m_anticull_distance;
		}

		const bool check_area = false;

		// MODE: force all leafs/nodes within a certain dist to the player (+ only in current area modifier)
		if (nocull_dist > 0.0f)
		{
			for (auto i = 0; i < world->numleafs; i++)
			{
				if (auto& l = world->leafs[i];
					!check_area || (int)l.area == g_current_area) // ignore area check if distance mode
				{
					if (is_aabb_within_distance(l.m_vecCenter, l.m_vecHalfDiagonal, *game::get_current_view_origin(), nocull_dist)) {
						force_leaf_vis(i);
					}
				}
			}
		}
	}

	HOOK_RETN_PLACE_DEF(pre_recursive_world_node_retn);
	__declspec(naked) void pre_recursive_world_node_stub()
	{
		__asm
		{
			pushad;
			call	pre_recursive_world_node;
			popad;

			// og
			push    ecx;
			push    dword ptr[eax + 0x50];
			push    edi;
			jmp		pre_recursive_world_node_retn;

		}
	}

	// Stub before calling 'R_CullNode' in 'R_RecursiveWorldNode'
	// Return 0 to NOT cull the node
	int r_cullnode_wrapper(game::Frustum_t* frustum, game::mnode_t* node, int unkown_flag)
	{
		// "global" nocull distance if area has no overrides
		float nocull_dist = 6000.0f;
		if (imgui::is_initialized()) {
			nocull_dist = imgui::get()->m_anticull_distance;
		}

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





		// stub before calling 'R_RecursiveWorldNode' to override node/leaf vis
		shared::utils::hook(ENGINE_BASE + 0x107225, pre_recursive_world_node_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(pre_recursive_world_node_retn, ENGINE_BASE + 0x10722A);

		// ^ :: xnode->visframe == r_visframecount check - check for rectangular cuboids that could match emissive lights
		//shared::utils::hook::nop(ENGINE_BASE + 0xE68E6, 9); // NO
		//shared::utils::hook(ENGINE_BASE + 0xE68E6, while_recursive_world_node_stub, HOOK_JUMP).install()->quick(); // NO
		//HOOK_RETN_PLACE(while_recursive_world_node_og_retn, ENGINE_BASE + 0xE6A42); // NO
		//HOOK_RETN_PLACE(while_recursive_world_node_cullnode_retn, ENGINE_BASE + 0xE68F5); // NO
		//HOOK_RETN_PLACE(while_recursive_world_node_force_retn, ENGINE_BASE + 0xE690B); // NO

		// ^ :: while( ... node->contents < -1 .. ) -> jl to jle
		shared::utils::hook::set<BYTE>(ENGINE_BASE + 0x10811D, 0x7E);

		// ^ :: backface check -> je to jl
		shared::utils::hook::nop(ENGINE_BASE + 0x108211, 2); // okay - draws a little more but not so heavy on perf.

		// ^ :: backface check -> jnz to je
		shared::utils::hook::set<BYTE>(ENGINE_BASE + 0x10821A, 0x74);

		// R_DrawLeaf :: backface check (emissive lamps) plane normal >= -0.00999f
		shared::utils::hook::nop(ENGINE_BASE + 0x1078F8, 2);

		// ^ :: while( ... !R_CullNode) - wrapper function to impl. additional culling control (force areas/leafs + use frustum culling when needed)
		//shared::utils::hook(ENGINE_BASE + 0xE68FB, r_cullnode_stub, HOOK_JUMP).install()->quick();
		//HOOK_RETN_PLACE(r_cullnode_cull_retn, ENGINE_BASE + 0xE6A42);
		//HOOK_RETN_PLACE(r_cullnode_skip_retn, ENGINE_BASE + 0xE690B);
		shared::utils::hook(ENGINE_BASE + 0x108133, r_cullnode_wrapper, HOOK_CALL).install()->quick();

		MH_EnableHook(MH_ALL_HOOKS);
	}
}
