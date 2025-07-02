#include <std_include.hpp>
#include "c_engine_client.hpp"
#include <shared/structs.hpp>

namespace sdk
{
	int engine_client::get_local_player()
	{
		using original_fn = int(__thiscall*)(engine_client*);
		return (*(original_fn * *)this)[12](this);
	}
	
	int engine_client::get_player_for_user_id(int user_id)
	{
		using original_fn = int(__thiscall*)(engine_client*, int);
		return (*(original_fn * *)this)[9](this, user_id);
	}

	void engine_client::get_screen_size(int& width, int& height)
	{
		using original_fn = void(__thiscall*)(engine_client*, int&, int&);
		return (*(original_fn**)this)[5](this, width, height);
	}

	shared::VMatrix& engine_client::world_to_screen_matrix()
	{
		using original_fn = shared::VMatrix& (__thiscall*)(engine_client*);
		return (*(original_fn**)this)[36](this);
	}

	const char* engine_client::get_level_name()
	{
		using original_fn = const char*(__thiscall*)(engine_client*);
		return (*(original_fn * *)this)[51](this);
	}
	
	bool engine_client::get_player_info(int id, player_info_t* info)
	{
		using original_fn = bool(__thiscall*)(engine_client*, int, player_info_t*);
		return (*(original_fn * *)this)[8](this, id, info);
	}

	void engine_client::execute_client_cmd(const char* m_cmd)
	{
		using original_fn = void(__thiscall*)(engine_client*, const char*);
		return (*(original_fn * *)this)[103](this, m_cmd);
	}

	void engine_client::execute_client_cmd_unrestricted(const char* m_cmd)
	{
		using original_fn = void(__thiscall*)(engine_client*, const char*);
		return (*(original_fn**)this)[108](this, m_cmd);
	}
}
