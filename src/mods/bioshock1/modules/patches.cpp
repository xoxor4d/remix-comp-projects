#include "std_include.hpp"
#include "patches.hpp"

#include "imgui.hpp"
#include "shared/common/flags.hpp"
#include "shared/common/remix.hpp"
#include "shared/common/remix_api.hpp"

// commandline: -dx9 -NOINTRO -windowed

namespace mods::bioshock1
{

	void patches::on_begin_scene()
	{
		if (game::rg)
		{
			shared::globals::d3d_device->SetTransform(D3DTS_WORLD, &game::rg->worldMatrix);
			shared::globals::d3d_device->SetTransform(D3DTS_VIEW, &game::rg->viewMatrix);
			shared::globals::d3d_device->SetTransform(D3DTS_PROJECTION, &game::rg->projMatrix); 
		}
	}

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
	bool ff_use_shader = false;
	bool ff_is_rendering_bsp = false;
	float ff_proj_world = 0.0f;
	bool g_enable_normal_fix = true;

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
		ff_is_rendering_bsp = true;
	}

	void post_bspren_hk()
	{
		ff_is_rendering_bsp = false;
	}

	__declspec(naked) void pre_bspren_stub()
	{
		static uint32_t retn_addr = 0x10B3AD3E;
		__asm
		{
			pushad;
			call	pre_bspren_hk;
			popad;

			push	eax;
			mov     eax, [esp + 0x20];
			mov     ecx, [eax + 0x18];
			mov     eax, [eax + 0x14];
			push    ecx;
			push    eax;
			push    edi;
			push    edi;
			mov     ecx, esi;
			call    edx; // render

			pushad;
			call	post_bspren_hk;
			popad;

			jmp		retn_addr;
		}
	}


	// 

	void pre_skyren_hk()
	{
		ff_use_shader = true;
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

	// Cache for modified vertex declarations (non-dynamic buffers)
	struct ModifiedBuffer {
		IDirect3DVertexDeclaration9* newDecl;
	};
	static std::unordered_map<IDirect3DVertexBuffer9*, ModifiedBuffer> bufferCache;

	// Global variables
	IDirect3DVertexDeclaration9* ff_og_vertexdecl;
	bool ff_fixed_normals = false;

//#define DEBUG_VERTEX_DECL

	// returns false if failed or vb does not contain packed tangent, binormal, and normal
	bool handle_vb_with_packed_normal(const bool is_viewmodel = false)
	{
		const auto& dev = shared::globals::d3d_device;
		HRESULT hr;

		// Get Stream 0 vertex buffer
		IDirect3DVertexBuffer9* pVB;
		UINT offset, stride;
		
		if (hr = dev->GetStreamSource(0, &pVB, &offset, &stride); FAILED(hr) || !pVB) {
			return false;
		}

		// get buffer description
		D3DVERTEXBUFFER_DESC vb_desc;
		if (hr = pVB->GetDesc(&vb_desc); FAILED(hr))
		{
			pVB->Release();
			return false;
		}

		// check if buffer is dynamic
		bool is_dynamic = (vb_desc.Usage & D3DUSAGE_DYNAMIC) != 0;

		// only touch static world buffers
		if (!is_viewmodel && is_dynamic)
		{
			pVB->Release();
			return false;
		}

		// only touch dynamic viewmodel buffers
		if (is_viewmodel && !is_dynamic)
		{
			pVB->Release();
			return false;
		}

		// check cache for static buffers
		if (!is_dynamic) 
		{
			if (const auto it = bufferCache.find(pVB); 
				it != bufferCache.end())
			{
				// use cached declaration
				dev->GetVertexDeclaration(&ff_og_vertexdecl);
				dev->SetVertexDeclaration(it->second.newDecl);

				ff_fixed_normals = true;
				pVB->Release();
				return true;
			}
		}
		
		// get current vertex declaration
		IDirect3DVertexDeclaration9* p_decl;
		if (hr = dev->GetVertexDeclaration(&p_decl); FAILED(hr) || !p_decl)
		{
			pVB->Release();
			return false;
		}

		// Get number of declaration elements
		UINT num_decl_elements;
		if (hr = p_decl->GetDeclaration(nullptr, &num_decl_elements); FAILED(hr) || !num_decl_elements)
		{
			p_decl->Release();
			pVB->Release();
			return false;
		}

#ifdef DEBUG_VERTEX_DECL
		const char* d3ddecltype_str[]
		{
			"D3DDECLTYPE_FLOAT1",		// 1D float expanded to (value, 0., 0., 1.)
			"D3DDECLTYPE_FLOAT2",		// 2D float expanded to (value, value, 0., 1.)
			"D3DDECLTYPE_FLOAT3",		// 3D float expanded to (value, value, value, 1.)
			"D3DDECLTYPE_FLOAT4",		// 4D float
			"D3DDECLTYPE_D3DCOLOR",	// 4D packed unsigned bytes mapped to 0. to 1. range

			// Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
			"D3DDECLTYPE_UBYTE4",		// 4D unsigned byte
			"D3DDECLTYPE_SHORT2",		// 2D signed short expanded to (value, value, 0., 1.)
			"D3DDECLTYPE_SHORT4",		// 4D signed short

			// The following types are valid only with vertex shaders >= 2.0
			"D3DDECLTYPE_UBYTE4N",	// Each of 4 bytes is normalized by dividing to 255.0
			"D3DDECLTYPE_SHORT2N",	// 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
			"D3DDECLTYPE_SHORT4N",	// 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
			"D3DDECLTYPE_USHORT2N",  // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
			"D3DDECLTYPE_USHORT4N",  // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
			"D3DDECLTYPE_UDEC3",		// 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
			"D3DDECLTYPE_DEC3N",		// 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
			"D3DDECLTYPE_FLOAT16_2",	// Two 16-bit floating point values, expanded to (value, value, 0, 1)
			"D3DDECLTYPE_FLOAT16_4",	// Four 16-bit floating point values
			"D3DDECLTYPE_UNUSED2",	// When the type field in a decl is unused.
		};

		const char* d3ddecluse_str[]
		{
			"D3DDECLUSAGE_POSITION",
			"D3DDECLUSAGE_BLENDWEIGHT",   // 1
			"D3DDECLUSAGE_BLENDINDICES",  // 2
			"D3DDECLUSAGE_NORMAL",        // 3
			"D3DDECLUSAGE_PSIZE",         // 4
			"D3DDECLUSAGE_TEXCOORD",      // 5
			"D3DDECLUSAGE_TANGENT",       // 6
			"D3DDECLUSAGE_BINORMAL",      // 7
			"D3DDECLUSAGE_TESSFACTOR",    // 8
			"D3DDECLUSAGE_POSITIONT",     // 9
			"D3DDECLUSAGE_COLOR",         // 10
			"D3DDECLUSAGE_FOG",           // 11
			"D3DDECLUSAGE_DEPTH",         // 12
			"D3DDECLUSAGE_SAMPLE",        // 13
		};

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

		d3dvertelem decl[MAX_FVF_DECL_SIZE];
		pDecl->GetDeclaration((D3DVERTEXELEMENT9*)decl, &numElements);
		//int x = 0;
#endif

		// Retrieve all elements
		std::vector<D3DVERTEXELEMENT9> elements(num_decl_elements);
		p_decl->GetDeclaration(elements.data(), &num_decl_elements);

		// Check for packed tangent, binormal, normal, and required elements
		bool has_packed_normal = false;
		UINT pos_offset = UINT_MAX, normal_offset = UINT_MAX, tc_offset = UINT_MAX;
		UINT tangent_offset = UINT_MAX, binormal_offset = UINT_MAX;
		UINT normal_stream = UINT_MAX;

		// Find offsets and streams
		for (const auto& element : elements) 
		{
			if (element.Usage == D3DDECLUSAGE_NORMAL && element.Type == D3DDECLTYPE_UBYTE4) 
			{
				has_packed_normal = true;
				normal_offset = element.Offset;
				normal_stream = element.Stream;
			}
			else if (element.Usage == D3DDECLUSAGE_TANGENT && element.Type == D3DDECLTYPE_UBYTE4) {
				tangent_offset = element.Offset;
			}
			else if (element.Usage == D3DDECLUSAGE_BINORMAL && element.Type == D3DDECLTYPE_UBYTE4) {
				binormal_offset = element.Offset;
			}
			else if (element.Usage == D3DDECLUSAGE_POSITION && element.Type == D3DDECLTYPE_FLOAT3) {
				pos_offset = element.Offset;
			}
			else if (element.Usage == D3DDECLUSAGE_TEXCOORD && element.Type == D3DDECLTYPE_FLOAT2) {
				tc_offset = element.Offset;
			}
		}

		// skip if required elements missing or normal not in stream 0
		if (!has_packed_normal || tangent_offset == UINT_MAX || binormal_offset == UINT_MAX ||
			pos_offset == UINT_MAX || tc_offset == UINT_MAX || normal_stream != 0) 
		{
			p_decl->Release();
			pVB->Release();
			return false;
		}

		// verify that tangent, binormal, and normal are contiguous (12 bytes)
		const std::uint32_t start_offset = std::min({ tangent_offset, binormal_offset, normal_offset });
		if (tangent_offset < start_offset || binormal_offset < start_offset || normal_offset < start_offset ||
			tangent_offset > start_offset + 8 || binormal_offset > start_offset + 8 || normal_offset > start_offset + 8) 
		{
			p_decl->Release();
			pVB->Release();
			return false;
		}

		// lock vertex buffer for modification
		void* pData;
		if (hr = pVB->Lock(0, 0, &pData, is_dynamic ? D3DLOCK_DISCARD : D3DLOCK_NOOVERWRITE); FAILED(hr))
		{
			p_decl->Release();
			pVB->Release();
			return false;
		}

		// unpack normal and override tangent, binormal, normal (12 bytes)
		for (auto i = 0u; i < vb_desc.Size / stride; ++i) 
		{
			BYTE* packedNormal = (BYTE*)((char*)pData + i * stride + normal_offset);
			float* unpackedNormal = (float*)((char*)pData + i * stride + start_offset);

			const auto xx = packedNormal[0];
			const auto yy = packedNormal[1];
			const auto zz = packedNormal[2];

			unpackedNormal[0] = 2.0f * ((float)xx / 255.0f) - 1.0f;
			unpackedNormal[1] = 2.0f * ((float)yy / 255.0f) - 1.0f;
			unpackedNormal[2] = 2.0f * ((float)zz / 255.0f) - 1.0f;
		}

		pVB->Unlock();

		// create new vertex declaration
		std::vector<D3DVERTEXELEMENT9> new_elements;
		bool replaced = false;

		for (const auto& element : elements) 
		{
			if ((element.Usage == D3DDECLUSAGE_TANGENT || element.Usage == D3DDECLUSAGE_BINORMAL ||
				element.Usage == D3DDECLUSAGE_NORMAL) && element.Type == D3DDECLTYPE_UBYTE4) 
			{
				if (!replaced) 
				{
					// replace with float3 normal at start_offset
					D3DVERTEXELEMENT9 new_element =
					{
						0, static_cast<WORD>(start_offset), D3DDECLTYPE_FLOAT3,
						D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0
					};

					new_elements.push_back(new_element);
					replaced = true;
				}
				// skip original tangent, binormal, normal
			}
			else {
				new_elements.push_back(element);
			}
		}

		// end marker
		new_elements.push_back({ 0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0 });

		IDirect3DVertexDeclaration9* new_decl;
		if (hr = dev->CreateVertexDeclaration(new_elements.data(), &new_decl); FAILED(hr) || !new_decl)
		{
			p_decl->Release();
			pVB->Release();
			return false;
		}

		// cache declaration for static buffers
		if (!is_dynamic) {
			bufferCache[pVB] = { new_decl };
		}

		// set new declaration
		dev->GetVertexDeclaration(&ff_og_vertexdecl);
		dev->SetVertexDeclaration(new_decl);
		ff_fixed_normals = true;

#ifdef DEBUG_VERTEX_DECL
		if (!isDynamic)
		{
			std::cout << "\n------------------------------------- old decl [" << std::to_string(bufferCache.size()) << "]\n";
			for (auto e = 0u; e < numElements; e++) {
				std::cout << "Stream: " << decl[e].Stream << " -- OFFSET: " << std::to_string(decl[e].Offset) << " -- Use: " << d3ddecluse_str[decl[e].Usage] << " -- Type: " << d3ddecltype_str[decl[e].Type] << "\n";
			}
		}
		

		d3dvertelem decl_new[MAX_FVF_DECL_SIZE];
		newDecl->GetDeclaration((D3DVERTEXELEMENT9*)decl_new, &numElements);

		if (!isDynamic)
		{
			std::cout << "to new decl:\n";

			for (auto e = 0u; e < numElements; e++) {
				std::cout << "Stream: " << decl_new[e].Stream << " -- OFFSET: " << std::to_string(decl_new[e].Offset) << " -- Use: " << d3ddecluse_str[decl_new[e].Usage] << " -- Type: " << d3ddecltype_str[decl_new[e].Type] << "\n";
			}

			// Release resources
			if (isDynamic) {
				newDecl->Release(); // No caching for dynamic buffers
			}
		}
		else
		{
			std::cout << "skipping dynamic decl ... \n";
		}
#endif

		p_decl->Release();
		pVB->Release();
		return true;
	}

	bool patches::pre_drawindexedprim_call()
	{
		const auto& dev = shared::globals::d3d_device;

		IDirect3DBaseTexture9* bound_tex = nullptr;
		dev->GetTexture(0, &bound_tex);

		if (!bound_tex) {
			return true;
		}

		
		if (const auto im = imgui::get(); im)
		{
			// hacky way to differ between world and viewmodel - TODO
			if (ff_is_rendering_bsp) {
				ff_proj_world = game::rg->projMatrix.m[1][1];
			}

			if (!shared::globals::imgui_is_rendering &&
				!ff_use_shader && game::rg)
			{
				{
					// if world
					if (shared::utils::float_equal(game::rg->projMatrix.m[1][1], ff_proj_world))
					{
						//DWORD alpha;
						//dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &alpha);

						// only fix up non-alphablended surfaces
						if (/*!alpha &&*/ g_enable_normal_fix && ff_is_rendering_bsp) {
							ff_fixed_normals = handle_vb_with_packed_normal();
						}

						dev->SetFVF(0);
						dev->GetVertexShader(&ff_og_shader);
						dev->SetVertexShader(nullptr);
						dev->GetPixelShader(&ff_og_psshader);
						dev->SetPixelShader(nullptr);
						dev->SetTransform(D3DTS_WORLD, &game::rg->worldMatrix);
						dev->SetTransform(D3DTS_VIEW, &game::rg->viewMatrix);
						//dev->SetTransform(D3DTS_PROJECTION, &game::rg->projMatrix);

						if (im->m_world_use_custom_proj && game::rg->ViewportWidth)
						{
							D3DXMATRIX im_proj;

							// construct projection matrix
							D3DXMatrixPerspectiveFovLH(&im_proj,
								D3DXToRadian(im->m_world_proj_fov),
								(float)game::rg->ViewportWidth / (float)game::rg->ViewportHeight,
								im->m_viewmodel_proj_near_plane,
								im->m_viewmodel_proj_far_plane);

							im_proj.m[2][2] = game::rg->projMatrix.m[2][2];
							im_proj.m[2][3] = game::rg->projMatrix.m[2][3];
							im_proj.m[3][2] = game::rg->projMatrix.m[3][2];
							im_proj.m[3][3] = game::rg->projMatrix.m[3][3];

							dev->SetTransform(D3DTS_PROJECTION, &im_proj);
						}
						else {
							dev->SetTransform(D3DTS_PROJECTION, &game::rg->projMatrix);
						}


						ff_was_modified = true;

						//DWORD og_srcblend = 0u, og_destblend = 0u;
						//dev->GetRenderState(D3DRS_SRCBLEND, &og_srcblend);
						//dev->GetRenderState(D3DRS_DESTBLEND, &og_destblend);

						// do not allow emissive surfaces - helps with meshes getting emissive when using plasmids
						//if (og_srcblend == D3DBLEND_ONE && og_destblend == D3DBLEND_ONE) 
						//{
						//	dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO); // FIXME! this breaks particles and decals 
						//	//return true;
						//}

						//dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
						//dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
						//dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
					}
					else if (ff_proj_world != 0.0f) // viewmodel
					{
						//return true;
						//bool use_fallback = !handle_vb_with_packed_normal();
						//if (use_fallback)
						{
							if (g_enable_normal_fix) {
								ff_fixed_normals = handle_vb_with_packed_normal(true);
							}

							dev->SetFVF(0);

							dev->GetVertexShader(&ff_og_shader);
							dev->SetVertexShader(nullptr);
							dev->GetPixelShader(&ff_og_psshader);
							dev->SetPixelShader(nullptr);

							dev->SetTransform(D3DTS_WORLD, &game::rg->worldMatrix);
							dev->SetTransform(D3DTS_VIEW, &game::rg->viewMatrix);
							

							dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
							dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
							dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);

							if (im->m_viewmodel_use_custom_proj && game::rg->ViewportWidth)
							{
								D3DXMATRIX im_proj;

								// construct projection matrix
								D3DXMatrixPerspectiveFovLH(&im_proj,
									D3DXToRadian(im->m_viewmodel_proj_fov),
									(float)game::rg->ViewportWidth / (float)game::rg->ViewportHeight,
									im->m_viewmodel_proj_near_plane,
									im->m_viewmodel_proj_far_plane);

								im_proj.m[2][2] = game::rg->projMatrix.m[2][2];
								im_proj.m[2][3] = game::rg->projMatrix.m[2][3];
								im_proj.m[3][2] = game::rg->projMatrix.m[3][2];
								im_proj.m[3][3] = game::rg->projMatrix.m[3][3];

								dev->SetTransform(D3DTS_PROJECTION, &im_proj);
							}
							else {
								dev->SetTransform(D3DTS_PROJECTION, &game::rg->projMatrix);
							}

							ff_was_modified = true;
						}
					}
				}
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

			if (ff_fixed_normals && ff_og_vertexdecl)
			{
				// restore original declaration
				dev->SetVertexDeclaration(ff_og_vertexdecl);
				ff_og_vertexdecl->Release();
				ff_og_vertexdecl = nullptr;
			}
		}

		ff_fixed_normals = false;
		ff_was_modified = false;
		ff_was_viewmodel = false;
		ff_use_shader = false;
	}

	patches::patches()
	{
		p_this = this;

		g_enable_normal_fix = !shared::common::flags::has_flag("disable_normal_hack");

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
		shared::utils::hook(0x10B3AD2B, pre_bspren_stub, HOOK_JUMP).install()->quick();

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


		// render more of the map to fix light leakage (imgui anti cull tweak #1)
		shared::utils::hook::nop(0x10AEC1DC, 2);
		shared::utils::hook::nop(0x10AE7D5D, 6);
		shared::utils::hook::set<BYTE>(0x10AE7D23, 0xEB);
		// shared::utils::hook::set<BYTE>(0x10AEC7E3, 0xEB); + that renders the entire map 

		// FIX VERTEX EXPLOSIONS!
		shared::utils::hook::set(0x10AE8D25, 0xE9, 0x29, 0x01, 0x0, 0x0, 0x90);

		// Notes
		/*
		 * Not calling 0x10AE90B8 (RenderScene) hides the viewmodel (alpha 0) and makes it emissive
		 */

		std::cout << "[Module] patches loaded.\n";
	}
}
