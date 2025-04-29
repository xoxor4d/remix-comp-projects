#pragma once
#include "utils/utils.hpp"

namespace shared::globals
{
	extern int game_version;

	template<typename... Args>
	bool check_game_support_by_sha1(std::string exe_sha1, Args... hashes)
	{
		utils::to_lower(exe_sha1);

		std::vector<std::string> supported_version_hashes = { static_cast<std::string>(hashes)... };
		for (auto i = 0u; i < supported_version_hashes.size(); i++)
		{
			if (utils::str_to_lower(supported_version_hashes[i]) == exe_sha1)
			{
				game_version = static_cast<int>(i);
				return true;
			}
		}

		return false;
	}

	template<typename... Args>
	DWORD CHOOSE_OFFSET(Args... offsets)
	{
		if (game_version >= 0)
		{
			const std::vector<DWORD> offset_vec = { static_cast<DWORD>(offsets)... };
			if (static_cast<int>(offset_vec.size()) > game_version) {
				return offset_vec[game_version];
			}
		}

		throw std::runtime_error("[GET_OFFSET] Unsupported game version or insufficient offsets!");
	}

	extern D3DXMATRIX IDENTITY;
	extern D3DXMATRIX TC_TRANSLATE_TO_CENTER;
	extern D3DXMATRIX TC_TRANSLATE_FROM_CENTER_TO_TOP_LEFT;

	extern std::string root_path;
	extern HWND main_window;

	extern HMODULE dll_hmodule;
	extern IDirect3DDevice9* d3d_device;
}