#include "std_include.hpp"
#include "patches.hpp"

#include "imgui.hpp"
#include "shared/common/remix.hpp"
#include "shared/common/remix_api.hpp"

namespace mods::fear1
{
	namespace tex_addons
	{
		LPDIRECT3DTEXTURE9 sky_gray_up;
	}

	void patches::init_texture_addons(bool release)
	{
		if (release)
		{
			if (tex_addons::sky_gray_up) tex_addons::sky_gray_up->Release();
			return;
		}

		if (!m_textures_initialized)
		{
			const auto dev = shared::globals::d3d_device;
			D3DXCreateTextureFromFileA(dev, "fear1-rtx\\textures\\graycloud_up.jpg", &tex_addons::sky_gray_up);

			m_textures_initialized = true;
		}
	}

	// -----

	void begin_scene_cb()
	{
	}

	void end_scene_cb()
	{
	}

	void present_scene_cb()
	{
	}

	// --------------------------

	IDirect3DVertexShader9* ff_og_shader = nullptr;
	IDirect3DPixelShader9* ff_og_psshader = nullptr;

	bool ff_was_viewmodel = false;
	bool ff_was_modified = false;
	bool ff_is_sky = false;
	bool ff_is_hud = false;
	std::uint8_t ff_skel_bone_count = 24u;

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

	// used for bulletholes
	bool patches::pre_drawprim_call()
	{
		const auto& dev = shared::globals::d3d_device;

		IDirect3DBaseTexture9* bound_tex = nullptr;
		dev->GetTexture(0, &bound_tex);

		if (!bound_tex) {
			return true;
		}

		const auto& im = imgui::get();
		//const auto& patches = patches::get();

		if (!im->m_is_rendering)
		{
			shared::float3x4* to_world = reinterpret_cast<shared::float3x4*>(0x5776F0);

			D3DXMATRIX transworld = {};
			shared::utils::transpose_float3x4_to_d3dxmatrix(*to_world, transworld);
			dev->SetTransform(D3DTS_WORLD, &transworld);

			dev->GetVertexShader(&ff_og_shader);
			dev->SetVertexShader(nullptr);
			dev->GetPixelShader(&ff_og_psshader);
			dev->SetPixelShader(nullptr);

			dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0u);
			ff_was_modified = true;
		}

