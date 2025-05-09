#include "std_include.hpp"
#include "modules/imgui.hpp"
#include "shared/common/dinput_hook.hpp"
#include "shared/common/flags.hpp"

namespace mods::wolfenstein2k9
{
	struct RenderContext
	{
		D3DXMATRIX unkMatrix1;
		D3DXMATRIX modelMatrix;
		D3DXMATRIX unkMatrix2;
		D3DXMATRIX worldViewProjection;
		D3DXMATRIX unkMatrix3;
		D3DXMATRIX unkMatrix4;
		D3DXMATRIX projectionMatrix;
		D3DXMATRIX unkMatrix5;
		char padding4[61];
		char padding5[8];
	};


	void on_begin_scene()
	{
		if (const auto im = imgui::get(); im)
		{
			if (im->m_dbg_use_fake_camera)
			{
				D3DXMATRIX rotation, translation, view_matrix, proj_matrix;

				D3DXMatrixRotationYawPitchRoll(&rotation,
					D3DXToRadian(im->m_dbg_camera_yaw),		// Yaw in radians
					D3DXToRadian(im->m_dbg_camera_pitch),	// Pitch in radians
					0.0f);									// No roll for simplicity

				D3DXMatrixTranslation(&translation,
					-im->m_dbg_camera_pos[0],				// Negate for camera (moves world opposite)
					-im->m_dbg_camera_pos[1],
					-im->m_dbg_camera_pos[2]);

				D3DXMatrixMultiply(&view_matrix, &rotation, &translation);

				// Construct projection matrix
				D3DXMatrixPerspectiveFovLH(&proj_matrix,
					D3DXToRadian(im->m_dbg_camera_fov),		// FOV in radians
					im->m_dbg_camera_aspect,
					im->m_dbg_camera_near_plane,
					im->m_dbg_camera_far_plane);

				shared::globals::d3d_device->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
				shared::globals::d3d_device->SetTransform(D3DTS_VIEW, &view_matrix);
				shared::globals::d3d_device->SetTransform(D3DTS_PROJECTION, &proj_matrix);
			}
		}
	}

	IDirect3DVertexShader9* ff_og_vs = nullptr;
	IDirect3DPixelShader9* ff_og_ps = nullptr;
	bool ff_was_modified = false;

	int ff_render_mesh = false;

	struct model_trans
	{
		char pad_0x0000[0x4]; //0x0000
		void* valid1; //0x0004 
		char pad_0x0008[0x4]; //0x0008
		void* valid2; //0x000C 
		char pad_0x0010[0x10]; //0x0010
		D3DXMATRIX* worldTransptr; //0x0020 
		char pad_0x0024[0x1C]; //0x0024
	}; //Size=0x0040


	struct model_view_sub
	{
		char pad_0x0000[0x60]; //0x0000
		D3DXMATRIX modelView; //0x0060 
	};

	struct model_view
	{
		char pad_0x0000[0x8]; //0x0000
		model_view_sub* valid1; //0x0008
	};


	D3DXMATRIX g_model;
	D3DXMATRIX g_proj;

