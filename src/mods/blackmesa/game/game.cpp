#include "std_include.hpp"

namespace mods::blackhawkdown::game
{
	int* g_free_mouse = reinterpret_cast<int*>(0xA35594);
	bool* g_is_paused = reinterpret_cast<bool*>(0x7C72BC);

	// init any adresses here
	void init_game_addresses()
	{
	}
}
