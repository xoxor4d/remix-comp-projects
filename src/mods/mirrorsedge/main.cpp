#include "std_include.hpp"
#include <psapi.h>

#include "modules/d3d9ex.hpp"
#include "modules/imgui.hpp"

BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM lParam)
{
	DWORD window_pid, target_pid = static_cast<DWORD>(lParam);
	GetWindowThreadProcessId(hwnd, &window_pid);

	if (window_pid == target_pid && IsWindowVisible(hwnd))
	{
		char class_name[256];
		GetClassNameA(hwnd, class_name, sizeof(class_name));

		char debugMsg[256];
		wsprintfA(debugMsg, "HWND: %p, PID: %u, Class: %s, Visible: %d",
			hwnd, window_pid, class_name, IsWindowVisible(hwnd));

		shared::common::console(); std::cout << debugMsg << "\n"; //printf("%s\n", debugMsg);

		if (std::string_view(class_name).contains("UnrealUWindows"))
		{
			shared::globals::main_window = hwnd;
			return FALSE;
		}
	}

	return TRUE;
}

DWORD WINAPI find_game_window_by_sha1([[maybe_unused]] LPVOID lpParam)
{
	std::uint32_t T = 0;

	char exe_path[MAX_PATH];
	GetModuleFileNameA(nullptr, exe_path, MAX_PATH); // Current process executable
	const std::string sha1 = shared::utils::hash_file_sha1(exe_path);

	//Sleep(5000);

	if (sha1 == "51b0808c50d24ee82e4c186d302e1d4acb6d5874")
	{
		while (!shared::globals::main_window)
		{
			EnumWindows(enum_windows_proc, static_cast<LPARAM>(GetCurrentProcessId()));
			if (!shared::globals::main_window) {
				Sleep(100); T += 100;
			}

			if (T >= 30000) 
			{
				Beep(300, 100); Sleep(100); Beep(200, 100);
				shared::common::console(); std::cout << "[!][INIT FAILED] Not loading RTX Compatibility Mod\n";
				return TRUE;
			}
		}
	}
	else
	{
		Beep(300, 100); Sleep(100); Beep(200, 100);
		shared::common::console(); std::cout << "[!][INIT FAILED] Unsupported Game or Version of the Game!\n";
		return TRUE;
	}

#ifdef DEBUG
	Beep(523, 100);
#endif

	SetWindowTextA(shared::globals::main_window, "MirrorsEdge - RTX");
	mods::mirrorsedge::main();
	return 0;
}


void force_sm2_hk()
{
	// GRHIShaderPlatform
	//*reinterpret_cast<int*>(0x1FFA330) = 2;

	// GVertexElementTypeSupport
	//*reinterpret_cast<int*>(0x1FFDA0C) = 0; // PackedNormal
	//*reinterpret_cast<int*>(0x1FFDA10) = 0; // UByte4
	//*reinterpret_cast<int*>(0x1FFDA14) = 0; // UByte4N

	//*reinterpret_cast<int*>(0x1FFDA18) = 0; // not sure what these are used for?
	//*reinterpret_cast<int*>(0x1FFDA1C) = 0; // not sure what these are used for?


	//*reinterpret_cast<int*>(0x1FFDA20) = 0; // Short2N
	//*reinterpret_cast<int*>(0x1FFDA24) = 0; // Half2

	// this is getting set to 0 with sm2 mode
	*reinterpret_cast<int*>(0x1F67604) = 0;


}

__declspec(naked) void force_sm2_stub()
{
	static uint32_t retn_addr = 0x18B12AB;
	__asm
	{
		pushad;
		call	force_sm2_hk;
		popad;

		mov     ecx, 3; // sm2
		jmp		retn_addr;
	}
}

BOOL APIENTRY DllMain(HMODULE, const DWORD ul_reason_for_call, LPVOID)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) 
	{
		if (const auto MH_INIT_STATUS = MH_Initialize(); MH_INIT_STATUS != MH_STATUS::MH_OK)
		{
			shared::common::console(); std::cout << "[!][INIT FAILED] MinHook failed to initialize with code: " << MH_INIT_STATUS << std::endl;
			return TRUE;
		}

		// unlock commandline args (thanks softsoundd)
		shared::utils::hook::set<BYTE>(0x10ED5F3 + 3, 0x0);
		shared::utils::hook::set<BYTE>(0x10EDB38 + 6, 0x0);
		shared::utils::hook::set_wstring(0x1CC3CA0, L"rtx");
		shared::utils::hook::set_wstring(0x1CC3CB8, L"TdMainMenu -sm2 -nostartupmovies"); // FORCESHADERMODEL2 -sm2 -d3ddebug
		//shared::utils::hook::set_wstring(0x1CC3CB8, L"EDITOR"); // FORCESHADERMODEL2 -sm2 -d3ddebug

		// crashes on init @ 0x10B6AF6
		//shared::utils::hook::nop(0x18B12A5, 6);
		//shared::utils::hook(0x18B12A5, force_sm2_stub, HOOK_JUMP).install()->quick();

		// 1BBF768
		//shared::utils::hook::set<BYTE>(0x1BBF768, 0x01);

		//shared::utils::hook::nop(0x10B6AF3, 10);
		//shared::utils::hook::set(0x10B6BBC, 0xE9, 0x8A, 0x05, 0x0, 0x0, 0x90);




		//shared::utils::hook::set<BYTE>(0x18B118D, 0xEB); // skip d3d cpas check (do not support packed normals, half floats etc)
		//shared::utils::hook::set<BYTE>(0x18B118D, 0xEB); // skip d3d cpas check (do not support packed normals, half floats etc)

		mods::mirrorsedge::module_loader::register_module(std::make_unique<mods::mirrorsedge::d3d9ex>());

		if (const auto t = CreateThread(nullptr, 0, find_game_window_by_sha1, nullptr, 0, nullptr); t) {
			CloseHandle(t);
		}
	}

	return TRUE;
}