	void pre_drawindexedprim()
	{
		const auto dev = shared::globals::d3d_device;

		auto model_data = reinterpret_cast<model_trans*>(0x10969B0C);
		if (model_data->worldTransptr /*ff_render_mesh*/)
		{
			dev->GetVertexShader(&ff_og_vs);
			dev->SetVertexShader(nullptr);
			ff_was_modified = true;

			dev->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
			dev->SetTransform(D3DTS_VIEW, &g_model);
			dev->SetTransform(D3DTS_PROJECTION, &g_proj);
		}

		//D3DXMATRIX world2 = shared::globals::IDENTITY;

		////bool is_model = false;

		//auto model_data = reinterpret_cast<model_trans*>(0x10969B0C);
		//if (model_data && model_data->valid1 && model_data->worldTransptr)
		//{
		//	/*D3DXMATRIX world*/ world2 = *model_data->worldTransptr;

		//	auto m_view = reinterpret_cast<model_view*>(0x10969B10);
		//	if (m_view && m_view->valid1)
		//	{
		//		//D3DXMATRIX* view = reinterpret_cast<D3DXMATRIX*>(0x109669A0);
		//		D3DXMATRIX* proj = reinterpret_cast<D3DXMATRIX*>(0x10966AE0);

		//		//D3DXMATRIX world2;
		//		//shared::utils::transpose_float4x4(world, world2);

		//		dev->SetTransform(D3DTS_WORLD, &world2);
		//		//dev->SetTransform(D3DTS_VIEW, &m_view->valid1->modelView);
		//		D3DXMATRIX* view = reinterpret_cast<D3DXMATRIX*>(0x109669A0);
		//		dev->SetTransform(D3DTS_VIEW, view);

		//		dev->SetTransform(D3DTS_PROJECTION, proj);

		//		if (model_data->worldTransptr /*ff_render_mesh*/)
		//		{
		//			dev->GetVertexShader(&ff_og_vs);
		//			dev->SetVertexShader(nullptr);
		//			ff_was_modified = true;
		//		}
		//	}

		//	
		//}

		//D3DXMATRIX* view = reinterpret_cast<D3DXMATRIX*>(0x109669A0);
		//D3DXMATRIX* proj = reinterpret_cast<D3DXMATRIX*>(0x10966AE0);

		//dev->SetTransform(D3DTS_WORLD, &world);
		//dev->SetTransform(D3DTS_VIEW, view);
		//dev->SetTransform(D3DTS_PROJECTION, proj);

		//if (model_data->worldTransptr /*ff_render_mesh*/)
		//{
		//	dev->GetVertexShader(&ff_og_vs);
		//	dev->SetVertexShader(nullptr);
		//	ff_was_modified = true;
		//}
	}

	void post_drawindexedprim()
	{
		const auto dev = shared::globals::d3d_device;

		if (ff_was_modified)
		{
			if (ff_og_vs) {
				dev->SetVertexShader(ff_og_vs);
			}
		}
		
		ff_was_modified = false;
		ff_og_vs = nullptr;
	}


	//__declspec(naked) void on_mesh_draw_stub()
	//{
	//	static uint32_t func_addr = 0x79A6D0;
	//	static uint32_t retn_addr = 0x690D5A;
	//	__asm
	//	{
	//		//pushad;
	//		mov		ff_render_mesh, 1;
	//		//popad;

	//		call	func_addr;

	//		//pushad;
	//		mov		ff_render_mesh, 0;
	//		//popad;

	//		jmp		retn_addr;
	//	}
	//}

	

	void grab_matrices(RenderContext* ctx)
	{
		g_model = ctx->modelMatrix;
		g_proj = ctx->projectionMatrix;
		int x = 1;
	}

	__declspec(naked) void on_matrix_calc_stub()
	{
		static uint32_t retn_addr = 0x100E0BF0;
		__asm
		{
			pushad;
			push	ebp;
			call	grab_matrices;
			add		esp, 4;
			popad;

			movss   xmm0, dword ptr[ebp + 0x50];
			jmp		retn_addr;
		}
	}

	void install_signature_patches()
	{

		//shared::common::dinput::init();

		std::uint32_t install_counter = 0u;
		std::uint32_t total_patch_amount = 0u;

		//{
		//	// C7 47 44 00 00 80 3F 5F 5E 8B E5
		//	auto offset = shared::utils::mem::find_pattern_in_module(game::exe_module, "C7 47 44 00 00 80 3F 5F 5E 8B E5", 7);
		//	
		//	if (offset)
		//	{
		//		std::cout << "[SIG] installed view frustum hook @ 0x" << std::uppercase << std::hex << offset << "!\n";
		//		shared::utils::hook captainhook(offset, post_get_view_frustum_stub, HOOK_JUMP);
		//		HOOK_RETN_PLACE(post_get_view_frustum_retn_addr, captainhook.install()->quick()->create_trampoline());
		//		install_counter++;
		//	}

		//	total_patch_amount++;
		//}


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

		// 60F8D3 0xEB
		//shared::utils::hook::set<BYTE>(0x60F8D3, 0xEB);

		//shared::utils::hook::nop(0x690D20, 6)
		//shared::utils::hook(0x690D55, on_mesh_draw_stub, HOOK_JUMP).install()->quick();

		// 100E0BEB
		shared::utils::hook(0x100E0BEB, on_matrix_calc_stub, HOOK_JUMP).install()->quick();

		shared::common::loader::module_loader::register_module(std::make_unique<imgui>());

		MH_EnableHook(MH_ALL_HOOKS);
	}
}
