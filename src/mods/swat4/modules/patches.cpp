#include "std_include.hpp"
#include "patches.hpp"

#include "imgui.hpp"
#include "shared/common/remix.hpp"
#include "shared/common/remix_api.hpp"

namespace mods::swat4
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
			D3DXCreateTextureFromFileA(dev, "swat4-rtx\\textures\\graycloud_up.jpg", &tex_addons::sky_gray_up);

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

		//const auto& im = imgui::get();
		//const auto& patches = patches::get();

		/* if (!im->m_is_rendering)
		{
			shared::float3x4* to_world = reinterpret_cast<shared::float3x4*>(0x5776F0);

			D3DXMATRIX transworld = {};
			shared::utils::transpose_float3x4_to_d3dxmatrix(*to_world, transworld);
			dev->SetTransform(D3DTS_WORLD, &transworld);

			dev->GetVertexShader(&ff_og_shader);
			dev->SetVertexShader(nullptr);
			dev->GetPixelShader(&ff_og_psshader);
			dev->SetPixelShader(nullptr);

			//dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0u);
			ff_was_modified = true;
		} */

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

		//const auto& im = imgui::get();
		//const auto& patches = patches::get(); 

		dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0u);
		return false;
	}


	// called right after device->DrawIndexedPrimitive
	void patches::post_drawindexedprim_call()
	{
		/*const auto& dev = shared::globals::d3d_device;
		if (ff_was_modified)
		{
			dev->SetVertexShader(ff_og_shader);
			dev->SetPixelShader(ff_og_psshader);
		}

		ff_is_hud = false;
		ff_was_modified = false;
		ff_was_viewmodel = false;
		patches::get()->m_ff_use_shader = false;*/
	}


	void post_get_view_frustum_hk(game::FConvexVolume* frustum)
	{
		const auto& im = imgui::get();

		if (im->m_cull_disable_frustum)
		{
			for (auto i = 0u; i < 5; i++) {
				frustum->BoundingPlanes[i].dist += im->m_cull_frustum_tweak_distance_offset; //10000.0f;
			}
		}
		
	}

	HOOK_RETN_PLACE_DEF(post_get_view_frustum_retn_addr);
	__declspec(naked) void post_get_view_frustum_stub()
	{
		__asm
		{
			pushad;
			push	eax;
			call	post_get_view_frustum_hk;
			add		esp, 4;
			popad;

			mov		[edi], eax;
			mov		[edi + 4], esi;
			jmp		post_get_view_frustum_retn_addr;
		}
	}

	// ---

	patches::patches()
	{
		p_this = this;

		// init addon textures
		//init_texture_addons();

		// uhm .. this crashes the game when pressing num7 or 8 on the numpad lmao (just 
		shared::common::remix_api::initialize(begin_scene_cb, end_scene_cb, present_scene_cb, true);

		// this fixes frustum culling
		shared::utils::hook(ENGINE_BASE + 0x1FDB19, post_get_view_frustum_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(post_get_view_frustum_retn_addr, ENGINE_BASE + 0x1FDB1E);

		// ENGINE + 0x1F6ABB (nop2)
		// ^ + 0x1F6ACD+3 to byte 01 (to mov [ebp-20],00000001)

		// ^ + 0x1F6AEC (nop5)
		// ^ + 0x1F6AF1 set bytes to B8 01 00 00 00 (mov eax,00000001)


		printf("[Module] patches loaded.\n");
	}
}
