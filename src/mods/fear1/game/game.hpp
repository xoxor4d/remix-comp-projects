#pragma once
#include "structs.hpp"

namespace mods::fear1::game
{
	extern void init_game_addresses();

	extern bool is_client_loaded;
	extern HMODULE game_client;
	extern DWORD game_client_addr;
	extern HMODULE game_server;

	extern inline void center_cursor(bool state)
	{
		*reinterpret_cast<int*>(0x56ABB4) = (int) state;
	}
}
