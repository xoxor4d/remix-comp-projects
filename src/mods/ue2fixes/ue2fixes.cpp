#include "std_include.hpp"
#include "modules/imgui.hpp"
#include "shared/common/flags.hpp"

namespace mods::ue2fixes
{
	void post_get_view_frustum_hk(game::FConvexVolume* frustum)
	{
		const auto& im = imgui::get();
		if (im->m_cull_disable_frustum)
		{
			for (auto i = 0u; i < 5; i++) {
				frustum->BoundingPlanes[i].dist += im->m_cull_frustum_tweak_distance_offset; //10000.0f;
			}
		}
	}

	HOOK_RETN_PLACE_DEF(post_get_view_frustum_retn_addr);
	__declspec(naked) void post_get_view_frustum_stub()
	{
		__asm
		{
			pushad;
			push	eax;
			call	post_get_view_frustum_hk;
			add		esp, 4;
			popad;

			jmp		post_get_view_frustum_retn_addr;
		}
	}

	void install_signature_patches()
	{
		std::uint32_t install_counter = 0u;
		std::uint32_t total_patch_amount = 0u;

		{
			auto offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "FF ?? ?? 89 ?? 89 ?? 04 8B ?? EB 0D 33 C0 89 ?? 89 ?? 04 8B ?? EB 02", 3);
			if (!offset) { offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "68 ?? ?? ?? ?? B9 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 3B ?? 74 ?? 8B ?? 50 8B ?? FF ?? ?? 89 ?? 89 ?? 04", 28); }

			if (offset)
			{
				std::cout << "[SIG] installed view frustum hook @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook captainhook(offset, post_get_view_frustum_stub, HOOK_JUMP);
				HOOK_RETN_PLACE(post_get_view_frustum_retn_addr, captainhook.install()->quick()->create_trampoline());
				install_counter++;
			}

			total_patch_amount++;
		}


		// anti culling patches

		{	// #1
			auto offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "0F 84 ?? ?? ?? 00 8B ?? 04 84 C0 89 55 ?? 89 55 ?? 0F 89 ?? ?? ?? 00 A8 01 75 ??", 0);
			if (offset)
			{
				std::cout << "[SIG] installed anti culling patch #1 @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook::nop(offset, 6);
				install_counter++;
			}

			total_patch_amount++;
		}

