#include "std_include.hpp"
#include "modules/patches.hpp"
#include "modules/imgui.hpp"

namespace mods::bioshock1
{
	void main()
	{
		{	// init filepath var
			char path[MAX_PATH]; GetModuleFileNameA(nullptr, path, MAX_PATH);
			shared::globals::root_path = path; shared::utils::erase_substring(shared::globals::root_path, "Bioshock.exe");
		}

		//shared::common::console();
		game::init_game_addresses();

		module_loader::register_module(std::make_unique<imgui>());
		module_loader::register_module(std::make_unique<patches>());
	}
}
