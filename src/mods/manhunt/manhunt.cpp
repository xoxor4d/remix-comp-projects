#include "std_include.hpp"
#include "shared/common/flags.hpp"

namespace mods::manhunt
{
	// once per frame
	/*void on_render_scene_hk()
	{
		
	}

	HOOK_RETN_PLACE_DEF(on_render_scene_retn_addr);
	__declspec(naked) void on_render_scene_stub()
	{
		__asm
		{
			pushad;
			call	on_render_scene_hk;
			popad;

			jmp		on_render_scene_retn_addr;
		}
	}*/

	void install_signature_patches()
	{
		std::uint32_t install_counter = 0u;
		std::uint32_t total_patch_amount = 0u;

		/*{
			auto offset = shared::utils::mem::find_pattern_in_module(shared::globals::exe_hmodule, "57 55 83 EC ? 83 3D ? ? ? ? ? 8B 6C 24 ? 74", 0);
			if (offset)
			{
				std::cout << "[SIG] installed CScene::Render hook @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook captainhook(offset, on_render_scene_stub, HOOK_JUMP);
				HOOK_RETN_PLACE(on_render_scene_retn_addr, captainhook.install()->quick()->create_trampoline());
				install_counter++;
			}

			total_patch_amount++;
		}*/

		{
			// no world frustumcull 1 (0x61F1F3)
			const auto offset = shared::utils::mem::find_pattern_in_module(shared::globals::exe_hmodule, "75 ? 42 83 C0", 0);
			if (offset)
			{
				std::cout << "[SIG] installed anti frustum patch 1 @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook::nop(offset, 2);
				install_counter++;
			}

			total_patch_amount++;
		}

		{
			// no world frustumcull 2 (0x61F28C)
			const auto offset = shared::utils::mem::find_pattern_in_module(shared::globals::exe_hmodule, "7D ? 8B 54 24 ? 85 D2", 0);
			if (offset)
			{
				std::cout << "[SIG] installed anti frustum patch 2 @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook::nop(offset, 2);
				install_counter++;
			}

			total_patch_amount++;
		}

		{
			// no world frustumcull 3 (0x61F294)
			const auto offset = shared::utils::mem::find_pattern_in_module(shared::globals::exe_hmodule, "7D ? ? ? ? ? ? ? ? DF E0 25 ? ? ? ? 75 ? B8", 0);
			if (offset)
			{
				std::cout << "[SIG] installed anti frustum patch 3 @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook::nop(offset, 2);
				install_counter++;
			}

			total_patch_amount++;
		}

#if 0
		{
			// disable PVS (0x63A4D0) .. make GetBspPVS return 0
			const auto offset = shared::utils::mem::find_pattern_in_module(shared::globals::exe_hmodule, "8B 44 24 ? 8B 0D ? ? ? ? A3 ? ? ? ? ? ? ? C3", 0);
			if (offset)
			{
				std::cout << "[SIG] installed anti pvs patch 1 @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook::set(offset, 0x31, 0xC0, 0xC3); // xor eax, eax - ret
				install_counter++;
			}

			total_patch_amount++;
		}
#endif

#if 1
		{
			// more entity anti culling (0x474F6F)
			const auto offset = shared::utils::mem::find_pattern_in_module(shared::globals::exe_hmodule, "0F 84 ? ? ? ? 53 FF 15", 0);
			if (offset)
			{
				std::cout << "[SIG] installed anti entity culling 1 @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook::nop(offset, 6);
				install_counter++;
			}

			total_patch_amount++;
		}

		{
			// more entity anti culling (0x475041)
			const auto offset = shared::utils::mem::find_pattern_in_module(shared::globals::exe_hmodule, "75 ? 80 BE ? ? ? ? ? 0F 84 ? ? ? ? A1", 0);
			if (offset)
			{
				std::cout << "[SIG] installed anti entity culling 2 @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook::set<BYTE>(offset, 0xEB);
				install_counter++;
			}

			total_patch_amount++;
		}

		{
			// more entity anti culling (0x474FF7)
			const auto offset = shared::utils::mem::find_pattern_in_module(shared::globals::exe_hmodule, "75 ? 8D 4E ? 51 68", 0);
			if (offset)
			{
				std::cout << "[SIG] installed anti entity culling 3 @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook::set<BYTE>(offset, 0xEB);
				install_counter++;
			}

			total_patch_amount++;
		}
#endif

		// ------------------
		std::cout << "[SIG] Installed " << std::to_string(install_counter) << "/" << std::to_string(total_patch_amount) << " signature patches.\n";
	}

	void main()
	{
		{	// init filepath var
			char path[MAX_PATH]; GetModuleFileNameA(nullptr, path, MAX_PATH);
			shared::globals::root_path = std::filesystem::path(path).parent_path().string();
		}

		shared::common::console();
		game::init_game_addresses();

		if (shared::common::flags::has_flag("use_signatures")) {
			mods::manhunt::install_signature_patches();
		}
		else
		{
			// anti frustum cull (world)
			shared::utils::hook::nop(0x61F1F3, 2);
			shared::utils::hook::nop(0x61F28C, 2);
			shared::utils::hook::nop(0x61F294, 2);

			// disable PVS (0x63A4D0) .. make GetBspPVS return 0
			//shared::utils::hook::set(0x63A4D0, 0x31, 0xC0, 0xC3); // xor eax, eax - ret

			// more anti entity culling
			shared::utils::hook::nop(0x474F6F, 6);
			shared::utils::hook::set<BYTE>(0x475041, 0xEB);
			shared::utils::hook::set<BYTE>(0x474FF7, 0xEB);

			std::cout << "[STATIC] installed static anti frustum & anti PVS patches.\n> Use commandline arg '-use_signatures' to use signatures instead of static offsets if you run into issues.\n";
		}

		MH_EnableHook(MH_ALL_HOOKS);
	}
}
