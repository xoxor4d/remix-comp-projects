#include "std_include.hpp"
#include "modules/imgui.hpp"
#include "shared/common/flags.hpp"

namespace mods::test
{
	struct frustum
	{
		char pad_0x0000[0x3C]; //0x0000
		Vector unk_vec3; //0x003C 
		char pad_0x0048[0x8]; //0x0048
		float windows_w; //0x0050 
		float window_h; //0x0054 
		Vector2D unk_vec2; //0x0058 
		char pad_0x0060[0x8]; //0x0060
		float large_float; //0x0068 
		char pad_0x006C[0xC]; //0x006C

		int frustum_indices[13];

		Vector p1_xyz;
		float p1_dist;

		Vector p2_xyz;
		float p2_dist;

		Vector p3_xyz;
		float p3_dist;

		Vector p4_xyz;
		float p4_dist;

		Vector p5_xyz;
		float p5_dist;

		Vector p6_xyz;
		float p6_dist;
	};

	//frustum* p_f = nullptr;

	// !!
	// 64E2B8 jmp to disable normalization after changes to frustum! -- not needed

	// !!
	// 004465A8 to mulss xmm1,[0067A7E4] (0.0) -- done .. change to 0.01 to not cripple perf --> 67D9F0 0.01

	// !!
	// 649343 nop 2 for more drawing

	void post_get_view_frustum_hk(frustum* f)
	{
		//p_f = f;
		//f->large_float = 36000.0f;

		// Only change the actual game camera and not the menus
		if (f->large_float == 36000.0f) 
		{
			//f->p1_dist = 0;
			//f->p2_dist = 0;
			//f->p3_dist = 0;
			//f->p4_dist = 1;
			//f->p5_dist = 0;
			//f->p6_dist = 0;
			f->unk_vec2.x = 0.01f; // disable frustum culling - do not adjust on menus! .. scale towards 0 increases menu scale

			f->p1_dist *= 50.0f;
			f->p2_dist *= 50.0f;

			//f->p3_xyz.x *= 10.0f;
			//f->p3_xyz.y *= 0.0f;
			f->p3_dist *= 50.0f;

			//f->p4_xyz.x = 100.0f;
			//f->p4_xyz.y = -0.5f;
			f->p4_dist *= 50.0f;

			f->p5_dist *= 50.0f;
			f->p6_dist *= 50.0f;

			int y = 0;  
		}
	}

	HOOK_RETN_PLACE_DEF(post_get_view_frustum_retn_addr);
	__declspec(naked) void post_get_view_frustum_stub()
	{
		__asm
		{
			pushad;
			push	edi;
			call	post_get_view_frustum_hk;
			add		esp, 4;
			popad;

			jmp		post_get_view_frustum_retn_addr;
		}
	}


	struct post
	{
		Vector4D x1;
		Vector4D x2;
		Vector4D x3;
		Vector4D x4;
		Vector4D x5;
		Vector4D x6;
	};

	void post_nrm(post* f)
	{
		/*auto asd = p_f;
		if (asd)
		{
			int y = 0;
		}*/

		f->x1.x = 0.0f;
		f->x1.y = 0.0f;
		f->x1.z = 0.0f;
		f->x1.w = 0.0f;

		f->x2.x = 0.0f;
		f->x2.y = 0.0f;
		f->x2.z = 0.0f;
		f->x2.w = 0.0f;

		f->x3.x = 0.0f;
		f->x3.y = 0.0f;
		f->x3.z = 0.0f;
		f->x3.w = 0.0f;

		f->x4.x = 0.0f;
		f->x4.y = 0.0f;
		f->x4.z = 0.0f;
		f->x4.w = 0.0f;

		f->x5.x = 0.0f;
		f->x5.y = 0.0f;
		f->x5.z = 0.0f;
		f->x5.w = 0.0f;

		f->x6.x = 0.0f;
		f->x6.y = 0.0f;
		f->x6.z = 0.0f;
		f->x6.w = 0.0f;
		int x = 1;  
	}

	__declspec(naked) void post_nrm_stub()
	{
		static uint32_t retn_addr = 0x64E363;
		__asm
		{
			/*pushad;
			push	eax;
			call	post_nrm;
			add		esp, 4;
			popad;*/

			add     esp, 0xC;
			mov     dword ptr[ebp - 0x198], 0x461c4000; // fixes void NaN's
			jmp		retn_addr;
		}
	}

	void install_signature_patches()
	{
		std::uint32_t install_counter = 0u;
		std::uint32_t total_patch_amount = 0u;

		{
			// C7 47 44 00 00 80 3F 5F 5E 8B E5
			auto offset = shared::utils::mem::find_pattern_in_module(game::exe_module, "C7 47 44 00 00 80 3F 5F 5E 8B E5", 7);
			
			if (offset)
			{
				std::cout << "[SIG] installed view frustum hook @ 0x" << std::uppercase << std::hex << offset << "!\n";
				shared::utils::hook captainhook(offset, post_get_view_frustum_stub, HOOK_JUMP);
				HOOK_RETN_PLACE(post_get_view_frustum_retn_addr, captainhook.install()->quick()->create_trampoline());
				install_counter++;
			}

			total_patch_amount++;
		}

		// ------------------
		std::cout << "[SIG] Installed " << std::to_string(install_counter) << "/" << std::to_string(total_patch_amount) << " signature patches.\n";

		// only needed to fix void NaN's
		shared::utils::hook::nop(0x64E356, 11); shared::utils::hook(0x64E356, post_nrm_stub, HOOK_JUMP).install()->quick();

		// anticull: 0.5 to mulss xmm1,[0067A7E4] (which holds 0.0) .... 67D9F0 0.01
		//shared::utils::hook::set(0x4465A8, 0xF3, 0x0F, 0x59, 0x0D, 0xF0, 0xD9, 0x67, 0x00);

		// more anticull .. bad performance
		shared::utils::hook::nop(0x649343, 2);

		// 649340 to test    bl, 1 (from 2) to disable backface cull?
		shared::utils::hook::set<BYTE>(0x649340 + 2, 1);


		// fix sky NaN's
		shared::utils::hook::nop(0x6099A6, 2);
	}

	void main()
	{
		{	// init filepath var
			char path[MAX_PATH]; GetModuleFileNameA(nullptr, path, MAX_PATH);
			shared::globals::root_path = std::filesystem::path(path).parent_path().string();
		}

		game::init_game_addresses();

		//shared::common::loader::module_loader::register_module(std::make_unique<imgui>());

		MH_EnableHook(MH_ALL_HOOKS);
	}
}
