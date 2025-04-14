#include "std_include.hpp"
#include "modules/patches.hpp"
#include "modules/imgui.hpp"

namespace mods::anno1404
{
	void main()
	{
		{	// init filepath var
			char path[MAX_PATH]; GetModuleFileNameA(nullptr, path, MAX_PATH);
			shared::globals::root_path = path; shared::utils::erase_substring(shared::globals::root_path, "Anno4.exe");
		}

		//shared::common::console();
		init_game_addresses();

		module_loader::register_module(std::make_unique<patches>());
		module_loader::register_module(std::make_unique<imgui>());
	}
}
