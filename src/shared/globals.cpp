#include "std_include.hpp"

namespace shared::globals
{
	int game_version = -1;

	D3DXMATRIX IDENTITY =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	D3DXMATRIX TC_TRANSLATE_TO_CENTER =
	{
		 1.0f,  0.0f, 0.0f, 0.0f,	// identity
		 0.0f,  1.0f, 0.0f, 0.0f,	// identity
		 0.0f,  0.0f, 1.0f, 0.0f,	// identity
		-0.5f, -0.5f, 0.0f, 1.0f,	// translate to center
	};

	D3DXMATRIX TC_TRANSLATE_FROM_CENTER_TO_TOP_LEFT =
	{
		1.0f, 0.0f, 0.0f, 0.0f,	// identity
		0.0f, 1.0f, 0.0f, 0.0f,	// identity
		0.0f, 0.0f, 1.0f, 0.0f,	// identity
		0.5f, 0.5f, 0.0f, 1.0f,	// translate back to the top left corner
	};

	std::string root_path;
	HWND main_window = nullptr;

	HMODULE exe_hmodule;
	DWORD exe_module_addr;

	void setup_exe_module()
	{
		exe_hmodule = GetModuleHandleA(nullptr);
		exe_module_addr = (DWORD)exe_hmodule;
	}

	HMODULE dll_hmodule;
	DWORD dll_module_addr;
	void setup_dll_module(const HMODULE mod)
	{
		dll_hmodule = mod;
		dll_module_addr = (DWORD)dll_hmodule;
	}

	IDirect3DDevice9* d3d_device = nullptr;

	bool imgui_is_rendering = false;
	bool imgui_menu_open = false;
	bool imgui_allow_input_bypass = false;
}