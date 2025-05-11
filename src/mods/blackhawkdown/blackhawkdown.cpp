#include "std_include.hpp"

#include <windowsx.h>

#include "modules/imgui.hpp"
#include "shared/common/flags.hpp"
#include "shared/common/remix_api.hpp"
#include "shared/common/remix_vars.hpp"

// -setup

namespace mods::blackhawkdown
{
	namespace
	{
		bool nvg_last_state = false;
	}

	namespace tex_addons
	{
		LPDIRECT3DTEXTURE9 white;
	}

	void init_texture_addons(bool release)
	{
		if (release)
		{
			if (tex_addons::white) tex_addons::white->Release();
			return;
		}

		const auto dev = shared::globals::d3d_device;
		D3DXCreateTextureFromFileA(dev, "rtx_comp\\textures\\white.dds", &tex_addons::white);
	}

	void draw_nocull_markers()
	{
		const auto dev = shared::globals::d3d_device;

		struct vertex { D3DXVECTOR3 position; D3DCOLOR color; float tu, tv; };

		// save & restore after drawing
		IDirect3DVertexShader9* og_vs = nullptr;
		dev->GetVertexShader(&og_vs);
		dev->SetVertexShader(nullptr);

		IDirect3DBaseTexture9* og_tex = nullptr;
		dev->GetTexture(0, &og_tex);
		dev->SetTexture(0, tex_addons::white);

		//DWORD og_rs;
		//dev->GetRenderState((D3DRENDERSTATETYPE)150, &og_rs);

		DWORD og_ff;
		dev->GetFVF(&og_ff);
		dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);

		dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

