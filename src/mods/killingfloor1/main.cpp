#include "std_include.hpp"
#include <psapi.h>

#include "modules/d3d9ex.hpp"

namespace mods::killingfloor1
{
	BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM lParam)
	{
		DWORD window_pid, target_pid = static_cast<DWORD>(lParam);
		GetWindowThreadProcessId(hwnd, &window_pid);

		if (window_pid == target_pid && IsWindowVisible(hwnd))
		{
			char class_name[256]; //char debug_msg[256];
			GetClassNameA(hwnd, class_name, sizeof(class_name));
			//wsprintfA(debug_msg, "HWND: %p, PID: %u, Class: %s, Visible: %d", hwnd, window_pid, class_name, IsWindowVisible(hwnd));
			//shared::common::console(); std::cout << debug_msg << "\n";

			if (std::string_view(class_name).contains("UnrealWWindows"s))
			{
				shared::globals::main_window = hwnd;
				return FALSE;
			}
		}

		return TRUE;
	}

#define GET_MODULE_HANDLE(HANDLE_OUT, NAME, T) \
	while (!(HANDLE_OUT)) { \
		if ((HANDLE_OUT) = (DWORD)GetModuleHandleA(NAME); !(HANDLE_OUT)) { \
			Sleep(1u); (T) += 1u; \
			if ((T) >= 30000) { \
				shared::common::console(); std::cout << "---------------> Failed to find module: " << (NAME) << std::endl; \
				return TRUE; \
			} \
		} \
	}

	DWORD WINAPI find_game_window_by_sha1([[maybe_unused]] LPVOID lpParam)
	{
		std::uint32_t T = 0;

		char exe_path[MAX_PATH]; GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
		const std::string sha1 = shared::utils::hash_file_sha1(exe_path);

		if (shared::globals::check_game_support_by_sha1(sha1, 
			"ED8EE6505EE506312365AF6C3D192032109FF885")) // steam
		{
			while (!shared::globals::main_window)
			{
				if (!game::rendev_module)
				{
					GET_MODULE_HANDLE(game::rendev_module, "D3D9Drv.dll", T);
					shared::common::loader::module_loader::register_module(std::make_unique<mods::killingfloor1::d3d9ex>());
				}

				EnumWindows(enum_windows_proc, static_cast<LPARAM>(GetCurrentProcessId()));
				if (!shared::globals::main_window) {
					Sleep(1u); T += 1u;
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

		GET_MODULE_HANDLE(game::engine_module, "Engine.dll", T);

#ifdef DEBUG
		Beep(523, 100);
#endif

		SetWindowTextA(shared::globals::main_window, "KillingFloor - RTX");
		mods::killingfloor1::main();
		return 0;
	}
}

BOOL APIENTRY DllMain(HMODULE hmodule, const DWORD ul_reason_for_call, LPVOID)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) 
	{
		shared::globals::dll_hmodule = hmodule;

		if (const auto MH_INIT_STATUS = MH_Initialize(); MH_INIT_STATUS != MH_STATUS::MH_OK)
		{
			shared::common::console(); std::cout << "[!][INIT FAILED] MinHook failed to initialize with code: " << MH_INIT_STATUS << std::endl;
			return TRUE;
		}

		if (const auto t = CreateThread(nullptr, 0, mods::killingfloor1::find_game_window_by_sha1, nullptr, 0, nullptr); t) {
			CloseHandle(t);
		}
	}

	return TRUE;
}