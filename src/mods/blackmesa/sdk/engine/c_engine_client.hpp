#pragma once
#define ENGINE_INTERFACE_VERSION "VEngineClient015"

namespace sdk
{
	struct player_info_t 
	{
	private:
		char __pad00[0x8];

	public:
		char name[32];
		int  userid;

	private:
		char __pad01[0x150];
	};

	class engine_client {
	public:
		int get_local_player();
		int get_player_for_user_id(int user_id);
		const char* get_level_name();
		bool get_player_info(int id, player_info_t* info);
		void execute_client_cmd(const char* m_cmd);
		void execute_client_cmd_unrestricted(const char* m_cmd);
	};
}
