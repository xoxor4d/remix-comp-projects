#include "std_include.hpp"
#include "patches.hpp"

#include "imgui.hpp"
#include "shared/common/remix_api.hpp"
#include "shared/imgui/imgui_helper.hpp"

namespace mods::mirrorsedge
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

		//if (im->m_dbg_use_fake_camera)
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

		//else if (game::rg)
		//{
		//	/*D3DXMATRIX pProjection, pView;
		//	shared::globals::d3d_device->GetVertexShaderConstantF(0, pProjection, 4);
		//	shared::globals::d3d_device->GetVertexShaderConstantF(14, pView, 4);*/

		//	shared::globals::d3d_device->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
		//	shared::globals::d3d_device->SetTransform(D3DTS_VIEW, &game::rg->viewMatrix);

		//	/*D3DXMATRIX mod_proj = game::rg->projMatrix;
		//	mod_proj.m[0][0] = 0.97741f;
		//	mod_proj.m[1][1] = 1.73763f;*/

		//	shared::globals::d3d_device->SetTransform(D3DTS_PROJECTION, &game::rg->projMatrix); //&mod_proj);
		//}
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
	bool ff_was_modified = false;
	bool ff_use_shader = false;

	void setup_prim_ff_rendering()
	{
		const auto& dev = shared::globals::d3d_device;
		dev->GetVertexShader(&ff_og_shader);
		dev->SetVertexShader(nullptr);

		D3DXMATRIX l2w = shared::globals::IDENTITY;
		shared::globals::d3d_device->GetVertexShaderConstantF(5, l2w, 4);
		//shared::globals::d3d_device->SetTransform(D3DTS_WORLD, &l2w);

		if (game::rg) {
			dev->SetTransform(D3DTS_WORLD, &game::rg->worldMatrix);
		}

		ff_was_modified = true;
	}


	void pre_bspren_hk()
	{
		const auto& dev = shared::globals::d3d_device;

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

		//d3dvertelem decl[MAX_FVF_DECL_SIZE]; UINT numElements = 0;
		//vertex_decl->GetDeclaration((D3DVERTEXELEMENT9*)decl, &numElements);


		D3DVERTEXELEMENT9 decl[MAXD3DDECLLENGTH]; UINT numElements;
		vertex_decl->GetDeclaration(decl, &numElements);

		decl[4].Type = D3DDECLTYPE_FLOAT2;

		IDirect3DVertexDeclaration9* newDecl = nullptr;
		dev->CreateVertexDeclaration(decl, &newDecl);

		dev->SetVertexDeclaration(newDecl);
		newDecl->Release();


		dev->GetVertexDeclaration(&vertex_decl);
		d3dvertelem read_decl[MAX_FVF_DECL_SIZE];
		vertex_decl->GetDeclaration((D3DVERTEXELEMENT9*)read_decl, &numElements);

		//dev->SetFVF(D3DFVF_XYZB5 | D3DFVF_DIFFUSE | D3DFVF_NORMAL | D3DFVF_TEX3);
		setup_prim_ff_rendering(); 
	}

	__declspec(naked) void pre_bspren_stub()
	{
		static uint32_t retn_addr = 0xCE1585;
		__asm
		{
			pushad;
			call	pre_bspren_hk;
			popad;

			mov     edx, [esi];
			mov     eax, [ebx + 4];
			jmp		retn_addr;
		}
	}

	// ------------------

	void patches::pre_drawindexedprim_call()
	{
		const auto& dev = shared::globals::d3d_device; 
		//if (!ff_use_shader && game::rg)
		//{
		//	/*if (game::rg->projMatrix.m[3][2] < -10.0015259)  
		//	{
		//		int breakme = 0;
		//	}
		//	else*/
		//	{
		//		D3DXMATRIX local2world = shared::globals::IDENTITY;
		//		dev->GetVertexShaderConstantF(5, local2world, 4);

		//		dev->GetVertexShader(&ff_og_shader);
		//		dev->SetVertexShader(nullptr);
		//		dev->SetTransform(D3DTS_WORLD, &local2world);//&game::rg->worldMatrix);

		//		//dev->SetTransform(D3DTS_WORLD, &game::rg->viewMatrix);
		//		//dev->SetTransform(D3DTS_WORLD, &game::rg->projMatrix);
		//		ff_was_modified = true;
		//	}
		//}

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

		/*dev->GetVertexDeclaration(&vertex_decl);
		d3dvertelem read_decl[MAX_FVF_DECL_SIZE]; UINT numElements;
		vertex_decl->GetDeclaration((D3DVERTEXELEMENT9*)read_decl, &numElements);*/

		D3DVERTEXELEMENT9 decl[MAXD3DDECLLENGTH]; UINT numElements;
		vertex_decl->GetDeclaration(decl, &numElements);


		d3dvertelem rdecl[MAXD3DDECLLENGTH];
		memcpy(&rdecl, &decl, sizeof(decl));

		if (decl[4].Type == D3DDECLTYPE_FLOAT16_2)
		{
			decl[4].Type = D3DDECLTYPE_FLOAT2;
			decl[5].Type = D3DDECLTYPE_FLOAT2;
			decl[6].Type = D3DDECLTYPE_FLOAT2;
			decl[7].Type = D3DDECLTYPE_FLOAT2;
		}

		IDirect3DVertexDeclaration9* newDecl = nullptr;
		dev->CreateVertexDeclaration(decl, &newDecl);

		dev->SetVertexDeclaration(newDecl);
		newDecl->Release();


		dev->GetVertexDeclaration(&vertex_decl);
		d3dvertelem read_decl[MAX_FVF_DECL_SIZE];
		vertex_decl->GetDeclaration((D3DVERTEXELEMENT9*)read_decl, &numElements);

		setup_prim_ff_rendering();
	}

	// called right after device->DrawIndexedPrimitive
	void patches::post_drawindexedprim_call()
	{
		const auto& dev = shared::globals::d3d_device;
		if (ff_was_modified)
		{
			dev->SetVertexShader(ff_og_shader);
			dev->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
			dev->SetFVF(0);
		}

		ff_was_modified = false;
		ff_use_shader = false;
	}

	patches::patches()
	{
		p_this = this;
		shared::common::remix_api::initialize(begin_scene_cb, end_scene_cb, present_scene_cb);

		//shared::utils::hook(0x10B91692, grab_renderglob_stub, HOOK_JUMP).install()->quick();

		shared::utils::hook(0xCE1580, pre_bspren_stub, HOOK_JUMP).install()->quick();
		//  CE15FA

		printf("[Module] patches loaded.\n");
	}
}
