#pragma once

namespace mods::wolfenstein2k9
{
	void on_begin_scene();
	void pre_drawindexedprim();
	void post_drawindexedprim();
	void install_signature_patches();
	void main();

	extern bool g_installed_signature_patches;
	extern bool g_install_signature_patches_async;
}
