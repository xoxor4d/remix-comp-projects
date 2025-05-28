#include "std_include.hpp"

namespace mods::blackmesa::game
{
	DWORD shaderapidx9_module = 0u;
	//DWORD studiorender_module = 0u;
	//DWORD materialsystem_module = 0u;
	DWORD engine_module = 0u;
	DWORD client_module = 0u;
	//DWORD server_module = 0u;
	DWORD vstdlib_module = 0u;

	// adds a simple console command
	void con_add_command(ConCommand* cmd, const char* name, void(__cdecl* callback)(), const char* desc)
	{
		// ConCommand *this, const char *pName, void (__cdecl *callback)(), const char *pHelpString, int flags, int (__cdecl *completionFunc)(const char *, char (*)[64]
		shared::utils::hook::call<void(__fastcall)(ConCommand* this_ptr, void* null, const char*, void(__cdecl*)(), const char*, int, int(__cdecl*)(const char*, char(*)[64]))>(CLIENT_BASE + 0x338690)
			(cmd, nullptr, name, callback, desc, 0x20000, nullptr);
	}

	void cvar_uncheat(const char* name)
	{
		if (const auto ivar = game::get_icvar(); ivar)
		{
			if (auto var = ivar->vftable->FindVar(ivar, name); var) {
				var->m_nFlags &= ~0x4000;
			}
		}
	}

	void cvar_uncheat_and_set_int(const char* name, const int val)
	{
		if (const auto ivar = game::get_icvar(); ivar)
		{
			if (auto var = ivar->vftable->FindVar(ivar, name); var)
			{
				var->vtbl->SetValue_Int(var, val);
				var->m_nFlags &= ~0x4000;
			}
		}
	}

	void cvar_uncheat_and_set_float(const char* name, const float val)
	{
		if (const auto ivar = game::get_icvar(); ivar)
		{
			if (auto var = ivar->vftable->FindVar(ivar, name); var)
			{
				var->vtbl->SetValue_Float(var, val);
				var->m_nFlags &= ~0x4000;
			}
		}
	}

	int get_visframecount() {
		return *reinterpret_cast<int*>(ENGINE_BASE + 0x9B95F8); // 0125
	}

	// init any adresses here
	void init_game_addresses()
	{
	}
}
