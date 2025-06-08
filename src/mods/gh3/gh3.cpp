#include "std_include.hpp"

//#include <windowsx.h>
#include "modules/imgui.hpp"
#include "shared/common/dinput_hook.hpp"
//#include "shared/common/flags.hpp"
#include "shared/common/remix_api.hpp"
//#include "shared/common/remix_vars.hpp"
#include "shared/common/flags.hpp"
#include "shared/common/shader_cache.hpp"

namespace mods::gh3
{
	namespace tex_addons
	{
		LPDIRECT3DTEXTURE9 white;
	}

	void init_texture_addons([[maybe_unused]] bool release)
	{
		/*if (release)
		{
			if (tex_addons::white) tex_addons::white->Release();
			return;
		}

		const auto dev = shared::globals::d3d_device;
		D3DXCreateTextureFromFileA(dev, "rtx_comp\\textures\\white.dds", &tex_addons::white);*/
	}

	void on_begin_scene_cb()
	{
		//draw_nocull_markers();

		if (static bool initiated_vars_once = false; !initiated_vars_once)
		{
			init_texture_addons(false);
			initiated_vars_once = true;
		}

		if (imgui::is_initialized())
		{
			const auto& im = imgui::get();
			if (im->m_dbg_use_fake_camera)
			{
				D3DXMATRIX view_matrix, proj_matrix;

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
			}
		}
	}

	IDirect3DVertexShader9* ff_og_shader = nullptr;
	bool ff_was_modified = false;
	bool ff_use_shader = false;
	bool ff_stage0_normalmap = false;

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

			if (vertex_decl)
			{
				d3dvertelem decl[MAX_FVF_DECL_SIZE]; UINT numElements = 0;
				vertex_decl->GetDeclaration((D3DVERTEXELEMENT9*)decl, &numElements);
				int x = 1;
			}
#endif
			//D3DXMATRIX proj;
			//dev->GetTransform(D3DTS_PROJECTION, &proj);

			//// ignore 2d?
			//if (proj.m[3][3] == 1.0f &&
			//	proj.m[2][3] == 0.0f)
			//{
			//	return;
			//}

			const auto& im = imgui::get(); 

			D3DXMATRIX* proj = reinterpret_cast<D3DXMATRIX*>(0xC5BA84);

			/*if (proj->m[3][3] == 1.0f &&
				proj->m[2][3] == 0.0f)
			{
				return;
			}*/

			// some objects have colordata in stage1 and normal map data in stage0
			if (im->m_dbg_texture_stage1_hack && ff_stage0_normalmap)
			{
				IDirect3DBaseTexture9* base = nullptr;
				shared::globals::d3d_device->GetTexture(1, &base);
				shared::globals::d3d_device->SetTexture(0, base);
			}

			if (im->m_dbg_disable_shaders)
			{
				// Get current vertex shader
				dev->GetVertexShader(&ff_og_shader);

				// Check if shader is whitelisted
				ff_use_shader = ff_og_shader && shared::common::g_shader_cache.is_shader_whitelisted(ff_og_shader);
				if (!ff_use_shader)
				{
					D3DXMATRIX* world = reinterpret_cast<D3DXMATRIX*>(0xC5B944);
					D3DXMATRIX* view = reinterpret_cast<D3DXMATRIX*>(0xC5BA04);

					shared::globals::d3d_device->SetTransform(D3DTS_WORLD, world);
					shared::globals::d3d_device->SetTransform(D3DTS_VIEW, view);
					shared::globals::d3d_device->SetTransform(D3DTS_PROJECTION, proj);

					dev->GetVertexShader(&ff_og_shader);
					dev->SetVertexShader(nullptr);
					ff_was_modified = true;
				}
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
		}

		ff_stage0_normalmap = false;
		ff_was_modified = false;
		ff_use_shader = false;
	}

	struct texdata_s
	{
		int unk1;
		int unk2;
		//DWORD flag1;
		std::uint16_t lflag1;
		std::uint16_t hflag1;
		//DWORD flag2;
		std::uint16_t lflag2;
		std::uint16_t hflag2;
		//DWORD flag3;
		std::uint16_t lflag3;
		std::uint16_t hflag3;
		//int unk3;
		std::uint16_t lflag4;
		std::uint16_t hflag4;
		IDirect3DBaseTexture9* tex0;
		IDirect3DBaseTexture9* tex;
		int unk3;
		int unk4;
	}; STATIC_ASSERT_OFFSET(texdata_s, tex, 0x1C);

	void set_texture_hk(int stage, texdata_s* info)
	{
		if (!imgui::is_initialized() || !info || !info->tex) {
			return;
		}

		const auto im = imgui::get();

		if (im->m_dbg_disable_stage0 && stage == 0 && info->unk1 == 0x0102280a) // normalmaps
		{
			//shared::globals::d3d_device->SetTexture(stage, tex_addons::white);
			ff_stage0_normalmap = true;
		}
		else
		{
			// og func
			shared::globals::d3d_device->SetTexture(stage, info->tex);
		}
	}

	__declspec(naked) void set_texture_stub()
	{
		static uint32_t retn_addr = 0x649681;
		__asm
		{
			push	eax;
			push	ebx;
			call	set_texture_hk;
			add		esp, 8;
			jmp		retn_addr;
		}
	}

	void install_signature_patches()
	{
		std::uint32_t install_counter = 0u;
		std::uint32_t total_patch_amount = 0u;

		// ------------------
		std::cout << "[SIG] Installed " << std::to_string(install_counter) << "/" << std::to_string(total_patch_amount) << " signature patches.\n";
	}

	void main()
	{
		game::init_game_addresses();

		// init remix api
		shared::common::remix_api::initialize(nullptr, nullptr, nullptr, false);

		// init remix variable system
		//shared::common::remix_vars::initialize(game::g_is_paused, &shared::globals::frame_time_ms);

		if (!shared::common::flags::has_flag("no_dinput_hook")) {
			shared::common::dinput::init();
		}

		// detect meshes rendering with normalmaps in stage0
		shared::utils::hook(0x64966F, set_texture_stub, HOOK_JUMP).install()->quick();

		// vertex shader whitelist (stuff not to render with FF)
		shared::common::g_shader_cache.add_to_whitelist(0xE53982FB); // crowd 1
		shared::common::g_shader_cache.add_to_whitelist(0xEC856774); // crowd 2

//#ifdef DEBUG
		shared::common::loader::module_loader::register_module(std::make_unique<imgui>());
//#endif

		MH_EnableHook(MH_ALL_HOOKS);
	}
}
