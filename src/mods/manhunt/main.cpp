#include "std_include.hpp"
#include <psapi.h>

#include "shared/common/flags.hpp"

namespace mods::manhunt
{
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

			if (std::string_view(class_name).contains("Manhunt"s))
			{
				shared::globals::main_window = hwnd;
				return FALSE;
			}
		}

		return TRUE;
	}

//#define GET_MODULE_HANDLE(HANDLE_OUT, NAME, T) \
//	while (!(HANDLE_OUT)) { \
//		if ((HANDLE_OUT) = (DWORD)GetModuleHandleA(NAME); !(HANDLE_OUT)) { \
//			Sleep(1u); (T) += 1u; \
//			if ((T) >= 30000) { \
//				shared::common::console(); std::cout << "---------------> Failed to find module: " << (NAME) << "\n" << "Not loading RTX Compatibility Mod.\n"; \
//				return TRUE; \
//			} \
//		} \
//	}

//#define GET_MODULE_HANDLE_WITH_FALLBACK(HANDLE_OUT, NAME, NAME_FALLBACK, T) \
//	while (!(HANDLE_OUT)) { \
//		if ((HANDLE_OUT) = (DWORD)GetModuleHandleA(NAME); !(HANDLE_OUT)) { \
//			if ((HANDLE_OUT) = (DWORD)GetModuleHandleA(NAME_FALLBACK); !(HANDLE_OUT)) { \
//				Sleep(1u); (T) += 1u; \
//				if ((T) >= 30000) { \
//					shared::common::console(); std::cout << "---------------> Failed to find module: " << (NAME) << " / " << (NAME_FALLBACK) << "\n" << "Not loading RTX Compatibility Mod.\n"; \
//					return TRUE; \
//				} \
//			} \
//		} \
//	}

	DWORD WINAPI find_game_window_by_sha1([[maybe_unused]] LPVOID lpParam)
	{
		shared::common::console();
		std::uint32_t T = 0;

		char exe_path[MAX_PATH]; GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
		const std::string sha1 = shared::utils::hash_file_sha1(exe_path);

		std::cout << "[MAIN] Waiting for window with classname containing 'Manhunt' ... \n";

		{
			while (!shared::globals::main_window)
			{
				EnumWindows(enum_windows_proc, static_cast<LPARAM>(GetCurrentProcessId()));
				if (!shared::globals::main_window) {
					Sleep(1u); T += 1u;
				}

				if (T >= 30000)
				{
					Beep(300, 100); Sleep(100); Beep(200, 100);
					shared::common::console(); std::cout << "[MAIN] Could not find Manhunt Window. Not loading RTX Compatibility Mod.\n";
					return TRUE;
				}
			}
		}

		if (!shared::common::flags::has_flag("nobeep")) {
			Beep(523, 100);
		}

		mods::manhunt::main();
		return 0;
	}
}

BOOL APIENTRY DllMain(HMODULE hmodule, const DWORD ul_reason_for_call, LPVOID)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) 
	{
		//shared::common::console();
		shared::globals::setup_dll_module(hmodule);
		shared::globals::setup_exe_module();

		if (const auto MH_INIT_STATUS = MH_Initialize(); MH_INIT_STATUS != MH_STATUS::MH_OK)
		{
			std::cout << "[!][INIT FAILED] MinHook failed to initialize with code: " << MH_INIT_STATUS << "\n";
			return TRUE;
		}

		/*if (shared::common::flags::has_flag("use_signatures")) {
			mods::manhunt::install_signature_patches();
		}*/

		if (const auto t = CreateThread(nullptr, 0, mods::manhunt::find_game_window_by_sha1, nullptr, 0, nullptr); t) {
			CloseHandle(t);
		}
	}

	return TRUE;
}