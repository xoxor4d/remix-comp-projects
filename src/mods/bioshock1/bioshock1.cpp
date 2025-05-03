#include "std_include.hpp"
#include "modules/patches.hpp"
#include "modules/imgui.hpp"
#include "shared/common/dinput_hook.hpp"

namespace mods::bioshock1
{
	// called from dllmain (main thread)
	void init_early_hooks()
	{
		shared::common::dinput::init();
	}

	// called from async thread after game window was found
	void main()
	{
		{	// init filepath var
			char path[MAX_PATH]; GetModuleFileNameA(nullptr, path, MAX_PATH);
			shared::globals::root_path = path; shared::utils::erase_substring(shared::globals::root_path, "Bioshock.exe");
		}

		game::init_game_addresses();
		module_loader::register_module(std::make_unique<imgui>());
		module_loader::register_module(std::make_unique<patches>());
	}
}