		return false;
	}

	void patches::post_drawprim_call()
	{
		if (ff_was_modified)
		{
			const auto& dev = shared::globals::d3d_device;
			if (ff_was_modified)
			{
				dev->SetVertexShader(ff_og_shader);
				dev->SetPixelShader(ff_og_psshader);
			}

			ff_was_modified = false;
		}
	}


	bool patches::pre_drawindexedprim_call(
		[[maybe_unused]] D3DPRIMITIVETYPE PrimitiveType, 
		[[maybe_unused]] INT BaseVertexIndex, 
		[[maybe_unused]] UINT MinVertexIndex, 
		[[maybe_unused]] UINT NumVertices, 
		[[maybe_unused]] UINT startIndex, 
		[[maybe_unused]] UINT primCount)
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
		if (!ff_is_sky && !ff_is_hud && !im->m_is_rendering &&
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

			//DWORD og_fvf = 0u;
			//dev->GetFVF(&og_fvf);
			//dev->SetFVF(0u);
#endif

			//dev->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
			shared::float3x4* to_world = reinterpret_cast<shared::float3x4*>(0x5776F0);

			D3DXMATRIX transworld = {};
			shared::utils::transpose_float3x4_to_d3dxmatrix(*to_world, transworld);
			dev->SetTransform(D3DTS_WORLD, &transworld);

			{
				//if (im->m_dbg_use_fake_camera)
				{
					//bool is_hw_skin = false;
					if (ff_skel_bone_count > 0 && decl[5].Stream == 0u && decl[5].Usage == D3DDECLUSAGE_BLENDWEIGHT)
					{
						//is_hw_skin = true;

						// get bone matrices and transpose to 4x4
						shared::float3x4* mModelObjectNodes = reinterpret_cast<shared::float3x4*>(0x577750);
						D3DXMATRIX boneMatrices[24];
						matrix3x4_transpose_to_4x4(mModelObjectNodes, boneMatrices, 24);

						auto bone_count = 24; 
						if (ff_skel_bone_count < bone_count) {
							bone_count = ff_skel_bone_count;
						}

						// add world translation to local bones because we cant use SetTransform WORLD when skinning
						D3DXMATRIX finalBoneMatrices[24];
						for (int i = 0; i < bone_count; ++i) {
							D3DXMatrixMultiply(&finalBoneMatrices[i], &boneMatrices[i], &transworld);
						}

						// set bone transforms
						for (int i = 0; i < bone_count; ++i) {
							dev->SetTransform(D3DTS_WORLDMATRIX(i), &finalBoneMatrices[i]); //&boneMatrices[i]);
						}

#if 0
						// this is not needed? - todo
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
#endif

						dev->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_0WEIGHTS); // only 0 works?
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
		if (info) {
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

	// testing
	void pre_sky_kh()
	{
		//ff_is_sky = true;

		/*patches::get()->init_texture_addons();

		const auto dev = shared::globals::d3d_device;
		dev->SetTexture(0, tex_addons::sky_gray_up);

		dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
		dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);*/
	}

	__declspec(naked) void pre_sky_stub()
	{
		static uint32_t retn_addr = 0x518A80;
		__asm
		{
			pushad;
			call	pre_sky_kh;
			popad;

			test    eax, eax;
			push    esi;
			mov     esi, ecx;
			jmp		retn_addr;
		}
	}


	void post_sky_kh([[maybe_unused]] game::viewParms* view)
	{
		ff_is_sky = false;

		{
			patches::get()->init_texture_addons();
			struct vertex { D3DXVECTOR3 position; D3DCOLOR color; float tu, tv; };

			const auto dev = shared::globals::d3d_device;

			// save & restore after drawing
			IDirect3DVertexShader9* og_vs = nullptr;
			dev->GetVertexShader(&og_vs);
			dev->SetVertexShader(nullptr);

			IDirect3DBaseTexture9* og_tex = nullptr;
			dev->GetTexture(0, &og_tex);
			dev->SetTexture(0, tex_addons::sky_gray_up);

			DWORD og_blend;
			dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &og_blend);

			D3DXMATRIX og_tex_transform = {};
			dev->GetTransform(D3DTS_TEXTURE0, &og_tex_transform);

			dev->SetTransform(D3DTS_TEXTURE0, &shared::globals::IDENTITY);
			dev->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);

			const float f_index = 0.0f;
			const vertex mesh_verts[4] =
			{
				D3DXVECTOR3(-4.1337f - (f_index * 0.01f), -4.1337f - (f_index * 0.01f), 0), D3DCOLOR_XRGB(0, 0, 0), 0.0f, f_index / 100.0f,
				D3DXVECTOR3(4.1337f + (f_index * 0.01f), -4.1337f - (f_index * 0.01f), 0), D3DCOLOR_XRGB(0, 0, 0), f_index / 100.0f, 0.0,
				D3DXVECTOR3(4.1337f + (f_index * 0.01f),  4.1337f + (f_index * 0.01f), 0), D3DCOLOR_XRGB(0, 0, 0), 0.0f, f_index / 100.0f,
				D3DXVECTOR3(-4.1337f - (f_index * 0.01f),  4.1337f + (f_index * 0.01f), 0), D3DCOLOR_XRGB(0, 0, 0), 0.0f, f_index / 100.0f,
			};

			D3DXMATRIX scale_matrix, rotation_x, rotation_y, rotation_z, mat_rotation, mat_translation, world;

			D3DXMatrixScaling(&scale_matrix, 200.0f, 200.0f, 200.0f);
			D3DXMatrixRotationX(&rotation_x, DEG2RAD(90.0f)); // pitch
			D3DXMatrixRotationY(&rotation_y, 0.0f); // yaw
			D3DXMatrixRotationZ(&rotation_z, 0.0f); // roll
			mat_rotation = rotation_z * rotation_y * rotation_x; // combine rotations (order: Z * Y * X)

			D3DXMatrixTranslation(&mat_translation, 0.0f, -10000.0f, 0.0f);
			world = scale_matrix * mat_rotation * mat_translation;

			dev->SetTransform(D3DTS_WORLD, &world);
			dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, mesh_verts, sizeof(vertex));

			// restore
			dev->SetVertexShader(og_vs);
			dev->SetTexture(0, og_tex);
			dev->SetRenderState(D3DRS_ALPHABLENDENABLE, og_blend);
			dev->SetFVF(NULL);
			dev->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
		}
	}

	__declspec(naked) void post_sky_stub()
	{
		static uint32_t retn_addr = 0x510B1E;
		__asm
		{
			pushad;
			push	edi; //viewparms
			call	post_sky_kh;
			add		esp, 4;
			popad;

			mov     eax, [esp + 0x18];
			push    eax;
			jmp		retn_addr;
		}
	}

	// ---

	Vector m_player_pos;

	//post_start_view_stub
	void post_start_view_hk(game::viewParms* view)
	{
		const auto& im = imgui::get();

		if (view)
		{
			m_player_pos = view->m_Pos;

			// can be used to disable sky, enable normal pass etc
			//view->m_DrawMode_maybe2 = 0;

			
			// this gets rid of frustum culling behind the player and nearby
			// but only with 'VisDrawFrustum' which renders every node
			// 0x5190DC numplanes must be set to 0
			const auto viewbox_offs = im->m_debug_vector.y;
			view->m_ViewAABBMin.x -= viewbox_offs;
			//view->m_ViewAABBMin.y -= 500.0f;
			view->m_ViewAABBMin.z -= viewbox_offs;
			view->m_ViewAABBMax.x += viewbox_offs;
			//view->m_ViewAABBMax.y += 500.0f;
			view->m_ViewAABBMax.z += viewbox_offs;

			/*view->m_pos.x += im->m_debug_vector2.x;
			view->m_pos.y += im->m_debug_vector2.y;
			view->m_pos.z += im->m_debug_vector2.z;
			view->m_Pos.x += im->m_debug_vector2.x;
			view->m_Pos.y += im->m_debug_vector2.y;
			view->m_Pos.z += im->m_debug_vector2.z;
			view->m_PosDupe.x += im->m_debug_vector2.x;
			view->m_PosDupe.y += im->m_debug_vector2.y;
			view->m_PosDupe.z += im->m_debug_vector2.z;*/
#if 1
			//if (im->m_viewmodel_use_custom_proj) 
			{
				//view->m_ViewBox.m_FarZ = im->m_viewmodel_proj_far_plane;

				D3DXMATRIX proj_matrix;

				// this fixes NaN's in the sky (can cause fx placement issues)
				D3DXMatrixPerspectiveFovLH(&proj_matrix,
					im->m_viewmodel_use_custom_proj ? D3DXToRadian(im->m_viewmodel_proj_fov) : view->m_yFov, // 
					view->m_fScreenWidth / view->m_fScreenHeight,
					view->m_ViewBox.m_NearZ, //im->m_viewmodel_proj_near_plane,
					view->m_ViewBox.m_FarZ); //im->m_viewmodel_proj_far_plane);

				
				D3DXMatrixTranspose(&view->projMatrix, &proj_matrix);
				view->projMatrix.m[2][2] = 1.000005f; // fix NaN's without causing fx issues
			}
#endif

			//const auto CPLANE_NEAR_INDEX = 0;
			//const auto CPLANE_FAR_INDEX = 1;
			//const auto CPLANE_LEFT_INDEX = 2;
			//const auto CPLANE_TOP_INDEX = 3;
			//const auto CPLANE_RIGHT_INDEX = 4;
			//const auto CPLANE_BOTTOM_INDEX = 5;

			////view->m_ClipPlanes[0].m_normal = { 0.0, 0.0, 1.0 }; // Near
			////view->m_ClipPlanes[1].m_normal = { 0.0, 0.0, -1.0 }; // Far
			////view->m_ClipPlanes[2].m_normal = { 1.0, 0.0, 0.0 }; // Left
			////view->m_ClipPlanes[3].m_normal = { 0.0, 1.0, 0.0 }; // Top
			////view->m_ClipPlanes[4].m_normal = { -1.0, 0.0, 0.0 }; // Right
			////view->m_ClipPlanes[5].m_normal = { 0.0, -1.0, 0.0 }; // Bottom

			//for (auto i = 0u; i < 5; i++) 
			//{
			//	//if (i != CPLANE_NEAR_INDEX /*&& i != CPLANE_FAR_INDEX*/) 
			//	{
			//		//view->m_ClipPlanes[i].m_normal.Init(0);
			//		//view->m_ClipPlanes[i].m_dist = 0.0f;
			//	}
			//}


			// had this ON???
			//view->m_ClipPlanes[0].m_dist -= 10000.0f;
			//view->m_ClipPlanes[0].m_dist -= im->m_debug_vector.z;
		}
	}

	__declspec(naked) void post_start_view_stub()
	{
		static uint32_t retn_addr = 0x4FFF59;
		__asm
		{
			lea     edx, [esp + 0xC];

			pushad;
			push	edx; //viewparms
			call	post_start_view_hk;
			add		esp, 4;
			popad;

			push    edx;
			jmp		retn_addr;
		}
	}

	struct cull02_s
	{
		cull02_s* sub[1];
		cull02_s* subbbbbb[1];
		float unkfloat2;
		Vector mins;
		Vector maxs;
		BYTE gap10[4];
		float* pfloat28;
		DWORD dword2C;
		cull02_s* sub2[1];
		int x2;
		BYTE x3a;
		BYTE x3b;
		BYTE x3c;
		BYTE x3d;
		int x3;
	};
	STATIC_ASSERT_OFFSET(cull02_s, mins, 0xC);


	bool IsAABBVisible(float* a, float a2, float a3, float a4)
	{
		return a2 >= *a && a2 <= a[3] && a3 >= a[1] && a3 <= a[4] && a4 >= a[2] && a4 <= a[5];
	}

	bool is_aabb_within_distance(const Vector& mins, const Vector& maxs, const Vector& player_origin, const float radius)
	{
		auto sq_dist = 0.0f;
		for (auto i = 0; i < 3; ++i)
		{
			if (player_origin[i] < mins[i])
			{
				const auto d = mins[i] - player_origin[i];
				sq_dist += d * d;
			}
			else if (player_origin[i] > maxs[i])
			{
				const auto d = player_origin[i] - maxs[i];
				sq_dist += d * d;
			}

			// return false if distance exceeds radius sqr
			if (sq_dist > radius * radius) {
				return false;
			}
		}

		return true;
	}

	char __fastcall cull02(cull02_s* this_ptr, [[maybe_unused]] void* junk, float* pos)
	{
		const auto* im = imgui::get();

		//return 0;
		float* i = nullptr;

		if (!is_aabb_within_distance(this_ptr->mins, this_ptr->maxs, m_player_pos, im->m_debug_vector.x))
		{
			//int x = 1;
			return 0;
		}

		if (IsAABBVisible(&this_ptr->mins.x, *pos, pos[1], pos[2]))
		//if (is_aabb_within_distance(this_ptr->mins, this_ptr->maxs, pos, im->m_debug_vector.x))  
		{
			
			//if (is_aabb_within_distance(this_ptr->mins, this_ptr->maxs, m_player_pos, im->m_debug_vector.x)) 
			//{
			//	//int x = 1;
			//	return 1; 
			//}
			// m_player_pos

			int v3 = this_ptr->dword2C; // Number of planes 
			int v4 = 0;

			if (!v3)
			{
				/*if (is_aabb_within_distance(this_ptr->mins, this_ptr->maxs, m_player_pos, im->m_debug_vector.x))
				{
					return 0;
				}*/

				return 1;
			}

			for (i = this_ptr->pfloat28; v4 < v3; i += 5, ++v4)
			{
				// Skip near plane (assume index 4 is near plane, adjust if needed)
				//if (v4 == 0)
				//	continue;

				if (i[2] * pos[2] + i[1] * pos[1] + *pos * *i - i[3] >= 0.0)
				{
					if (++v4 >= v3)
						return 1;
				}
				else
				{
					return 0;
				}
			}
		}
		return 0;
	}


	// ---

	char cull02_none_hk(cull02_s* this_ptr, float* pos)
	{
		//const auto* im = imgui::get();

		if (!this_ptr)
		{
			return 0;
		}

		//return 0;
		float* i = nullptr;

		if (IsAABBVisible(&this_ptr->mins.x, *pos, pos[1], pos[2]))
			//if (is_aabb_within_distance(this_ptr->mins, this_ptr->maxs, pos, im->m_debug_vector.x))  
		{

			//if (is_aabb_within_distance(this_ptr->mins, this_ptr->maxs, m_player_pos, im->m_debug_vector.x)) 
			//{
			//	//int x = 1;
			//	return 1; 
			//}
			// m_player_pos

			int v3 = this_ptr->dword2C; // Number of planes 
			int v4 = 0;

			if (!v3)
			{
				/*if (is_aabb_within_distance(this_ptr->mins, this_ptr->maxs, m_player_pos, im->m_debug_vector.x))
				{
					return 0;
				}*/

				return 1;
			}

			for (i = this_ptr->pfloat28; v4 < v3; i += 5, ++v4)
			{
				// Skip near plane (assume index 4 is near plane, adjust if needed)
				//if (v4 == 0)
				//	continue;

				if (i[2] * pos[2] + i[1] * pos[1] + *pos * *i - i[3] >= 0.0)
				{
					if (++v4 >= v3)
						return 1;
				}
				else
				{
					return 0;
				}
			}
		}
		return 0;
	}

	struct temp_some_bspnode_sub
	{
		cull02_s* x0;
		int x1;
		temp_some_bspnode_sub* x2;
		temp_some_bspnode_sub* x3;
		int x4;
		float* x5;
	};

	struct temp_some_bspnode_struct
	{
		void* unk_objects;
		void* brushes;
		int some_flags0;
		void* ptr_to_planes_or_similar;
		int some_flags1;
		temp_some_bspnode_sub* dword14;
		int some_flags2;
	};

	// returning 0 renders the node
	cull02_s* __fastcall Cull01(temp_some_bspnode_struct* this_ptr, [[maybe_unused]] void* junk, Vector* box_pos)
	{
		temp_some_bspnode_sub* dword14; // esi
		int v3; // edi
		unsigned int x4; // eax

		//const auto* im = imgui::get();

		if (!this_ptr->some_flags2)
		{
			return nullptr;
		}

		dword14 = this_ptr->dword14;
		if (!dword14)
		{
			return nullptr;
		}

		while (true)
		{
			v3 = 0;
			if (dword14->x1)                           // setting the initial value (test was 7) to 0 kinda inverts culling
			{
				//if (is_aabb_within_distance(dword14->x0->sub->mins, dword14->x0->sub->maxs, m_player_pos, im->m_debug_vector.x))
				//{
				//	break;
				//	//int x = 1;
				//	//return 1; 
				//}

				break;
			}
			
		LABEL_6:
			x4 = dword14->x4;
			if (x4 <= 2)
			{
				auto flt_cmp = (double)*(float*)&dword14->x5;

				dword14 = *(&box_pos->x + x4) >= flt_cmp ? dword14->x3 : dword14->x2;

				//if (is_aabb_within_distance(dword14->x0->sub->mins, dword14->x0->sub->maxs, m_player_pos, im->m_debug_vector.x))
				//{
				//	continue;
				//	//int x = 1;
				//	//return 1; 
				//}

				// If the next node exists, continue the loop
				if (dword14) {
					continue;
				}
			}

			return nullptr;
		}

		while (!cull02_none_hk(dword14->x0->sub[v3], &box_pos->x))
		{
			if (++v3 >= dword14->x1)
			{
				goto LABEL_6;
			}
		}

		//if (v3 + 2 < dword14->x1)
		//{
		//	if (is_aabb_within_distance(dword14->x0->sub[v3+ 2]->mins, dword14->x0->sub[v3 + 2]->maxs, m_player_pos, im->m_debug_vector.x))
		//	{
		//		//(unsigned int)++v3;
		//		//goto LABEL_6;
		//		//continue;
		//		//int x = 1;
		//		//return 1;

		//		const auto ret_node = dword14->x0->sub[v3 + 2];
		//		return ret_node;
		//	}
		//}

		

		const auto ret_node = dword14->x0->sub[v3];
		return ret_node;
	}

	// ---

	// post_world_node_vis
	__declspec(naked) void post_world_node_vis()
	{
		static uint32_t retn_og_jnz_addr = 0x5190FF;
		static uint32_t retn_past_jnz_addr = 0x519092;
		static uint32_t world_node_vis_func_addr = 0x521370;

		__asm
		{
			add     esp, 0x28;
			//test    al, al;
			//jnz		LOC_5190FF;

			// custom code
			mov     edi, [esp + 0x78];
			mov     ebx, [esp + 0x74];
			mov     ebp, [esp + 0x70];

			lea     ecx, [esp + 0x6C];
			push    ecx;
			push    0;
			push    1;
			xor		eax, eax;
			or		eax, 0xFFFFFFFF; // -1 recurseDepth
			push	eax;
			push    edi;
			push    ebx;
			push    ebp;

			push    0x578440; // cullbox_planes
			lea     edx, [esp + 0x4C];
			push    5; // culling plane num
			push    edx;

			call	world_node_vis_func_addr;
			add     esp, 0x28;
			test    al, al;

			jnz		LOC_5190FF; 

			// custom code end

			jmp		retn_past_jnz_addr;

		LOC_5190FF:
			jmp		retn_og_jnz_addr;
		}
	}

	// ---

	patches::patches()
	{
		p_this = this;

		// init addon textures
		//init_texture_addons();

		// uhm .. this crashes the game when pressing num7 or 8 on the numpad lmao (just 
		//shared::common::remix_api::initialize(nullptr, nullptr, nullptr); //begin_scene_cb, end_scene_cb, present_scene_cb);

		// stub before hud is rendered
		shared::utils::hook(0x50171E, pre_render_hud_stub, HOOK_JUMP).install()->quick();

		// get bone count of next mesh to be rendered
		shared::utils::hook(0x50FDD3, pre_mesh_stub, HOOK_JUMP).install()->quick();

		// before and after sky stubs
		shared::utils::hook(0x518A7B, pre_sky_stub, HOOK_JUMP).install()->quick();
		shared::utils::hook(0x510B19, post_sky_stub, HOOK_JUMP).install()->quick();

		// called after viewparams init (before any drawing)
		shared::utils::hook(0x4FFF54, post_start_view_stub, HOOK_JUMP).install()->quick();

		// skip prepass shit (normalmap flickering)
		// 0x516B0A -> jmp (0xEB) to disable more prepass stuff ... or nop 0x5106EF 3 + 0x5106F4 5
		shared::utils::hook::nop(0x5106EF, 3); shared::utils::hook::nop(0x5106F4, 5); // nop entire function call, overs below are obsolete
		shared::utils::hook::nop(0x517A18, 6); // nop 6 @ 0x517A18 should be enough - the rest could be removed
		shared::utils::hook::nop(0x517A06, 3); shared::utils::hook::nop(0x517A0B, 5); // 01
		shared::utils::hook::nop(0x5179EC, 3); shared::utils::hook::nop(0x5179F1, 5); // 02
		shared::utils::hook::nop(0x5179D2, 3); shared::utils::hook::nop(0x5179D7, 5); // 03 - flashlight
		

		// disable SetTransform's for the sky
		shared::utils::hook::nop(0x518B3E, 5); 
		shared::utils::hook::nop(0x518C20, 5); 

		// disable SetTransform's for ... 3d skybox fx?
		shared::utils::hook::nop(0x518CBC, 5);
		shared::utils::hook::nop(0x518D0E, 5); 


		// distance based anti cull
		//shared::utils::hook::set<BYTE>(0x5190DC + 1, 0x00); // set number of culling planes from 5 to 0
		//shared::utils::hook(0x458446, cull02, HOOK_CALL).install()->quick(); // add distance based anti culling check

		// test
		//shared::utils::hook(0x52138C, Cull01, HOOK_CALL).install()->quick();

		// test
		//shared::utils::hook(0x51908B, post_world_node_vis, HOOK_JUMP).install()->quick();


		shared::utils::hook::set<BYTE>(0x519083 + 1, 0x01); // set number of culling planes from 5 to 1
		shared::utils::hook::nop(0x521210, 6);
		shared::utils::hook::nop(0x521221, 6);

		// TODO:
		// hk: 0x5009C7 to toggle cvar overrides on/off // 0xEB - 0x74

		printf("[Module] patches loaded.\n");
	}
}
