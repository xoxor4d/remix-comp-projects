#include "std_include.hpp"
#include "patches.hpp"

#include "imgui.hpp"
#include "shared/common/remix_api.hpp"

namespace mods::anno1404
{
	void begin_scene_cb()
	{

		
		D3DXMATRIX view_matrix
		(
			1.0f, 0.0f, 0.0f, 0.0f,    
			0.0f, 0.447f, 0.894f, 0.0f,
			0.0f, -0.894f, 0.447f, 0.0f,
			0.0f, 100.0f, -50.0f, 1.0f
		);

		D3DXMATRIX proj_matrix
		(
			1.359f, 0.0f, 0.0f, 0.0f,
			0.0f, 2.414f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.001f, 1.0f,
			0.0f, 0.0f, -1.0f, 0.0f
		);

		const auto& im = imgui::get();

		if (im->m_dbg_use_fake_camera)
		{
			// Construct view matrix
			D3DXMATRIX rotation, translation;
			D3DXMatrixRotationYawPitchRoll(&rotation,
				D3DXToRadian(im->m_dbg_camera_yaw),   // Yaw in radians
				D3DXToRadian(im->m_dbg_camera_pitch), // Pitch in radians
				0.0f);                      // No roll for simplicity

			D3DXMatrixTranslation(&translation,
				-im->m_dbg_camera_pos[0], // Negate for camera (moves world opposite)
				-im->m_dbg_camera_pos[1],
				-im->m_dbg_camera_pos[2]);

			D3DXMatrixMultiply(&view_matrix, &rotation, &translation);

			// Construct projection matrix
			D3DXMatrixPerspectiveFovLH(&proj_matrix,
				D3DXToRadian(im->m_dbg_camera_fov), // FOV in radians
				im->m_dbg_camera_aspect,
				im->m_dbg_camera_near_plane,
				im->m_dbg_camera_far_plane);


			shared::globals::d3d_device->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
			shared::globals::d3d_device->SetTransform(D3DTS_VIEW, &view_matrix);
			shared::globals::d3d_device->SetTransform(D3DTS_PROJECTION, &proj_matrix);

			// engine_interface_obj
			auto engine = reinterpret_cast<engine_interface_obj*>(0x1187E8C);

			// 0x5448B5 + 0xAB8 -> 0x164 ?
			//D3DXMATRIX* view_game = reinterpret_cast<D3DXMATRIX*>(0x115B4E6C); 
			shared::globals::d3d_device->SetTransform(D3DTS_VIEW, &engine->ptr->towards_cam_ptr->towards_cam2_ptr->cam_struct_ptr->cam->viewMatrix); 
			//shared::globals::d3d_device->SetTransform(D3DTS_VIEW, view_game);

			// 0x5448B5 + 0xAB8 -> 0x1A4 ?
			//D3DXMATRIX* proj_game = reinterpret_cast<D3DXMATRIX*>(0x115B4EAC);
			//engine->ptr->towards_cam_ptr->towards_cam2_ptr->cam_struct_ptr->cam->projMatrix.m[2][3] = 1.0f;
			//engine->ptr->towards_cam_ptr->towards_cam2_ptr->cam_struct_ptr->cam->projMatrix.m[3][3] = 0.0f;
			shared::globals::d3d_device->SetTransform(D3DTS_PROJECTION, &engine->ptr->towards_cam_ptr->towards_cam2_ptr->cam_struct_ptr->cam->projMatrix);
			//shared::globals::d3d_device->SetTransform(D3DTS_PROJECTION, proj_game);

			shared::globals::d3d_device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0u);
		}
	}

	void end_scene_cb()
	{

	}

	void present_scene_cb()
	{
		imgui::get()->on_present();
	}

	bool m_ff_instance = false;
	std::uint32_t m_ff_instance_count = 1u;

	bool patches::pre_drawindexedprim_call(IDirect3DDevice9* unhooked_device, D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
	{
		const auto p = patches::get();
		//const auto& im = imgui::get();
		//const auto& dev = shared::globals::d3d_device;

		if (/*m_ff_instance_count > 1 &&*/ !p->m_cInstanceBuffer.empty() && m_ff_instance && PrimitiveType == D3DPT_TRIANGLELIST)
		{
			unhooked_device->GetVertexShader(&p->m_ff_og_shader);
			unhooked_device->SetVertexShader(nullptr);
			//dev->SetPixelShader(nullptr);

			for (auto i = 0u; i < m_ff_instance_count; ++i) 
			{
				auto idx = i * 3;
				const auto inst_data = reinterpret_cast<mesh_inst_data*>(&p->m_cInstanceBuffer[idx]);


				D3DXMATRIX temp_wrld = shared::globals::IDENTITY;
				temp_wrld.m[3][0] = inst_data->pos.x;
				temp_wrld.m[3][1] = inst_data->pos.y;
				temp_wrld.m[3][2] = inst_data->pos.z;

				unhooked_device->SetTransform(D3DTS_WORLD, &temp_wrld); //&shared::globals::IDENTITY);//&instanceData[i]);

				// Draw using original buffers and parameters
				//device->SetStreamSource(0, vb, offset, stride);
				//device->SetIndices(ib);
				unhooked_device->DrawIndexedPrimitive(PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices / m_ff_instance_count, startIndex, primCount / m_ff_instance_count);
			}

			//p->m_ff_was_modified = true;

			unhooked_device->SetVertexShader(p->m_ff_og_shader);
			m_ff_instance = false;

			return true; // skip og draw call
		}

#if 0
		if (!im->m_is_rendering && !p->m_ff_use_shader)
		{
			{
				// only valid on 16:9 - TODO
				//if (shared::utils::float_equal(game::rg->projMatrix.m[0][0], 0.750000060f))
				{
					dev->GetVertexShader(&p->m_ff_og_shader);
					dev->SetVertexShader(nullptr);
					//dev->GetPixelShader(&p->m_ff_og_psshader);
					//dev->SetPixelShader(nullptr);

					//dev->SetTransform(D3DTS_WORLD, &game::rg->worldMatrix);
					//dev->SetTransform(D3DTS_VIEW, &game::rg->viewMatrix);
					//dev->SetTransform(D3DTS_PROJECTION, &game::rg->projMatrix);
					p->m_ff_was_modified = true;

#if 0
					IDirect3DVertexDeclaration9* vertex_decl = nullptr;
					dev->GetVertexDeclaration(&vertex_decl);

					enum d3ddecltype : BYTE
					{
						D3DDECLTYPE_FLOAT1 = 0,		// 1D float expanded to (value, 0., 0., 1.)
						D3DDECLTYPE_FLOAT2 = 1,		// 2D float expanded to (value, value, 0., 1.)
						D3DDECLTYPE_FLOAT3 = 2,		// 3D float expanded to (value, value, value, 1.)
						D3DDECLTYPE_FLOAT4 = 3,		// 4D float
						D3DDECLTYPE_D3DCOLOR = 4,	// 4D packed unsigned bytes mapped to 0. to 1. range

						// Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
						D3DDECLTYPE_UBYTE4 = 5,		// 4D unsigned byte
						D3DDECLTYPE_SHORT2 = 6,		// 2D signed short expanded to (value, value, 0., 1.)
						D3DDECLTYPE_SHORT4 = 7,		// 4D signed short

						// The following types are valid only with vertex shaders >= 2.0
						D3DDECLTYPE_UBYTE4N = 8,	// Each of 4 bytes is normalized by dividing to 255.0
						D3DDECLTYPE_SHORT2N = 9,	// 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
						D3DDECLTYPE_SHORT4N = 10,	// 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
						D3DDECLTYPE_USHORT2N = 11,  // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
						D3DDECLTYPE_USHORT4N = 12,  // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
						D3DDECLTYPE_UDEC3 = 13,		// 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
						D3DDECLTYPE_DEC3N = 14,		// 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
						D3DDECLTYPE_FLOAT16_2 = 15,	// Two 16-bit floating point values, expanded to (value, value, 0, 1)
						D3DDECLTYPE_FLOAT16_4 = 16,	// Four 16-bit floating point values
						D3DDECLTYPE_UNUSED = 17,	// When the type field in a decl is unused.
					};
					enum d3ddecluse : BYTE
					{
						D3DDECLUSAGE_POSITION = 0,
						D3DDECLUSAGE_BLENDWEIGHT,   // 1
						D3DDECLUSAGE_BLENDINDICES,  // 2
						D3DDECLUSAGE_NORMAL,        // 3
						D3DDECLUSAGE_PSIZE,         // 4
						D3DDECLUSAGE_TEXCOORD,      // 5
						D3DDECLUSAGE_TANGENT,       // 6
						D3DDECLUSAGE_BINORMAL,      // 7
						D3DDECLUSAGE_TESSFACTOR,    // 8
						D3DDECLUSAGE_POSITIONT,     // 9
						D3DDECLUSAGE_COLOR,         // 10
						D3DDECLUSAGE_FOG,           // 11
						D3DDECLUSAGE_DEPTH,         // 12
						D3DDECLUSAGE_SAMPLE,        // 13
					};
					struct d3dvertelem
					{
						WORD Stream;		// Stream index
						WORD Offset;		// Offset in the stream in bytes
						d3ddecltype Type;	// Data type
						BYTE Method;		// Processing method
						d3ddecluse Usage;	// Semantics
						BYTE UsageIndex;	// Semantic index
					};

					d3dvertelem decl[MAX_FVF_DECL_SIZE]; UINT numElements = 0;
					vertex_decl->GetDeclaration((D3DVERTEXELEMENT9*)decl, &numElements);
					int x = 0;
#endif
				}
			}
		}
#endif

		return false;
	}

	// called right after device->DrawIndexedPrimitive
	void patches::post_drawindexedprim_call()
	{
		const auto p = patches::get();
		const auto& dev = shared::globals::d3d_device;

		if (p->m_ff_was_modified)
		{
			dev->SetVertexShader(p->m_ff_og_shader);
			//dev->SetPixelShader(p->m_ff_og_psshader);
		}

		p->m_ff_was_modified = false;
		p->m_ff_use_shader = false;
	}





	void pre_model_render_hk()
	{
		const auto p = patches::get();
		const auto& dev = shared::globals::d3d_device;

		dev->GetVertexShader(&p->m_ff_og_shader);
		dev->SetVertexShader(nullptr);

		//dev->GetPixelShader(&p->m_ff_og_psshader);
		//dev->SetPixelShader(nullptr);

		p->m_ff_was_modified = true;
	}

	__declspec(naked) void pre_model_render_stub()
	{
		static uint32_t retn_addr = 0x9681A6;
		__asm
		{
			pushad;
			call	pre_model_render_hk;
			popad;

			push    edx;
			mov     edx, [esp + 0x28];
			jmp		retn_addr;
		}
	}

	void post_model_render_hk()
	{
		const auto p = patches::get();
		const auto& dev = shared::globals::d3d_device;

		if (p->m_ff_was_modified)
		{
			dev->SetVertexShader(p->m_ff_og_shader);
			//dev->SetPixelShader(p->m_ff_og_psshader);
			p->m_ff_was_modified = false;
		}
	}

	__declspec(naked) void post_model_render_stub()
	{
		static uint32_t retn_addr = 0x9681B3;
		__asm
		{
			pushad;
			call	post_model_render_hk;
			popad;

			add     ebx, 1;
			cmp     ebx, [esp + 0x24];
			jmp		retn_addr;
		}
	}


	// ---
	void pre_hud_render_hk()
	{
		patches::get()->m_ff_use_shader = true;
	}

	__declspec(naked) void pre_hud_render_stub()
	{
		static uint32_t retn_addr = 0x8E0325;
		__asm
		{
			pushad;
			call	pre_hud_render_hk;
			popad;

			push    esi;
			mov     esi, ecx;
			mov     eax, esi;
			jmp		retn_addr;
		}
	}

	void post_hud_render_hk()
	{
		patches::get()->m_ff_use_shader = false;
	}

	__declspec(naked) void post_hud_render_stub()
	{
		__asm
		{
			pushad;
			call	post_hud_render_hk;
			popad;

			pop     esi
			retn;
		}
	}

	// ----

	void pre_instance_render_hk(const int num_objects)
	{
		m_ff_instance = true;
		m_ff_instance_count = num_objects;
	}

	__declspec(naked) void pre_instance_render_stub()
	{
		static uint32_t retn_addr = 0x9DCCA0;
		__asm
		{
			push    ebp;
			mov     ebp, [esp + 0x1C]; // numobj

			pushad;
			push	ebp;
			call	pre_instance_render_hk;
			add		esp, 4;
			popad;

			jmp		retn_addr;
		}
	}


	void post_instance_render_hk()
	{
		m_ff_instance = false;
	}

	__declspec(naked) void post_instance_render_stub()
	{
		static uint32_t retn_addr = 0x9DCCB8;
		__asm
		{
			call    edx; // func towards drawindexedpprim

			pushad;
			call	post_instance_render_hk;
			popad;

			cmp     edi, 1;
			jmp		retn_addr;
		}
	}

	patches::patches()
	{
		p_this = this;
		shared::common::remix_api::initialize(begin_scene_cb, end_scene_cb, present_scene_cb);

		// disable intro
		shared::utils::hook::set(0x485C8C, 0xE9, 0xB8, 0x03, 0x0, 0x0, 0x90);


		shared::utils::hook(0x9681A1, pre_model_render_stub, HOOK_JUMP).install()->quick();

		shared::utils::hook::nop(0x9681AC, 7);
		shared::utils::hook(0x9681AC, post_model_render_stub, HOOK_JUMP).install()->quick();

		// 8E0320
		shared::utils::hook(0x8E0320, pre_hud_render_stub, HOOK_JUMP).install()->quick();
		shared::utils::hook(0x8E0330, post_hud_render_stub, HOOK_JUMP).install()->quick();

		// 9DCC81
		shared::utils::hook(0x9DCC9B, pre_instance_render_stub, HOOK_JUMP).install()->quick();
		shared::utils::hook(0x9DCCB3, post_instance_render_stub, HOOK_JUMP).install()->quick();

		printf("[Module] patches loaded.\n");
	}
}