#if 0
		{	// #2
			auto offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "0F 84 ?? ?? ?? FF 83 ?? ?? FF 0F 84 ?? ?? ?? FF 8B ?? 04 8B ?? ?? F7 ?? ?? 05 00 00 00 00", 0);
			if (offset)
			{
				std::cout << "[SIG] installed anti culling patch #2 @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook::nop(offset, 6);
				install_counter++;

				// #3
				if (*reinterpret_cast<BYTE*>(offset + 10) == 0x0F)
				{
					std::cout << "[SIG] installed anti culling patch #3 @ 0x" << std::uppercase << std::hex << offset << "!\n";
					shared::utils::hook::nop(offset + 10, 6);
					install_counter++;
				}
				else std::cout << "[SIG] unexpected instruction @ anti culling patch #3 - was " << std::uppercase << std::hex << *reinterpret_cast<BYTE*>(offset + 10) << "\n";

				// #4
				if (*reinterpret_cast<BYTE*>(offset + 32) == 0x0F)
				{
					std::cout << "[SIG] installed anti culling patch #4 @ 0x" << std::uppercase << std::hex << offset << "!\n";
					shared::utils::hook::nop(offset + 32, 6);
					install_counter++;
				}
				else std::cout << "[SIG] unexpected instruction @ anti culling patch #4 - was " << std::uppercase << std::hex << *reinterpret_cast<BYTE*>(offset + 32) << "\n";
			}
			else
			{
				// r6 raven
				std::cout << "[SIG] trying different signatures for anti culling patch #2 #3 #4 ...\n";

				// #2
				offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "74 ?? 83 ?? ?? FF  74 ?? 8B ?? ?? F7 ?? ?? ?? ?? 00 00 00 00 02 74 ??", 0);
				if (offset)
				{
					std::cout << "[SIG] installed anti culling patch #2 @ 0x" << std::uppercase << std::hex << offset << "!\n";
					shared::utils::hook::nop(offset, 2);
					install_counter++;

					// #3
					if (*reinterpret_cast<BYTE*>(offset + 6) == 0x74)
					{
						std::cout << "[SIG] installed anti culling patch #3 @ 0x" << std::uppercase << std::hex << offset << "!\n";
						shared::utils::hook::nop(offset + 6, 2);
						install_counter++;
					}
					else std::cout << "[SIG] unexpected instruction @ anti culling patch #3 - was " << std::uppercase << std::hex << *reinterpret_cast<BYTE*>(offset + 6) << "\n";

					// #4
					if (*reinterpret_cast<BYTE*>(offset + 21) == 0x74)
					{
						std::cout << "[SIG] installed anti culling patch #4 @ 0x" << std::uppercase << std::hex << offset << "!\n";
						shared::utils::hook::nop(offset + 21, 2);
						install_counter++;
					}
					else std::cout << "[SIG] unexpected instruction @ anti culling patch #4 - was " << std::uppercase << std::hex << *reinterpret_cast<BYTE*>(offset + 21) << "\n";
				}
			}

			total_patch_amount += 3;
		}


		{	// #5
			auto offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "0F ?? ?? ?? ?? 00 A9 00 00 00 04 0F ?? ?? ?? ?? 00", 0);
			if (offset)
			{
				std::cout << "[SIG] installed anti culling patch #5 @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook::nop(offset, 6);
				install_counter++;

				// #6
				if (*reinterpret_cast<BYTE*>(offset + 11) == 0x0F)
				{
					std::cout << "[SIG] installed anti culling patch #6 @ 0x" << std::uppercase << std::hex << offset << "!\n";
					shared::utils::hook::nop(offset + 11, 6);
					install_counter++;
				}
				else std::cout << "[SIG] unexpected instruction @ anti culling patch #6 - was " << std::uppercase << std::hex << *reinterpret_cast<BYTE*>(offset + 11) << "\n";
			}

			total_patch_amount += 2;
		}

		{	// #7
			auto offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "8D ?? ?? ?? ?? 00 56 FF ?? ?? 85 ?? 0F 84 ?? ?? ?? 00 8B ?? D9 ?? ?? ?? ?? ?? 8B ?? ?? 8B ?? D8", 12); // swat4
			if (!offset) { offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "FF ?? 08 85 C0 0F 84 ?? ?? ?? 00 8B ?? 08 8B ?? 8B ?? 04", 5); } // killingfloor
			if (!offset) { offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "FF ?? 08 85 C0 0F 84 ?? ?? ?? 00 8B ?? 8B ?? 04", 5); } // r6raven

			if (offset)
			{
				if (*reinterpret_cast<BYTE*>(offset) == 0x0F)
				{
					std::cout << "[SIG] installed anti culling patch #7 @ 0x" << std::uppercase << std::hex << offset << "!\n";
					shared::utils::hook::nop(offset, 6);
					install_counter++;
				}
				else std::cout << "[SIG] unexpected instruction @ anti culling patch #7 - was " << std::uppercase << std::hex << *reinterpret_cast<BYTE*>(offset + 11) << "\n";
			}

			total_patch_amount++;
		}
#endif

#if 1
		{	// #2 - Visibility MaskTests
			auto offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "74 ?? 8B ?? ?? 8B ?? ?? ?? ?? 00 23 ?? 8B ?? ?? ?? ?? 00 23 ?? ?? 0B ??", 0); // swat4
			if (!offset) { offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "75 ?? 8B ?? 08 8B ?? ?? ?? 00 00 85 C9 74 ?? A1 ?? ?? ??", 0); } // killingfloor
			if (offset)
			{
				std::cout << "[SIG] installed anti culling patch #2 @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook::set<BYTE>(offset, 0xEB);
				install_counter++;
			}
			else
			{
				offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "0F ?? ?? ?? ?? 00 8B ?? ?? ?? ?? 00 85 C9 0F ?? ?? ?? ?? 00 33 C0", 0); // r6
				if (offset)
				{
					if (shared::utils::hook::conditional_jump_to_jmp(offset))
					{
						std::cout << "[SIG] installed anti culling patch #2 @ 0x" << std::uppercase << std::hex << offset << "!\n";
						install_counter++;
					}
				}
			}

			total_patch_amount++;
		}


		{	// #3 - Visibility BoxTests
			auto offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "83 ?? FF 0F ?? ?? ?? ?? 00  8B ?? ?? ?? ?? 00 6B ?? 1C 8B", 3); // swat4
			if (!offset) { offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "8B ?? ?? 83 F8 FF 0F 84 ?? ?? 00 00 8B ?? ??", 6); } // killingfloor + r6
			if (offset)
			{
				if (shared::utils::hook::conditional_jump_to_jmp(offset))
				{
					std::cout << "[SIG] installed anti culling patch #3 @ 0x" << std::uppercase << std::hex << offset << "!\n";
					install_counter++;
				}
			}

			total_patch_amount++;
		}