		{
			size_t hash = 0;
			auto mapname = std::string(game::get_map_name());
			for (const char c : mapname) {
				hash = hash * 31 + c;
			}
			const float f_index = 1.0f + (static_cast<float>(hash % 40000) / 40000.0f) * 4.0f;

			const vertex mesh_verts[4] =
			{
				D3DXVECTOR3(-1.337f - (f_index), 0, -1.337f - (f_index)), D3DCOLOR_COLORVALUE(f_index, 0.0f, 0.0f, 1.0f), 0.0f, f_index / 100.0f,
				D3DXVECTOR3( 1.337f + (f_index), 0, -1.337f - (f_index)), D3DCOLOR_COLORVALUE(0.0f, f_index, 0.0f, 1.0f), f_index / 100.0f, 0.0,
				D3DXVECTOR3( 1.337f + (f_index), 0,  1.337f + (f_index)), D3DCOLOR_COLORVALUE(0.0f,0.0f, f_index, 1.0f), 0.0f, f_index / 100.0f,
				D3DXVECTOR3(-1.337f - (f_index), 0,  1.337f + (f_index)), D3DCOLOR_COLORVALUE(f_index, 0.0f, f_index, 1.0f), 0.0f, f_index / 100.0f,
			};

			D3DXMATRIX scale_matrix, mat_translation, world;
			D3DXMatrixTranslation(&mat_translation, 0.0f, -100.0f, 0.0f);
			D3DXMatrixScaling(&scale_matrix, 5.0f, 5.0f, 5.0f);
			world = scale_matrix * mat_translation;

			// set remix texture hash ~req. dxvk-runtime changes - not really needed
			//dev->SetRenderState((D3DRENDERSTATETYPE)150, 100 + m.index);

			dev->SetTransform(D3DTS_WORLD, &world);
			dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, mesh_verts, sizeof(vertex));
		}
		// restore
		dev->SetVertexShader(og_vs);
		dev->SetTexture(0, og_tex);
		//dev->SetRenderState((D3DRENDERSTATETYPE)150, og_rs);
		dev->SetFVF(og_ff);
		dev->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
	}

	void on_begin_scene_cb()
	{
		//draw_nocull_markers();

		if (static bool initiated_vars_once = false; !initiated_vars_once)
		{
			init_texture_addons(false);

			initiated_vars_once = true;
		}

		// set lod multiplier to 10
		*reinterpret_cast<int*>(0x9F20B8) = 10;

		// disable terrain lights
		*reinterpret_cast<BYTE*>(0x9F2F0E) = 1;

		if (!nvg_last_state && game::is_nightvision_on())
		{
			shared::common::remix_vars::parse_and_apply_conf_with_lerp("nightvision_on.conf", 0, shared::common::remix_vars::EASE_TYPE_SIN_IN, 0.0f, 0.0f);
			nvg_last_state = true;
		}
		else if (nvg_last_state && !game::is_nightvision_on())
		{
			shared::common::remix_vars::parse_and_apply_conf_with_lerp("nightvision_off.conf", 0, shared::common::remix_vars::EASE_TYPE_SIN_IN, 0.0f, 0.0f);
			nvg_last_state = false;
		}
	}

	IDirect3DVertexShader9* ff_og_shader = nullptr;
	bool ff_was_modified = false;
	bool ff_use_shader = false;

	bool is_rendering_mesh = false;
	bool render_skinned = false;
	DWORD og_alphablend = 0;

	struct meshinfo
	{
		BYTE gap0[16];
		DWORD base_vert_index;
		BYTE gap19[27];
		std::uint8_t num_bone_matrices;
	};
	STATIC_ASSERT_OFFSET(meshinfo, num_bone_matrices, 0x2F);


	void pre_drawindexedprim()
	{ 
		const auto& dev = shared::globals::d3d_device;

		if (!shared::globals::imgui_is_rendering)
		{
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
			int x = 1;
#endif
			D3DXMATRIX proj;
			dev->GetTransform(D3DTS_PROJECTION, &proj);

			// ignore 2d?
			if (proj.m[3][3] == 1.0f &&
				proj.m[2][3] == 0.0f)
			{
				return;
			}

			if (const auto im = imgui::get(); im && im->m_dbg_use_fake_camera
				|| render_skinned)
			{
				if (render_skinned)
				{
					//og_alphablend
					dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &og_alphablend);
					dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
					//dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESSEQUAL);
					//dev->SetRenderState(D3DRS_ALPHAREF, (DWORD)0x00000080); // 128
				}

				dev->GetVertexShader(&ff_og_shader);  
				dev->SetVertexShader(nullptr);
				ff_was_modified = true;

				/*if (render_skinned)
				{
					dev->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_3WEIGHTS);
					dev->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, TRUE);
				}*/
			}
		}
	}

	void post_drawindexedprim()
	{
		const auto& dev = shared::globals::d3d_device;
		if (ff_was_modified)
		{
			dev->SetVertexShader(ff_og_shader);
			dev->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
			//dev->SetFVF(0);

			if (render_skinned)
			{
				dev->SetRenderState(D3DRS_ALPHABLENDENABLE, og_alphablend);
			}
		}

		is_rendering_mesh = false;
		render_skinned = false;
		ff_was_modified = false;
		ff_use_shader = false;
	}

	
	void install_signature_patches()
	{
		std::uint32_t install_counter = 0u;
		std::uint32_t total_patch_amount = 0u;

		// ------------------
		std::cout << "[SIG] Installed " << std::to_string(install_counter) << "/" << std::to_string(total_patch_amount) << " signature patches.\n";
	}

	void setup_bone_matrices(const meshinfo* info)
	{
		if (info)
		{
			const auto dev = shared::globals::d3d_device;
			const auto numbones = info->num_bone_matrices;
			//static D3DXMATRIX trans_bones[512];
			static D3DXMATRIX trans_bone;

			const auto bones = reinterpret_cast<D3DXMATRIX*>(0xB7FF10);

			for (auto i = 0u; i < numbones; i++)
			{
				const auto bone = &bones[i];
				D3DXMatrixTranspose(&trans_bone, bone);
				dev->SetTransform(D3DTS_WORLDMATRIX(i), &trans_bone);
			}
			
			//shared::utils::transpose_d3dxmatrix(bones, trans_bones, numbones);

			dev->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_3WEIGHTS);
			dev->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, 1);

			//dev->GetVertexShader(&ff_og_shader);
			//dev->SetVertexShader(nullptr);
			//ff_was_modified = true;
			render_skinned = true; 
		}
	}

	__declspec(naked) void on_skinned_stub()
	{
		static uint32_t og_fn_addr = 0x4FEDC0;
		static uint32_t retn_addr = 0x4FFF28;
		__asm
		{
			pushad;
			//mov		render_skinned, 1;
			push	ebx; // meshinfo
			call	setup_bone_matrices;
			add		esp, 4;
			popad;

			call	og_fn_addr;
			jmp		retn_addr;
		}
	}

	void on_map_load_hk()
	{
		nvg_last_state = false;
		shared::common::remix_vars::on_map_load(game::get_map_name());
	}

	__declspec(naked) void on_map_load_stub()
	{
		static uint32_t retn_addr = 0x415F79;
		__asm
		{
			pushad;
			call	on_map_load_hk;
			popad;

			push    ebp;
			mov     ebp, esp;
			sub     esp, 0x20C;
			jmp		retn_addr;
		}
	}

	void post_sky_render_hk()
	{
		draw_nocull_markers();
	}

	__declspec(naked) void post_sky_render_stub()
	{
		static uint32_t og_fn_addr = 0x5A9D00;
		static uint32_t retn_addr = 0x4F376D;
		__asm
		{
			call	og_fn_addr;

			pushad;
			call	post_sky_render_hk;
			popad;

			jmp		retn_addr;
		}
	}

	void main()
	{
		game::init_game_addresses();

		// init remix api
		shared::common::remix_api::initialize(nullptr, nullptr, nullptr, false);

		// init remix variable system
		shared::common::remix_vars::initialize(game::g_is_paused, &shared::globals::frame_time_ms);

		// 415F70 bmission_LoadMission
		shared::utils::hook(0x415F70, on_map_load_stub, HOOK_JUMP).install()->quick();

		// 4B76C2
		shared::utils::hook(0x4F3768, post_sky_render_stub, HOOK_JUMP).install()->quick();

		// pre transform skinned vertices on the cpu - render via FF
		//shared::utils::hook::conditional_jump_to_jmp(0x4FFDAF);
		//shared::utils::hook::nop(0x4FFDBF, 2);

		shared::utils::hook(0x4FFF23, on_skinned_stub, HOOK_JUMP).install()->quick();

		// disable terrain light ... set byte 0x9F2F0E to 1

		// less cull
		// 4187E0
		//shared::utils::hook::set(0x4187E0, 0x31, 0xC0, 0xC3); // camera_ClipSphere
		shared::utils::hook::conditional_jump_to_jmp(0x41882F);
		shared::utils::hook::conditional_jump_to_jmp(0x418847);
		shared::utils::hook::conditional_jump_to_jmp(0x41889D);
		shared::utils::hook::conditional_jump_to_jmp(0x4188E0);
		shared::utils::hook::conditional_jump_to_jmp(0x418923);
		shared::utils::hook::conditional_jump_to_jmp(0x418966);

		//  terain objects anticull 0x005433C0 nop 2

		// set terrain cache to 512x512?
		//shared::utils::hook::set<BYTE>(0x53F1D4 + 7, 0x02);
		// 53D34C mov eax, 512

		// 46A000
		shared::utils::hook::set<BYTE>(0x46A000 + 1, 0x01);

		// 46A65A
		shared::utils::hook::nop(0x46A65A, 5);
		shared::utils::hook::conditional_jump_to_jmp(0x46A661);

		// fix nightvision camera
		shared::utils::hook::conditional_jump_to_jmp(0x4B75D3);

		if (shared::common::flags::has_flag("disable_terrain"))
		{
			shared::utils::hook::nop(0x5212C6, 5);
			shared::utils::hook::nop(0x52BA3C, 5);
			shared::utils::hook::nop(0x5429AC, 5);
			shared::utils::hook::nop(0x542AC6, 5);
			shared::utils::hook::nop(0x542BE0, 5);
			shared::utils::hook::nop(0x542CFF, 5);
			shared::utils::hook::nop(0x542EB1, 5);
		}

#ifdef DEBUG
		shared::common::loader::module_loader::register_module(std::make_unique<imgui>());
#endif

		MH_EnableHook(MH_ALL_HOOKS);
	}
}
