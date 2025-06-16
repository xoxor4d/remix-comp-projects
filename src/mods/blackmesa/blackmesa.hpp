#pragma once
#include "modules/map_settings.hpp"

namespace mods::blackmesa
{
	namespace tex_addons
	{
		extern LPDIRECT3DTEXTURE9 white;
		extern LPDIRECT3DTEXTURE9 black;
		extern LPDIRECT3DTEXTURE9 water_temp;
		extern LPDIRECT3DTEXTURE9 sky;
		extern LPDIRECT3DTEXTURE9 decal;
	}

	extern int g_current_leaf;
	extern int g_current_area;
	extern map_settings::area_overrides_s* g_player_current_area_override;

	extern void on_begin_scene_cb();

	extern void trigger_vis_logic();
	extern void force_cvars();
	extern void cross_handle_map_and_game_settings();

	extern void install_signature_patches();
	extern void main();

	extern bool g_installed_signature_patches;
	extern bool g_install_signature_patches_async;
}