#endif

#if 1
		{	// backface culling
			auto offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "75 ?? 8B ?? C8 01 00 00 85 ?? 0F 85 ?? ?? 00 00 8B ?? C8 01 00 00", 0); // swat4 + killingfloor
			if (!offset) { offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "75 ?? 8B ?? 39 ?? ?? 01 00 00 74 ?? 8B ?? ?? F6 ?? ?? ?? 00 00 01", 0); } // r6
			if (offset)
			{
				std::cout << "[SIG] installed backface culling patch @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook::set<BYTE>(offset, 0xEB);
				install_counter++;
			}

			total_patch_amount++;
		}
#endif

		
		{	// disable sky
			if (shared::common::flags::has_flag("disable_sky") || shared::common::flags::has_flag("anticullex"))
			{
				auto offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "85 ?? 0F 84 CD ?? 00 00 F6 ?? ?? ?? ?? 00 01 0F 85 ?? ?? 00 00 8B", 15); // swat4 + killingfloor
				if (!offset) { offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "F6 ?? ?? ?? 00 00 01 0F 85 ?? ?? 00 00 8B ?? ?? ?? ?? 00 85 ?? 0F 84 ?? ?? 00 00", 7); } // r6
				if (offset)
				{
					if (shared::utils::hook::conditional_jump_to_jmp(offset))
					{
						std::cout << "[SIG] installed disable sky patch @ 0x" << std::uppercase << std::hex << offset << "!\n";
						install_counter++;
					}
				}

				total_patch_amount++;
			}
		}


		{	// #7 - extended anticulling
			if (shared::common::flags::has_flag("anticullex"))
			{	
				auto offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "8D ?? ?? ?? ?? 00 56 FF ?? ?? 85 ?? 0F 84 ?? ?? ?? 00 8B ?? D9 ?? ?? ?? ?? ?? 8B ?? ?? 8B ?? D8", 12); // swat4
				if (!offset) { offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "FF ?? 08 85 C0 0F 84 ?? ?? ?? 00 8B ?? 08 8B ?? 8B ?? 04", 5); } // killingfloor
				if (!offset) { offset = shared::utils::mem::find_pattern_in_module((HMODULE)game::engine_module, "FF ?? 08 85 C0 0F 84 ?? ?? ?? 00 8B ?? 8B ?? 04", 5); } // r6raven

				if (offset)
				{
					if (*reinterpret_cast<BYTE*>(offset) == 0x0F)
					{
						std::cout << "[SIG] installed anti culling patch #7 @ 0x" << std::uppercase << std::hex << offset << "!\n";
						shared::utils::hook::nop(offset, 6);
						install_counter++;
					}
					else std::cout << "[SIG] unexpected instruction @ anti culling patch #7 - was " << std::uppercase << std::hex << *reinterpret_cast<BYTE*>(offset + 11) << "\n";
				}

				total_patch_amount++;
			}
		}
		

		// ------------------
		std::cout << "[SIG] Installed " << std::to_string(install_counter) << "/" << std::to_string(total_patch_amount) << " signature patches.\n";
	}

	void main()
	{
		{	// init filepath var
			char path[MAX_PATH]; GetModuleFileNameA(nullptr, path, MAX_PATH);
			shared::globals::root_path = std::filesystem::path(path).parent_path().string();
		}

		game::init_game_addresses();

		shared::common::loader::module_loader::register_module(std::make_unique<imgui>());

		MH_EnableHook(MH_ALL_HOOKS);
	}
}
