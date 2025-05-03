#include "std_include.hpp"
#include <psapi.h>

#include "modules/d3d9ex.hpp"

std::unordered_set<HWND> wnd_class_list; // so we don't print the same window strings over and over again

BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM lParam)
{
	DWORD window_pid, target_pid = static_cast<DWORD>(lParam);
	GetWindowThreadProcessId(hwnd, &window_pid);

	if (window_pid == target_pid && IsWindowVisible(hwnd))
	{
		char class_name[256];
		GetClassNameA(hwnd, class_name, sizeof(class_name));

		if (!wnd_class_list.contains(hwnd))
		{
			char debug_msg[256];
			wsprintfA(debug_msg, "|> HWND: %p, PID: %u, Class: %s, Visible: %d \n", hwnd, window_pid, class_name, IsWindowVisible(hwnd));
			std::cout << debug_msg;
			wnd_class_list.insert(hwnd);
		}

		// BioshockUnrealWWindowsViewportWindow
		if (std::string_view(class_name).contains("UnrealWWindows"))
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

	if (sha1 == "9df7b1406ab76e547034a193cf5360d2dc4a126a")
	{
		std::cout << "[MAIN] Waiting for window with classname containing 'UnrealWWindows' ... \n";
		while (!shared::globals::main_window)
		{
			EnumWindows(enum_windows_proc, static_cast<LPARAM>(GetCurrentProcessId()));
			if (!shared::globals::main_window) {
				Sleep(10u); T += 10u;
			}

			if (T >= 30000) 
			{
				Beep(300, 100); Sleep(100); Beep(200, 100);
				shared::common::console(); std::cout << "[MAIN] Could not find UnrealWindow. Not loading RTX Compatibility Mod.\n";
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

	SetWindowTextA(shared::globals::main_window, "Bioshock 1 - RTX");
	mods::bioshock1::main();
	return 0;
}

BOOL APIENTRY DllMain(HMODULE, const DWORD ul_reason_for_call, LPVOID)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) 
	{
#if DEBUG
		shared::common::console();
#endif

		if (const auto MH_INIT_STATUS = MH_Initialize(); MH_INIT_STATUS != MH_STATUS::MH_OK)
		{
			std::cout << "[!][INIT FAILED] MinHook failed to initialize with code: " << MH_INIT_STATUS << std::endl;
			return TRUE;
		}

		mods::bioshock1::module_loader::register_module(std::make_unique<mods::bioshock1::d3d9ex>());
		mods::bioshock1::init_early_hooks();

		if (const auto t = CreateThread(nullptr, 0, find_game_window_by_sha1, nullptr, 0, nullptr); t) {
			CloseHandle(t);
		}
	}

	return TRUE;
}