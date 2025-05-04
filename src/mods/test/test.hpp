#pragma once

namespace mods::test
{
	void install_signature_patches();
	void main();

	extern bool g_installed_signature_patches;
	extern bool g_install_signature_patches_async;
}
