#pragma once

namespace mods::gh3
{
	extern bool g_is_ingame;

	namespace tex_addons
	{
		extern LPDIRECT3DTEXTURE9 white;
	}

	void on_begin_scene_cb();
	void pre_drawindexedprim();
	void post_drawindexedprim();
	void install_signature_patches();
	void main();

	extern bool g_installed_signature_patches;
	extern bool g_install_signature_patches_async;
}
