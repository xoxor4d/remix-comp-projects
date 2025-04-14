#pragma once

namespace shared::globals
{
	extern D3DXMATRIX IDENTITY;
	extern D3DXMATRIX TC_TRANSLATE_TO_CENTER;
	extern D3DXMATRIX TC_TRANSLATE_FROM_CENTER_TO_TOP_LEFT;

	extern std::string root_path;
	extern HWND main_window;

	extern IDirect3DDevice9* d3d_device;
}