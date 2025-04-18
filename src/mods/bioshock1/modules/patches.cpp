#include "std_include.hpp"
#include "patches.hpp"

#include "imgui.hpp"
#include "shared/common/remix.hpp"
#include "shared/common/remix_api.hpp"

// commandline: -dx9 -NOINTRO -windowed

namespace mods::bioshock1
{
	void begin_scene_cb()
	{
#if 1	// not useful anymore
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

			// Alternative: Use look-at if preferred
			// D3DXVECTOR3 eye(camera_pos[0], camera_pos[1], camera_pos[2]);
			// D3DXVECTOR3 at(0, 0, 0); // Target at origin, adjust as needed
			// D3DXVECTOR3 up(0, 1, 0);
			// D3DXMatrixLookAtLH(&view_matrix, &eye, &at, &up);

			// Construct projection matrix
			D3DXMatrixPerspectiveFovLH(&proj_matrix,
				D3DXToRadian(im->m_dbg_camera_fov), // FOV in radians
				im->m_dbg_camera_aspect,
				im->m_dbg_camera_near_plane,
				im->m_dbg_camera_far_plane);


			shared::globals::d3d_device->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
			shared::globals::d3d_device->SetTransform(D3DTS_VIEW, &view_matrix);
			shared::globals::d3d_device->SetTransform(D3DTS_PROJECTION, &proj_matrix);
		}

