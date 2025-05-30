#include "std_include.hpp"

#include <windowsx.h>

#include "modules/game_settings.hpp"
#include "modules/imgui.hpp"
#include "modules/interfaces.hpp"
#include "modules/map_settings.hpp"
#include "modules/renderer.hpp"
#include "shared/common/flags.hpp"
#include "shared/common/remix_api.hpp"
#include "shared/common/remix_vars.hpp"

// -setup

// -dx9 -dxlevel 95 -oldgameui -window -w 1920 -h 1080

namespace mods::blackmesa
{
	namespace tex_addons
	{
		LPDIRECT3DTEXTURE9 white;
		LPDIRECT3DTEXTURE9 black;
		LPDIRECT3DTEXTURE9 water_temp;
	}

	int g_current_leaf = -1;
	int g_current_area = -1;
	bool g_player_leaf_update = false;
	int g_current_area_all_views = -1; // updated on each view-scene (eg. monitor + main)

	// contains overrides for the current area, nullptr if no overrides exist
	map_settings::area_overrides_s* g_player_current_area_override = nullptr;

	void init_texture_addons([[maybe_unused]] bool release)
	{
		const auto dev = shared::globals::d3d_device;
		D3DXCreateTextureFromFileA(dev, "rtx_comp\\textures\\white.dds", &tex_addons::white);
		D3DXCreateTextureFromFileA(dev, "rtx_comp\\textures\\black.dds", &tex_addons::black);
		D3DXCreateTextureFromFileA(dev, "rtx_comp\\textures\\water_temp.png", &tex_addons::water_temp);
	}

	void on_begin_scene_cb()
	{
		
		if (static bool initiated_vars_once = false; !initiated_vars_once)
		{
			init_texture_addons(false);
			initiated_vars_once = true;
		}
	}

	/**
	 * Called from 'CViewRender::DrawOneMonitor' before calling 'CViewRender::RenderView'
	 * - in use when game is rendering a scene to a monitor (different rendertarget)
	 */
	void cviewrenderer_drawonemonitor_hk()
	{
		auto enginerender = game::get_engine_renderer();
		const auto dev = game::get_d3d_device();

		float colView[4][4] = {};
		shared::utils::transpose_float4x4(enginerender->m_matrixView.m[0], colView[0]);

		float colProj[4][4] = {};
		shared::utils::transpose_float4x4(enginerender->m_matrixProjection.m[0], colProj[0]);

		dev->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
		dev->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(colView));
		dev->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(colProj));
	}

	HOOK_RETN_PLACE_DEF(cviewrenderer_drawonemonitor_retn);
	__declspec(naked) void cviewrenderer_drawonemonitor_stub()
	{
		__asm
		{
			pushad;
			call	cviewrenderer_drawonemonitor_hk;
			popad;

			// og
			lea     eax, [ebp - 0x1AC];
			jmp		cviewrenderer_drawonemonitor_retn;
		}
	}

#if DEBUG
	//std::unordered_set<int> dumped_classes;
	std::unordered_map<int, std::string> dumped_classes;
