#pragma once

#define RENDERER_BASE			game::shaderapidx9_module
#define STUDIORENDER_BASE		game::studiorender_module
//#define MATERIALSTYSTEM_BASE	game::materialsystem_module
#define ENGINE_BASE				game::engine_module
#define CLIENT_BASE				game::client_module
//#define SERVER_BASE				game::server_module
#define VSTDLIB_BASE			game::vstdlib_module
#define STDSHADERDX9_BASE		game::stdshader_dx9_module

namespace mods::blackmesa::game
{
	extern DWORD shaderapidx9_module;
	extern DWORD studiorender_module;
	//extern DWORD materialsystem_module;
	extern DWORD engine_module;
	extern DWORD client_module;
	//extern DWORD server_module;
	extern DWORD vstdlib_module;
	extern DWORD stdshader_dx9_module;

	extern void con_add_command(ConCommand* cmd, const char* name, void(__cdecl* callback)(), const char* desc);
	extern void cvar_uncheat(const char* name);
	extern void cvar_uncheat_and_set_int(const char* name, const int val);
	extern void cvar_uncheat_and_set_float(const char* name, const float val);

	extern void init_game_addresses();

	inline CRender* get_engine_renderer() { return reinterpret_cast<CRender*>(ENGINE_BASE + 0x8E27C8); }
	inline IDirect3DDevice9* get_d3d_device() { return reinterpret_cast<IDirect3DDevice9*>(*(DWORD*)(RENDERER_BASE + 0xED788)); }
	inline IShaderAPIDX8* get_shaderapi() { return reinterpret_cast<IShaderAPIDX8*>(*(DWORD*)(RENDERER_BASE + 0xE097C)); }
	inline worldbrushdata_t* get_hoststate_worldbrush_data() { return reinterpret_cast<CCommonHostState*>(ENGINE_BASE + 0x974BBC)->worldbrush; }
	inline CGlobalVarsBase* get_global_vars() { return reinterpret_cast<CGlobalVarsBase*>(*(DWORD*)(CLIENT_BASE + 0x530A00)); }
	inline CCvar* get_icvar() { return reinterpret_cast<CCvar*>((VSTDLIB_BASE + 0x321F0)); }

	inline Vector* get_current_view_origin() { return reinterpret_cast<Vector*>(ENGINE_BASE + 0x4E0CEC); }
	inline Vector* get_current_view_forward() { return reinterpret_cast<Vector*>(ENGINE_BASE + 0x3F3164); }
	inline Vector* get_current_view_right() { return reinterpret_cast<Vector*>(ENGINE_BASE + 0x3F3170); }
	inline Vector* get_current_view_up() { return reinterpret_cast<Vector*>(ENGINE_BASE + 0x3F317C); }

	inline view_id* get_current_view_id() { return reinterpret_cast<view_id*>(CLIENT_BASE + 0x539A54); }
	extern view_id saved_view_id;

	// CM_PointLeafnum
	inline int get_leaf_from_position(const Vector& pos) { return shared::utils::hook::call<int(__cdecl)(const float*)>(ENGINE_BASE + 0x185D00)(&pos.x); }

	inline bool is_paused()
	{
		// CEngineTool::IsGamePaused
		return shared::utils::hook::call<bool(__cdecl)()>(ENGINE_BASE + 0xA6CC0)();
		
		//// 4A9C38 BaseLocalClient
		//const auto blc = reinterpret_cast<void*>(ENGINE_BASE + 0x4A9C38);
		//// CClientState::IsPaused
		//return shared::utils::hook::call<BOOL(__fastcall)(void* this_ptr, void* null)>(ENGINE_BASE + 0xD4D00)(blc, nullptr);
	}

	extern void debug_add_text_overlay(const float* pos, const char* text, const int line_offset, const float r, const float g, const float b, const float a);

	extern int get_visframecount();
}
