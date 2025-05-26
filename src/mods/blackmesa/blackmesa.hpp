#pragma once

namespace mods::blackmesa
{
	namespace tex_addons
	{
		extern LPDIRECT3DTEXTURE9 white;
		extern LPDIRECT3DTEXTURE9 black;
	}

	extern int g_current_leaf;
	extern int g_current_area;

	void on_begin_scene_cb();

	void force_cvars();

	void install_signature_patches();
	void main();

	extern bool g_installed_signature_patches;
	extern bool g_install_signature_patches_async;
}
