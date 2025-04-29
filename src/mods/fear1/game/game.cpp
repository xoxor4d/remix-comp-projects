#include "std_include.hpp"

namespace mods::fear1::game
{
	bool is_client_loaded = false;
	HMODULE game_client = nullptr;
	DWORD game_client_addr = 0u;
	HMODULE game_server = nullptr;
}
