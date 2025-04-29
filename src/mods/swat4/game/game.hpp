#pragma once

namespace mods::swat4::game
{
#define RENDEV_BASE game::rendev_module
#define ENGINE_BASE game::engine_module

	extern DWORD* GSceneMem;

	extern void init_game_addresses();

	extern DWORD rendev_module;
	extern DWORD engine_module;
}
