#include "std_include.hpp"

namespace mods::ue2fixes::game
{
	DWORD* GSceneMem = nullptr;

	// init any adresses here
	void init_game_addresses()
	{
		GSceneMem = reinterpret_cast<DWORD*>(ENGINE_BASE + 0x77D7F8);
	}

	DWORD rendev_module = 0u;
	DWORD engine_module = 0u;
}
