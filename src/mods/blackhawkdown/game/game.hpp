#pragma once

namespace mods::blackhawkdown::game
{
	extern int* g_free_mouse;
	extern bool* g_is_paused;

	inline bool is_nightvision_on()
	{
		return *reinterpret_cast<bool*>(0x96C4BC);
	}

	inline int get_camera_view_mode()
	{
		return *reinterpret_cast<int*>(0x7F2DD0);
	}

	inline const char* get_map_name()
	{
		return reinterpret_cast<const char*>(0x9F1F62);
	}

	extern void init_game_addresses();
}
