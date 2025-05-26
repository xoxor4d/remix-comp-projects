#pragma once

#define RENDERER_BASE			game::shaderapidx9_module
//#define STUDIORENDER_BASE		game::studiorender_module
//#define MATERIALSTYSTEM_BASE	game::materialsystem_module
#define ENGINE_BASE				game::engine_module
#define CLIENT_BASE				game::client_module
//#define SERVER_BASE				game::server_module
#define VSTDLIB_BASE			game::vstdlib_module

namespace mods::blackmesa::game
{
	extern DWORD shaderapidx9_module;
	//extern DWORD studiorender_module;
	//extern DWORD materialsystem_module;
	extern DWORD engine_module;
	extern DWORD client_module;
	//extern DWORD server_module;
	extern DWORD vstdlib_module;

	extern void cvar_uncheat(const char* name);
	extern void cvar_uncheat_and_set_int(const char* name, const int val);
	extern void cvar_uncheat_and_set_float(const char* name, const float val);

	extern void init_game_addresses();

	inline CRender* get_engine_renderer() { return reinterpret_cast<CRender*>(ENGINE_BASE + 0x8E27C8); }
	inline IDirect3DDevice9* get_d3d_device() { return reinterpret_cast<IDirect3DDevice9*>(*(DWORD*)(RENDERER_BASE + 0xED788)); }
	inline IShaderAPIDX8* get_shaderapi() { return reinterpret_cast<IShaderAPIDX8*>(*(DWORD*)(RENDERER_BASE + 0xE097C)); }
	inline CCvar* get_icvar() { return reinterpret_cast<CCvar*>((VSTDLIB_BASE + 0x321F0)); }
}