		else if (game::rg)
		{
			shared::globals::d3d_device->SetTransform(D3DTS_WORLD, &game::rg->worldMatrix);
			shared::globals::d3d_device->SetTransform(D3DTS_VIEW, &game::rg->viewMatrix);
			shared::globals::d3d_device->SetTransform(D3DTS_PROJECTION, &game::rg->projMatrix);
		}
#endif
	}

	void end_scene_cb()
	{
	}

	void present_scene_cb()
	{
		imgui::get()->on_present();
	}

	// -------

	void grab_renderglob_hk(game::render_glob* ren)
	{
		game::rg = ren;
	}

	__declspec(naked) void grab_renderglob_stub()
	{
		static uint32_t retn_addr = 0x10B91697;
		__asm
		{
			// mov eax, [edi] // eax = &device

			push	esi;
			push	ecx;

			pushad;
			PUSHFD;
			push	ecx;
			call	grab_renderglob_hk;
			add		esp, 4;
			POPFD;
			popad;

			pop		ecx;
			pop		esi;

			mov     esi, ecx;
			mov[ebp - 0x10], esp;
			jmp		retn_addr;
		}
	}

	// --------------------------

	IDirect3DVertexShader9* ff_og_shader = nullptr;
	IDirect3DPixelShader9* ff_og_psshader = nullptr;
	DWORD ff_og_fvf = 0u;

	bool ff_was_viewmodel = false;
	bool ff_was_modified = false;
	//bool ff_use_shader = false;

	void setup_prim_ff_rendering()
	{
		const auto& dev = shared::globals::d3d_device;
		dev->GetVertexShader(&ff_og_shader);
		dev->SetVertexShader(nullptr);

		if (game::rg) {
			dev->SetTransform(D3DTS_WORLD, &game::rg->worldMatrix);
		}

		ff_was_modified = true;
	}

	void pre_dynmodelren_hk([[maybe_unused]] void* model_info)
	{
		setup_prim_ff_rendering();
	}

	__declspec(naked) void pre_dynmodelren_stub()
	{
		static uint32_t retn_addr = 0x10B0584B;
		__asm
		{
			pushad;
			PUSHFD;
			push	edi;
			call	pre_dynmodelren_hk;
			add		esp, 4;
			POPFD;
			popad;

			mov     edx, [edi];
			lea     eax, [esi + 0x1C];
			jmp		retn_addr;
		}
	}

	// 

	void pre_staticmodelren_hk()
	{
		setup_prim_ff_rendering();
	}

	__declspec(naked) void pre_staticmodelren_stub()
	{
		static uint32_t retn_addr = 0x10B0649A;
		__asm
		{
			pushad;
			call	pre_staticmodelren_hk;
			popad;

			mov		[esi + 0x38], eax;
			mov     edx, [ebp + 0];
			jmp		retn_addr;
		}
	}

	//

	void pre_batchedmodelren_hk()
	{
		setup_prim_ff_rendering();
	}

	__declspec(naked) void pre_batchedmodelren_stub()
	{
		static uint32_t retn_addr = 0x10B3A66D;
		__asm
		{
			pushad;
			call	pre_batchedmodelren_hk;
			popad;

			push	ecx;
			movzx   ecx, word ptr[eax + 8];
			jmp		retn_addr;
		}
	}

	//

	void pre_bspren_hk()
	{
		setup_prim_ff_rendering();
	}

	__declspec(naked) void pre_bspren_stub()
	{
		static uint32_t retn_addr = 0x10B3AD30;
		__asm
		{
			pushad;
			call	pre_bspren_hk;
			popad;

			push	eax;
			mov     eax, [esp + 0x20];
			jmp		retn_addr;
		}
	}


	// 

	void pre_skyren_hk()
	{
		patches::get()->m_ff_use_shader = true;
	}

	__declspec(naked) void pre_skyren_stub()
	{
		static uint32_t retn_addr = 0x10B39932;
		__asm
		{
			pushad;
			call	pre_skyren_hk;
			popad;

			mov     eax, [edi];
			mov     eax, [eax];
			push    ecx;
			jmp		retn_addr;
		}
	}


	void post_get_view_frustum_hk(game::FConvexVolume* frustum)
	{
		for (auto i = 0u; i < 5; i++) {
			frustum->BoundingPlanes[i].dist += 10000.0f;
		}
	}

	__declspec(naked) void post_get_view_frustum_stub()
	{
		static uint32_t retn_addr = 0x10AE27F3;
		__asm
		{
			pushad;
			push	eax;
			call	post_get_view_frustum_hk;
			add		esp, 4;
			popad;

			mov		[ebx], eax;
			mov		[ebx + 4], ebp;
			jmp		retn_addr;
		}
	}



	// ------------------

	bool patches::pre_drawindexedprim_call()
	{
		const auto& dev = shared::globals::d3d_device;

		// bsp
		/*if (game::rg->worldMatrix.m[0][0] == 1.0f &&
			game::rg->worldMatrix.m[1][1] == 1.0f &&
			game::rg->worldMatrix.m[2][2] == 1.0f &&
			game::rg->worldMatrix.m[3][0] == 0.0f &&
			game::rg->worldMatrix.m[3][1] == 0.0f &&
			game::rg->worldMatrix.m[3][2] == 0.0f)
		{
			patches::get()->m_ff_use_shader = true;
			dev->SetTransform(D3DTS_WORLD, &game::rg->worldMatrix);
		}*/

		IDirect3DBaseTexture9* bound_tex = nullptr;
		dev->GetTexture(0, &bound_tex);

		if (!bound_tex) {
			return true;
		}

		const auto& im = imgui::get();
		const auto& patches = patches::get();

		if (!im->m_is_rendering && 
			!patches->m_ff_use_shader && game::rg)
		{
			{
				// only valid on 16:9 - TODO
				if (shared::utils::float_equal(game::rg->projMatrix.m[0][0], 0.750000060f))
				{
					dev->GetVertexShader(&ff_og_shader);
					dev->SetVertexShader(nullptr);
					dev->GetPixelShader(&ff_og_psshader);
					dev->SetPixelShader(nullptr);
					dev->SetTransform(D3DTS_WORLD, &game::rg->worldMatrix);
					dev->SetTransform(D3DTS_VIEW, &game::rg->viewMatrix);
					dev->SetTransform(D3DTS_PROJECTION, &game::rg->projMatrix);
					ff_was_modified = true;

					DWORD og_srcblend = 0u, og_destblend = 0u;
					dev->GetRenderState(D3DRS_SRCBLEND, &og_srcblend);
					dev->GetRenderState(D3DRS_DESTBLEND, &og_destblend);

					// do not allow emissive surfaces - helps with meshes getting emissive when using plasmids
					if (og_srcblend == D3DBLEND_ONE && og_destblend == D3DBLEND_ONE)
					{
						dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
						//return true;
					}

					//dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
					//dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
					//dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);

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
				else // viewmodel
				{
					dev->GetVertexShader(&ff_og_shader);
					dev->SetVertexShader(nullptr);
					dev->GetPixelShader(&ff_og_psshader);
					dev->SetPixelShader(nullptr);
					dev->SetTransform(D3DTS_WORLD, &game::rg->worldMatrix);
					dev->SetTransform(D3DTS_VIEW, &game::rg->viewMatrix);
					dev->SetTransform(D3DTS_PROJECTION, &game::rg->projMatrix);

					dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
					dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
					dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);

					if (im->m_viewmodel_use_custom_proj)
					{
						D3DXMATRIX im_proj;

						// Construct projection matrix
						D3DXMatrixPerspectiveFovLH(&im_proj,
							D3DXToRadian(im->m_viewmodel_proj_fov),
							im->m_viewmodel_proj_aspect,
							im->m_viewmodel_proj_near_plane,
							im->m_viewmodel_proj_far_plane);

						dev->SetTransform(D3DTS_PROJECTION, &im_proj);
					}

					ff_was_modified = true;
				}

				//else // viewmodel
				//{
				//	dev->GetVertexShader(&ff_og_shader);
				//	dev->SetVertexShader(nullptr);

				//	float zNear = 1.0f;
				//	float zFar = 100.0f;

				//	D3DXMATRIX cproj = game::rg->projMatrix;
				//	cproj.m[2][2] = zFar / (zFar - zNear);
				//	cproj.m[3][2] = -zNear * zFar / (zFar - zNear);
				//	dev->SetTransform(D3DTS_PROJECTION, &game::rg->projMatrix);

				//	ff_was_modified = true;
				//	dev->SetTransform(D3DTS_WORLD, &game::rg->worldMatrix);
				//	dev->SetTransform(D3DTS_VIEW, &game::rg->viewMatrix);
				//	//dev->SetTransform(D3DTS_PROJECTION, &game::rg->projMatrix);
				//}

				//dev->SetTransform(D3DTS_WORLD, &game::rg->viewMatrix);
				//dev->SetTransform(D3DTS_WORLD, &game::rg->projMatrix);
			}
		}

		dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0u);
		return false;
	}

	// called right after device->DrawIndexedPrimitive
	void patches::post_drawindexedprim_call()
	{
		const auto& dev = shared::globals::d3d_device;
		if (ff_was_modified)
		{
			dev->SetVertexShader(ff_og_shader);
			dev->SetPixelShader(ff_og_psshader);
		}

		ff_was_modified = false;
		ff_was_viewmodel = false;
		patches::get()->m_ff_use_shader = false;
	}

	patches::patches()
	{
		p_this = this;
		shared::common::remix_api::initialize(begin_scene_cb, end_scene_cb, present_scene_cb);

		shared::utils::hook(0x10B91692, grab_renderglob_stub, HOOK_JUMP).install()->quick();

		// dynamic models
		//shared::utils::hook::set(0x10B0558D, 0xE9, 0xD1, 0x02, 0x0, 0x0, 0x90); // skip below call that draws dynamic meshes
		//shared::utils::hook(0x10B05846, pre_dynmodelren_stub, HOOK_JUMP).install()->quick();


		// static models
		//shared::utils::hook::set<BYTE>(0x10B06438, 0xEB); // skip call that draws static? models
		//shared::utils::hook::nop(0x10B06494, 6); shared::utils::hook(0x10B06494, pre_staticmodelren_stub, HOOK_JUMP).install()->quick();


		// batched models
		//shared::utils::hook::nop(0x10B3A668, 1);  shared::utils::hook::nop(0x10B3A66D, 1); shared::utils::hook::nop(0x10B3A676, 6);  shared::utils::hook::nop(0x10B3A67E, 2); // skip call
		//shared::utils::hook(0x10B3A668, pre_batchedmodelren_stub, HOOK_JUMP).install()->quick();

		// bsp ... could also hook somewhere here (0x10B8DDF4) to FF all batched meshes at once?
		//shared::utils::hook::set(0x10B3A765, 0xE9, 0x6D, 0x06, 0x0, 0x0, 0x90); // skip call that draws bsp
		//shared::utils::hook(0x10B3AD2B, pre_bspren_stub, HOOK_JUMP).install()->quick();


		// skip call that draws videos / menu background ?
		//shared::utils::hook::set(0x10B1A0BE, 0xE9, 0x84, 0x02, 0x0, 0x0, 0x90);


		// skybox
		//shared::utils::hook::set(0x10B397C9, 0xE9, 0xCB, 0x01, 0x0, 0x0, 0x90); // skip call that draws the skybox
		shared::utils::hook(0x10B3992D, pre_skyren_stub, HOOK_JUMP).install()->quick();


		// >> do not draw untextured pre-draw meshes (occlusion?)

		// 01 -> removes some walls that shouldnt be removed
		//shared::utils::hook::nop(0x10AE7096, 3); 
		//shared::utils::hook::nop(0x10AE70A0, 5);

		// 02
		shared::utils::hook::nop(0x10B39CDB, 7);
		shared::utils::hook::nop(0x10B39CE2, 5);




		// disable frustum culling by setting num frustum planes to 0 (FLevelSceneNode::GetViewFrustum)
		// 0x10A55322 + 6 -> 0x0 (mov [esi+00000400],00000005) <<
		// 0x10A55339 + 6 -> 0x0 (mov [esi+00000400],00000004)
		//shared::utils::hook::set<BYTE>(0x10A55322 + 6, 0x00);
		//shared::utils::hook::set<BYTE>(0x10A55339 + 6, 0x00);

		// skip call to GetViewFrustum
		//shared::utils::hook::set<BYTE>(0x10B3099C, 0xEB);

		// this fixes frustum culling
		shared::utils::hook(0x10AE27EE, post_get_view_frustum_stub, HOOK_JUMP).install()->quick();

		

		// FIX VERTEX EXPLOSIONS!
		shared::utils::hook::set(0x10AE8D25, 0xE9, 0x29, 0x01, 0x0, 0x0, 0x90);

		// Notes
		/*
		 * Not calling 0x10AE90B8 (RenderScene) hides the viewmodel (alpha 0) and makes it emissive
		 */

		printf("[Module] patches loaded.\n");
	}
}
