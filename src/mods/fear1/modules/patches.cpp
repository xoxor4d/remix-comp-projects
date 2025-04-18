#include "std_include.hpp"
#include "patches.hpp"

#include "imgui.hpp"
#include "shared/common/remix.hpp"
#include "shared/common/remix_api.hpp"

// commandline: -dx9 -NOINTRO -windowed

namespace mods::fear1
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

		/* else if (game::rg)
		{
			shared::globals::d3d_device->SetTransform(D3DTS_WORLD, &game::rg->worldMatrix);
			shared::globals::d3d_device->SetTransform(D3DTS_VIEW, &game::rg->viewMatrix);
			shared::globals::d3d_device->SetTransform(D3DTS_PROJECTION, &game::rg->projMatrix);
		} */
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

	/* void grab_renderglob_hk(game::render_glob* ren)
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
	} */

	// --------------------------

	IDirect3DVertexShader9* ff_og_shader = nullptr;
	IDirect3DPixelShader9* ff_og_psshader = nullptr;
	DWORD ff_og_fvf = 0u;

	bool ff_was_viewmodel = false;
	bool ff_was_modified = false;
	//bool ff_use_shader = false;
	bool ff_is_hud = false;
	std::uint8_t ff_skel_bone_count = 24u;

	void setup_prim_ff_rendering()
	{
		const auto& dev = shared::globals::d3d_device;
		dev->GetVertexShader(&ff_og_shader);
		dev->SetVertexShader(nullptr);

		/* if (game::rg) {
			dev->SetTransform(D3DTS_WORLD, &game::rg->worldMatrix);
		} */

		ff_was_modified = true;
	}

	// ------------------


	void matrix3x4_transpose_to_4x4(const shared::float3x4* input, D3DXMATRIX* output, const int count)
	{
		for (int i = 0; i < count; ++i)
		{
			const shared::float3x4& in = input[i];
			D3DXMATRIX& out = output[i];

			// column-major D3DXMATRIX from row-major 3x4
			out._11 = in.m[0][0]; out._12 = in.m[1][0]; out._13 = in.m[2][0]; out._14 = 0.0f;
			out._21 = in.m[0][1]; out._22 = in.m[1][1]; out._23 = in.m[2][1]; out._24 = 0.0f;
			out._31 = in.m[0][2]; out._32 = in.m[1][2]; out._33 = in.m[2][2]; out._34 = 0.0f;
			out._41 = in.m[0][3]; out._42 = in.m[1][3]; out._43 = in.m[2][3]; out._44 = 1.0f;
		}
	}

	void ApplyWorldTransformToBones(const D3DXMATRIX& world_transform, const D3DXMATRIX* local_bones, D3DXMATRIX* out_bones, const int count)
	{
		for (int i = 0; i < count; ++i) {
			D3DXMatrixMultiply(&out_bones[i], &local_bones[i], &world_transform);
		}
	}

	std::set<IDirect3DVertexBuffer9*> fixedVertexBuffers;

	bool patches::pre_drawindexedprim_call(D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
	{
		const auto& dev = shared::globals::d3d_device;

		IDirect3DBaseTexture9* bound_tex = nullptr;
		dev->GetTexture(0, &bound_tex);

		if (!bound_tex) {
			return true;
		}

		const auto& im = imgui::get();
		const auto& patches = patches::get();

#if 1
		if (!ff_is_hud && !im->m_is_rendering &&
			!patches->m_ff_use_shader)
		{
#if 1
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

			DWORD og_fvf = 0u;
			dev->GetFVF(&og_fvf);
			dev->SetFVF(0u);
#endif

			dev->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
			shared::float3x4* to_world = reinterpret_cast<shared::float3x4*>(0x5776F0);

			D3DXMATRIX transworld = {};
			shared::utils::transpose_float3x4_to_d3dxmatrix(*to_world, transworld);

			dev->SetTransform(D3DTS_WORLD, &transworld);

			{
				//if (im->m_dbg_use_fake_camera)
				{
					bool is_hw_skin = false;
					if (ff_skel_bone_count > 0 && decl[5].Stream == 0u && decl[5].Usage == D3DDECLUSAGE_BLENDWEIGHT)
					{
						//dev->SetFVF(D3DFVF_XYZB4 | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_LASTBETA_D3DCOLOR);
						is_hw_skin = true;

						shared::float3x4* mModelObjectNodes = reinterpret_cast<shared::float3x4*>(0x577750);
						D3DXMATRIX boneMatrices[24];
						matrix3x4_transpose_to_4x4(mModelObjectNodes, boneMatrices, 24);

						auto bone_count = 24; 
						if (ff_skel_bone_count < bone_count) {
							bone_count = ff_skel_bone_count;
						}

						D3DXMATRIX finalBoneMatrices[24];
						for (int i = 0; i < bone_count; ++i) {
							D3DXMatrixMultiply(&finalBoneMatrices[i], &boneMatrices[i], &transworld);
						}

						//ApplyWorldTransformToBones(transworld, boneMatrices, finalBoneMatrices, ff_skel_bone_count);

						for (int i = 0; i < bone_count; ++i) {
							dev->SetTransform(D3DTS_WORLDMATRIX(i), &finalBoneMatrices[i]); //&boneMatrices[i]);
						}

						D3DVERTEXELEMENT9 new_decl[] = {
							{ 0, 0,  D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
							{ 0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL,   0 },
							{ 0, 24, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0 },
							{ 0, 32, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TANGENT,  0 },
							{ 0, 44, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_BINORMAL,  0 },
							{ 0, 56, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 },
							{ 0, 60, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0 }, // D3DDECLTYPE_UBYTE4
							D3DDECL_END()
						};

						IDirect3DVertexDeclaration9* pNewDecl = nullptr;
						dev->CreateVertexDeclaration(new_decl, &pNewDecl);
						dev->SetVertexDeclaration(pNewDecl);

						vertex_decl->Release();
						pNewDecl->Release();

						//dev->SetTransform(D3DTS_WORLDMATRIX(bone_count), &transworld);

						dev->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_0WEIGHTS); // 3 bone influences
						dev->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, TRUE);

#if 0
						{
							IDirect3DVertexBuffer9* vb = nullptr; UINT t_stride = 0u, t_offset = 0u;
							dev->GetStreamSource(0, &vb, &t_offset, &t_stride);

							if (!fixedVertexBuffers.contains(vb)) 
							{
								void* src_buffer_data;
								if (SUCCEEDED(vb->Lock(MinVertexIndex * t_stride, NumVertices * t_stride, &src_buffer_data, 0)))
								{
									struct src_vert {
										float pos[3]; float normal[3]; float tc[2]; float tangent[3]; float binormal[3]; D3DCOLOR blendweight; D3DCOLOR blendindices;
									};

									for (UINT i = 0; i < NumVertices; ++i)
									{
										src_vert* src = reinterpret_cast<src_vert*>((DWORD)src_buffer_data + i * t_stride);

										float w[4] = {
											((src->blendweight >> 16) & 0xFF) / 255.0f,
											((src->blendweight >> 8) & 0xFF) / 255.0f,
											((src->blendweight >> 0) & 0xFF) / 255.0f,
											((src->blendweight >> 24) & 0xFF) / 255.0f,
										};

										// Normalize
										float sum = w[0] + w[1] + w[2] + w[3];
										if (sum > 0.0f)
										{
											w[0] /= sum; w[1] /= sum; w[2] /= sum; w[3] /= sum;
										}

										// Repack
										DWORD r = (DWORD)(w[0] * 255.0f);
										DWORD g = (DWORD)(w[1] * 255.0f);
										DWORD b = (DWORD)(w[2] * 255.0f);
										DWORD a = (DWORD)(w[3] * 255.0f);
										src->blendweight = (a << 24) | (r << 16) | (g << 8) | b;

										uint8_t* indices = reinterpret_cast<uint8_t*>(&src->blendindices);
	
									}

									vb->Unlock();
									fixedVertexBuffers.insert(vb);
								}
							}
							vb->Release();
						}
#endif
					}
					else {
						dev->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
					}

					dev->GetVertexShader(&ff_og_shader);

					//if (!is_hw_skin) {
						dev->SetVertexShader(nullptr);
					//}
					

					dev->GetPixelShader(&ff_og_psshader);
					dev->SetPixelShader(nullptr);
					ff_was_modified = true;

					//DWORD og_srcblend = 0u, og_destblend = 0u;
					//dev->GetRenderState(D3DRS_SRCBLEND, &og_srcblend);
					//dev->GetRenderState(D3DRS_DESTBLEND, &og_destblend);

					//// do not allow emissive surfaces - helps with meshes getting emissive when using plasmids
					//if (og_srcblend == D3DBLEND_ONE && og_destblend == D3DBLEND_ONE)
					//{
					//	dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
					//	//return true;
					//}

					//dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
					//dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
					//dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
				}
			}
		}
#endif

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

		ff_skel_bone_count = 24u;
		ff_is_hud = false;
		ff_was_modified = false;
		ff_was_viewmodel = false;
		patches::get()->m_ff_use_shader = false;
	}

	void pre_render_hud_hk()
	{
		ff_is_hud = true;
	}

	__declspec(naked) void pre_render_hud_stub()
	{
		static uint32_t retn_addr = 0x501723;
		__asm
		{
			pushad;
			call	pre_render_hud_hk;
			popad;

			mov     ecx, [esp + 0x14];
			push    ebx;
			jmp		retn_addr;
		}
	}

	// ----

	void pre_mesh_hk(game::mesh_info* info)
	{
		if (info)
		{
			ff_skel_bone_count = info->bone_count;
		}
	}

	__declspec(naked) void pre_mesh_stub()
	{
		static uint32_t retn_addr = 0x50FDD8;
		__asm
		{
			pushad;
			push	eax;
			call	pre_mesh_hk;
			add		esp, 4;
			popad;

			push    edx;
			mov     edx, [esp + 0x1C];
			jmp		retn_addr;
		}
	}


	// ---

	/*__declspec(naked) void pre_sky_stub()
	{
		static uint32_t retn_addr = 0x518A82;
		__asm
		{
			pushad;
			push	eax;
			call	pre_mesh_hk;
			add		esp, 4;
			popad;

			push    edx;
			mov     edx, [esp + 0x1C];
			jmp		retn_addr;
		}
	}*/

	patches::patches()
	{
		p_this = this;
		shared::common::remix_api::initialize(begin_scene_cb, end_scene_cb, present_scene_cb);

		shared::utils::hook(0x50171E, pre_render_hud_stub, HOOK_JUMP).install()->quick();
		shared::utils::hook(0x50FDD3, pre_mesh_stub, HOOK_JUMP).install()->quick();

		// 518A7D
		//shared::utils::hook(0x518A7B, pre_sky_stub, HOOK_JUMP).install()->quick();

		// skip prepass shit
		shared::utils::hook::nop(0x517A18, 6); // nop 6 @ 0x517A18 should be enough - the rest could be removed
		shared::utils::hook::nop(0x517A06, 3); shared::utils::hook::nop(0x517A0B, 5); // 01
		shared::utils::hook::nop(0x5179EC, 3); shared::utils::hook::nop(0x5179F1, 5); // 02
		shared::utils::hook::nop(0x5179D2, 3); shared::utils::hook::nop(0x5179D7, 5); // 03 - flashlight

		shared::utils::hook::nop(0x518B3E, 5); // disable SetTransform's for the sky

		//shared::utils::hook(0x10B91692, grab_renderglob_stub, HOOK_JUMP).install()->quick();

		printf("[Module] patches loaded.\n");
	}
}
