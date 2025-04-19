#include "std_include.hpp"
#include <psapi.h>

#include "modules/d3d9ex.hpp"

BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM lParam)
{
	DWORD window_pid, target_pid = static_cast<DWORD>(lParam);
	GetWindowThreadProcessId(hwnd, &window_pid);

	if (window_pid == target_pid && IsWindowVisible(hwnd))
	{
		char class_name[256]; char debug_msg[256];
		GetClassNameA(hwnd, class_name, sizeof(class_name));
		wsprintfA(debug_msg, "HWND: %p, PID: %u, Class: %s, Visible: %d", hwnd, window_pid, class_name, IsWindowVisible(hwnd));
		shared::common::console(); std::cout << debug_msg << "\n";

		if (std::string_view(class_name) == "FEAR"s)
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

	if (sha1 == "cc74990b4df5378caf4d5342fd3fc73f410a3eca")
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

	SetWindowTextA(shared::globals::main_window, "Fear 1 - RTX");
	mods::fear1::main();
	return 0;
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

		shared::common::loader::module_loader::register_module(std::make_unique<mods::fear1::d3d9ex>());

		if (const auto t = CreateThread(nullptr, 0, find_game_window_by_sha1, nullptr, 0, nullptr); t) {
			CloseHandle(t);
		}
	}

	return TRUE;
}