#endif

	// iterate all ents 
	void iterate_entities()
	{
		if (interfaces::is_initialized())
		{
			const auto intf = interfaces::get();
			const auto max_ent = intf->m_entity_list->get_max_entity();

			for (auto i = 0; i < max_ent; i++)
			{
				if (const auto	entity = reinterpret_cast<sdk::c_base_player*>(intf->m_entity_list->get_client_entity(i));
					entity)
				{
					if (const auto* m_classes = entity->client_class();
						m_classes)
					{
#if DEBUG
						if (!dumped_classes.contains(m_classes->class_id))
						{
							dumped_classes[m_classes->class_id] = m_classes->network_name;
							std::cout << "[CLASSID] " << m_classes->network_name << " = " << m_classes->class_id << ",\n";
						}
#endif
						switch (m_classes->class_id)
						{
						default:
							continue;

						case sdk::CBlackMesaPlayer:
						{
							sdk::player_info_t info;
							if (!intf->m_engine->get_player_info(i, &info)) {
								continue;
							}

							if (const auto is_player = i == intf->m_engine->get_local_player();
								is_player)
							{
								const auto& flashlight_enabled = entity->read<bool>(0x13C5);
								const auto& eyepos = entity->get_eye_pos(); //entity->read<Vector>(0x1110);
								const auto& QAngle = entity->read<Vector>(0x1710);

								const auto gs = game_settings::get();
								const auto offs = gs->flashlight_offset_player.get_as<Vector*>();

								shared::common::remix_api::flashlight_def_s def = {};
								utils::vector::AngleVectors(QAngle, &def.fwd, &def.rt, &def.up);

								def.pos = eyepos + (def.fwd * offs->x) + (def.rt * offs->z) + (def.up * offs->y);
								def.radius = gs->flashlight_radius.get_as<float>();
								def.angle = gs->flashlight_angle.get_as<float>();
								def.softness = gs->flashlight_softness.get_as<float>();
								def.expo = gs->flashlight_expo.get_as<float>();
								def.intensity = gs->flashlight_intensity.get_as<float>();

								shared::common::remix_api::get().flashlight_create_or_update(info.name, def, flashlight_enabled, true);

								// inner flashlight
								def.angle = gs->flashlight_angle_inner.get_as<float>();
								def.softness = gs->flashlight_softness_inner.get_as<float>();
								def.intensity = gs->flashlight_intensity_inner.get_as<float>();
								shared::common::remix_api::get().flashlight_create_or_update(shared::utils::va("%s_inner", info.name), def, flashlight_enabled, true);
							}

							else // bots .. TODO? - is this needed?
							{
								//const auto& m_fEffects = entity->read<int>(0xE0);
								//const bool flashlight_enabled = m_fEffects & 4;

								//const auto& eyepos = entity->get_eye_pos();
								//const auto& angles = entity->read<Vector>(0x196C); // m_angEyeAngles[0] - DT_CSPlayer 

								//Vector fwd, rt, up;
								//utils::vector::AngleVectors(angles, &fwd, &rt, &up);

								//shared::common::remix_api::get().flashlight_create_or_update(info.name, eyepos, fwd, rt, up, flashlight_enabled);
							}
							break;
						}
						}
					}
				}
			}
		}
	}

	void on_renderview()
	{
		auto enginerender = game::get_engine_renderer();
		const auto dev = game::get_d3d_device();

		// resets
		renderer::get()->m_drew_model = false; // helper for nocull markers
		renderer::get()->m_unbake_transforms_on_next_static_prop = false;
		renderer::get()->m_unbake_transforms_p2w_transform = shared::globals::IDENTITY;

		// setup main camera (currently req. for nocull markers)
		{
			float colView[4][4] = {};
			shared::utils::transpose_float4x4(enginerender->m_matrixView.m[0], colView[0]);

			float colProj[4][4] = {};
			shared::utils::transpose_float4x4(enginerender->m_matrixProjection.m[0], colProj[0]);

			//dev->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
			//dev->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(colView));
			//dev->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(colProj));
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

		/*auto org = *game::get_current_view_origin();
		Vector mmin = {};
		mmin.x = org.x - 30.0f;
		mmin.y = org.y - 30.0f;
		mmin.z = org.z - 30.0f;

		Vector mmax = {};
		mmax.x = org.x + 30.0f;
		mmax.y = org.y + 30.0f;
		mmax.z = org.z + 30.0f;

		shared::common::remix_api::get().debug_draw_box(mmin, mmax, 4.0f, shared::common::remix_api::DEBUG_REMIX_LINE_COLOR::GREEN);*/

		shared::common::remix_vars::on_client_frame();
		force_cvars();

		// TODO - find better spot to call this
		//map_settings::spawn_markers_once();
		//renderer::draw_nocull_markers();

		// CM_PointLeafnum :: get current leaf
		const auto current_leaf = game::get_leaf_from_position(*game::get_current_view_origin());
		g_player_leaf_update = g_current_leaf != current_leaf;
		g_current_leaf = current_leaf;

		// CM_LeafArea :: get current area the camera is in
		g_current_area = shared::utils::hook::call<int(__cdecl)(int leafnum)>(ENGINE_BASE + 0x185A10)(current_leaf);

		iterate_entities();
		shared::common::remix_api::get().flashlight_frame();
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

	/**
	 * Called from CModelLoader::Map_LoadModel
	 * @param map_name  Name of loading map
	 */
	void on_map_load_hk(const char* map_name)
	{
		shared::common::remix_vars::on_map_load(map_name);
		map_settings::on_map_load(map_name);
	}

	HOOK_RETN_PLACE_DEF(on_map_load_stub_retn);
	__declspec(naked) void on_map_load_stub()
	{
		__asm
		{
			lea     eax, [ebx + 0x160];

			pushad;
			push    eax;
			call	on_map_load_hk;
			add		esp, 4;
			popad;

			// og
			lea     eax, [ebx + 0x160];
			jmp		on_map_load_stub_retn;

		}
	}

	/**
	 * Called from Host_Disconnect
	 * on: disconnect, restart, killserver, stopdemo ...
	 */
	void on_host_disconnect_hk()
	{
		trigger_vis_logic();

		// ----------

		map_settings::on_map_unload();

		// reload rtx.conf
		shared::common::remix_vars::xo_vars_parse_options_fn();
	}

	HOOK_RETN_PLACE_DEF(on_host_disconnect_retn);
	__declspec(naked) void on_host_disconnect_stub()
	{
		__asm
		{
			pushad;
			call	on_host_disconnect_hk;
			popad;

			// og
			mov     edx, 5;
			jmp		on_host_disconnect_retn;
		}
	}

	/**
	 * Called from Host_Changelevel
	 * Host_Disconnect is not called when this triggers
	 */
	void on_host_change_level_hk()
	{
		on_host_disconnect_hk();
	}

	HOOK_RETN_PLACE_DEF(on_host_change_level_retn);
	__declspec(naked) void on_host_change_level_stub()
	{
		__asm
		{
			pushad;
			call	on_host_change_level_hk;
			popad;

			// og
			push    0x103;
			jmp		on_host_change_level_retn;
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
		game::cvar_uncheat_and_set_int("r_3dsky", game_settings::get()->enable_3d_sky.get_as<bool>());
		game::cvar_uncheat_and_set_int("r_flashlightrender", 0);
		game::cvar_uncheat_and_set_int("r_occlusion", 0);

		game::cvar_uncheat_and_set_int("r_shadows", 0);
		game::cvar_uncheat_and_set_int("mat_vsync", 0);
		game::cvar_uncheat_and_set_int("r_waterforcereflectentities", 0);
		game::cvar_uncheat_and_set_int("mat_motion_blur_enabled", 0);
		//game::cvar_uncheat_and_set_int("r_flashlightdepthtexture", 0); // breaks font rendering?
		//game::cvar_uncheat_and_set_int("r_waterforceexpensive", 0); // breaks font rendering?
		game::cvar_uncheat_and_set_int("mat_supports_d3d9ex", 0);
		

		if (game_settings::get()->lod_forcing.get_as<bool>())
		{
			game::cvar_uncheat_and_set_int("r_staticprop_lod", 0);
			game::cvar_uncheat_and_set_int("r_lod", 0);
			game::cvar_uncheat_and_set_int("r_staticprop_lod", 0);
		}

		if (game_settings::get()->force_graphic_settings.get_as<bool>())
		{
			game::cvar_uncheat_and_set_int("cl_csm_enabled", 0);
			game::cvar_uncheat_and_set_int("np_gr_quality", 0);
			game::cvar_uncheat_and_set_int("nr_lights_quality", 0);
			game::cvar_uncheat_and_set_int("nr_shadow_quality", 0);
			game::cvar_uncheat_and_set_int("mat_geiger_noise_enable", 0);
			game::cvar_uncheat_and_set_int("mat_chromatic_damage_enable", 0);
			game::cvar_uncheat_and_set_int("mat_colorcorrection", 1); // 0 breaks font rendering
			game::cvar_uncheat_and_set_int("r_shadowrendertotexture", 0);
		}

		// mat_hdr_level
		//game::cvar_uncheat_and_set_float("mat_hdr_level", 0);
		//game::cvar_uncheat_and_set_int("mat_use_compressed_hdr_textures", 0);

		game::cvar_uncheat_and_set_int("con_enable", 1);
		game::cvar_uncheat_and_set_int("mat_fullbright", 1);
		game::cvar_uncheat_and_set_int("mat_softwareskin", 1); // .... studio + 0x10AC7 .. nop 2 to disable softwareskin
		game::cvar_uncheat_and_set_int("mat_phong", 1); 
		game::cvar_uncheat_and_set_int("mat_disable_ps_patch", 1);
		game::cvar_uncheat_and_set_int("r_gbuffer_disable", 1);
		game::cvar_uncheat_and_set_int("mat_disable_bloom", 1);
		game::cvar_uncheat_and_set_int("mat_drawflat", 1);
		game::cvar_uncheat_and_set_int("mat_fastnobump", 1);
	}

	// logic after loading either map or game settings
	void cross_handle_map_and_game_settings()
	{
		if (shared::common::remix_api::is_initialized())
		{
			// rtx.skyAutoDetect
			const auto is_3d_sky_enabled = game_settings::get()->enable_3d_sky.get_as<bool>();

			shared::common::remix_vars::set_option(
				shared::common::remix_vars::get_option("rtx.skyAutoDetect"), 
				shared::common::remix_vars::string_to_option_value(shared::common::remix_vars::OPTION_TYPE_FLOAT, is_3d_sky_enabled ? "1" : "0"));

			shared::common::remix_vars::set_option(
				shared::common::remix_vars::get_option("rtx.skyReprojectToMainCameraSpace"),
				shared::common::remix_vars::string_to_option_value(shared::common::remix_vars::OPTION_TYPE_BOOL, is_3d_sky_enabled ? "True" : "False"));
		}
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

	// Trigger leaf/node forcing logic and updates 'g_player_current_area_override' when 'pre_recursive_world_node()' gets called
	void trigger_vis_logic()
	{
		//g_player_current_leaf = -1;
		g_player_leaf_update = true;
		g_player_current_area_override = nullptr;
	}

	// Called once before 'R_RecursiveWorldNode' is getting called for the first time
	void pre_recursive_world_node()
	{
		if (*game::get_current_view_id() == game::VIEW_3DSKY || *game::get_current_view_id() == game::VIEW_MONITOR) {
			return;
		}

		const auto world = game::get_hoststate_worldbrush_data();
		//auto& map_settings = map_settings::get_map_settings();

		float nocull_dist = game_settings::get()->default_nocull_distance.get_as<float>();

		auto& ms = map_settings::get_map_settings();
		if (!ms.area_settings.empty())
		{
			g_player_current_area_override = nullptr;
			if (const auto& t = ms.area_settings.find(g_current_area); t != ms.area_settings.end()) {
				g_player_current_area_override = &t->second; // cache
			}
		}

		if (g_player_current_area_override) {
			nocull_dist = g_player_current_area_override->nocull_distance;
		}
		
		//const bool check_area = false;

		// MODE: force all leafs/nodes within a certain dist to the player (+ only in current area modifier)
		if (nocull_dist > 0.0f)
		{
			for (auto i = 0; i < world->numleafs; i++)
			{
				//if (auto& l = world->leafs[i];
				//	!check_area || (int)l.area == g_current_area) // ignore area check if distance mode
				auto& l = world->leafs[i];
				{
					if (is_aabb_within_distance(l.m_vecCenter, l.m_vecHalfDiagonal, *game::get_current_view_origin(), nocull_dist)) {
						force_leaf_vis(i);
					}
				}
			}
		}

		// update visibility of nocull markers
		if (g_player_leaf_update)
		{
			for (auto& m : ms.map_markers)
			{
				// ignore normal markers
				if (m.areas.empty()) {
					continue;
				}

				// hide marker
				m.is_hidden = true;

				// check if player is in specified area & not in specified leaf
				if (m.areas.contains(g_current_area)) {
					m.is_hidden = false; // show marker
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
		const auto view_id = game::get_current_view_id();
		const bool is_monitor = *view_id == game::VIEW_MONITOR || (*view_id == game::VIEW_ILLEGAL && game::saved_view_id == game::VIEW_MONITOR);

		// "global" nocull distance if area has no overrides
		float nocull_dist = game_settings::get()->default_nocull_distance.get_as<float>();

		// shortcut for monitor rendering
		if (is_monitor)
		{
			// R_CullNode - uses area frustums if avail. and not in a solid - uses player frustum otherwise
			if (!shared::utils::hook::call<bool(__cdecl)(game::Frustum_t*, game::mnode_t*, int)>(ENGINE_BASE + 0x1379C0)(frustum, node, unkown_flag)) { // #OFFS
				return 0;
			}

			// this should work?
			if (is_aabb_within_distance(node->m_vecCenter, node->m_vecHalfDiagonal, *game::get_current_view_origin(), nocull_dist)) {
				return 0;
			}

			return 1;
		}


		if (g_player_current_area_override)
		{
			// nocull distance if area has override
			nocull_dist = g_player_current_area_override->nocull_distance;
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

	HOOK_RETN_PLACE_DEF(save_viewid_retn);
	__declspec(naked) void save_viewid_stub()
	{
		__asm
		{
			mov     ecx, ebx; // og
			mov		[ebp - 8], esi; // og - esi = g_CurrentViewID
			mov		game::saved_view_id, esi; // save to our global
			jmp		save_viewid_retn;
		}
	}

	void install_signature_patches()
	{
		std::uint32_t install_counter = 0u;
		std::uint32_t total_patch_amount = 0u;

		// ------------------
		std::cout << "[SIG] Installed " << std::to_string(install_counter) << "/" << std::to_string(total_patch_amount) << " signature patches.\n";
	}

#if DEBUG
	game::ConCommand xo_dump_classes{};
	void xo_dump_classes_fn()
	{
		for (const auto& c : dumped_classes) {
			std::cout << "[CLASSID] " << c.second << " = " << std::to_string(c.first) << ",\n";
		}
	}
#endif

	void main()
	{
		game::init_game_addresses();

		// init remix api
		shared::common::remix_api::initialize(on_begin_scene_cb, nullptr, nullptr, false);

		// init remix variable system
		shared::common::remix_vars::initialize(game::is_paused, &game::get_global_vars()->frametime);

		shared::common::loader::module_loader::register_module(std::make_unique<interfaces>());
		shared::common::loader::module_loader::register_module(std::make_unique<game_settings>());
		shared::common::loader::module_loader::register_module(std::make_unique<map_settings>());
		shared::common::loader::module_loader::register_module(std::make_unique<imgui>());
		shared::common::loader::module_loader::register_module(std::make_unique<renderer>());


		// CViewRender::DrawOneMonitor
		shared::utils::hook::nop(CLIENT_BASE + 0x1F7E26, 6);
		shared::utils::hook(CLIENT_BASE + 0x1F7E26, cviewrenderer_drawonemonitor_stub).install()->quick();
		HOOK_RETN_PLACE(cviewrenderer_drawonemonitor_retn, CLIENT_BASE + 0x1F7E2C);

		// CViewRender::RenderView :: "start" of current frame (after CViewRender::DrawMonitors)
		shared::utils::hook(CLIENT_BASE + 0x1FE0F3, cviewrenderer_renderview_stub).install()->quick();
		HOOK_RETN_PLACE(cviewrenderer_renderview_retn, CLIENT_BASE + 0x1FE0F8);


		// stub before calling 'R_RecursiveWorldNode' to override node/leaf vis
		shared::utils::hook(ENGINE_BASE + 0x107225, pre_recursive_world_node_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(pre_recursive_world_node_retn, ENGINE_BASE + 0x10722A);

		// ^ :: while( ... node->contents < -1 .. ) -> jl to jle
		shared::utils::hook::set<BYTE>(ENGINE_BASE + 0x10811D, 0x7E);

		// ^ :: backface check -> je to jl
		shared::utils::hook::nop(ENGINE_BASE + 0x108211, 2); // okay - draws a little more but not so heavy on perf.

		// ^ :: backface check -> jnz to je
		shared::utils::hook::set<BYTE>(ENGINE_BASE + 0x10821A, 0x74);

		// R_DrawLeaf :: backface check (emissive lamps) plane normal >= -0.00999f
		shared::utils::hook::nop(ENGINE_BASE + 0x1078F8, 2);

		// ^ :: while( ... !R_CullNode) - wrapper function to impl. additional culling control (force areas/leafs + use frustum culling when needed)
		shared::utils::hook(ENGINE_BASE + 0x108133, r_cullnode_wrapper, HOOK_CALL).install()->quick();

		// disable displacement culling
		shared::utils::hook::nop(ENGINE_BASE + 0xED2FB, 2);

		// CBrushBatchRender::DrawOpaqueBrushModel :: :: backface check - nop 'if ( bShadowDepth )' to disable culling
		//shared::utils::hook::nop(ENGINE_BASE + 0x7156E, 2); // 0125
		shared::utils::hook::conditional_jump_to_jmp(ENGINE_BASE + 0x104796);

		// CClientLeafSystem::CollateRenderablesInLeaf :: skip culling checks
		shared::utils::hook::nop(CLIENT_BASE + 0xFDCBF, 6);

		// CSimpleWorldView::Setup :: nop 'DoesViewPlaneIntersectWater' check
		shared::utils::hook::nop(CLIENT_BASE + 0x1FF55D, 2);

		// ^ next instruction :: OR m_DrawFlags with 0x60 instead of 0x30
		shared::utils::hook::set<BYTE>(CLIENT_BASE + 0x1FF55F + 6, 0x60);

		// CBaseWorldView::DrawSetup :: save 'g_CurrentViewID' 
		shared::utils::hook(CLIENT_BASE + 0x1F8AD6, save_viewid_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(save_viewid_retn, CLIENT_BASE + 0x1F8ADB);


		// PixelVisibility_FractionVisible :: disable visiblity checks for glowsprites and other fx?
		shared::utils::hook::nop(CLIENT_BASE + 0xD4D88, 2);
		shared::utils::hook::conditional_jump_to_jmp(CLIENT_BASE + 0xD4DA6);


		// --

		// CModelLoader::Map_LoadModel :: called on map load
		shared::utils::hook::nop(ENGINE_BASE + 0x124B44, 6);
		shared::utils::hook(ENGINE_BASE + 0x124B44, on_map_load_stub).install()->quick();
		HOOK_RETN_PLACE(on_map_load_stub_retn, ENGINE_BASE + 0x124B4A);

		// called on map unload
		shared::utils::hook(ENGINE_BASE + 0x1CB3B1, on_host_disconnect_stub).install()->quick();
		HOOK_RETN_PLACE(on_host_disconnect_retn, ENGINE_BASE + 0x1CB3B6);

		shared::utils::hook(ENGINE_BASE + 0x1B9F78, on_host_change_level_stub).install()->quick();
		HOOK_RETN_PLACE(on_host_change_level_retn, ENGINE_BASE + 0x1B9F7D);

		MH_EnableHook(MH_ALL_HOOKS);

#if DEBUG
		game::con_add_command(&xo_dump_classes, "xo_dump_classes", xo_dump_classes_fn, "Dump collected classes to console");
#endif
	}
}
