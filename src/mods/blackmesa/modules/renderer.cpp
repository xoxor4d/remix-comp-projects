#include "std_include.hpp"
#include "renderer.hpp"

#include "shared/common/flags.hpp"

namespace mods::blackmesa
{
	void cmeshdx8_renderpass_pre_draw(game::CMeshDX8* mesh, [[maybe_unused]] /*CPrimList**/ std::uint32_t primlist)
	{
		const auto dev = game::get_d3d_device();

		IDirect3DVertexBuffer9* buffer9 = nullptr;
		UINT stride = 0;
		{
			UINT ofs = 0; dev->GetStreamSource(0, &buffer9, &ofs, &stride);
		}

		//DWORD bufferedstateaddr = RENDERER_BASE + 0x19530;
		//auto x = reinterpret_cast<components::IShaderAPIDX8*>(*(DWORD*)(RENDERER_BASE + 0xC9C50));
		//auto y = reinterpret_cast<components::IShaderAPIDX8*>((RENDERER_BASE + 0xC9C54));

		auto& ctx = renderer::primctx;
		const auto shaderapi = game::get_shaderapi();

		if (ctx.get_info_for_pass(shaderapi))
		{
#if 0
			// added format check
			if (mesh->m_VertexFormat == 0x480033 || mesh->m_VertexFormat == 0x80033)
			{
				if (ctx.info.shader_name.starts_with("Wa") && ctx.info.shader_name.contains("Water"))
				{
					game::IMaterialVar* var = nullptr;
					if (has_materialvar(ctx.info.material, "$basetexture", &var))
					{
						// if material has NO defined basetexture
						if (var && !var->vftable->IsDefined(var))
						{
							// check if it has a defined bottommaterial
							var = nullptr;
							const auto has_bottom_mat = has_materialvar(ctx.info.material, "$bottommaterial", &var);

							if (has_bottom_mat)
							{
								const auto& ms = map_settings::get_map_settings();

								// we only need one surface
								ctx.modifiers.as_water = true;
								ctx.modifiers.og_mesh_z_offset = ms.water_offset_bottom;
								ctx.modifiers.dual_render_with_specified_texture = true;
								ctx.modifiers.dual_render_texture_z_offset = ms.water_offset_top; //0.5f;
								ctx.modifiers.dual_render_texture = shaderapi->vtbl->GetD3DTexture(shaderapi, nullptr, ctx.info.buffer_state.m_BoundTexture[2]);

								// assign flowmap
								IDirect3DBaseTexture9* tex = shaderapi->vtbl->GetD3DTexture(shaderapi, nullptr, ctx.info.buffer_state.m_BoundTexture[4]);
								if (tex)
								{
									ctx.save_texture(dev, 0);
									dev->SetTexture(0, tex);
								}

								// scale water uv
								D3DXMATRIX scaleMatrix; // create a scaling matrix
								D3DXMatrixScaling(&scaleMatrix, 1.5f * ms.water_uv_scale, 1.5f * ms.water_uv_scale, 1.0f);

								ctx.save_ss(dev, D3DSAMP_ADDRESSU);
								ctx.save_ss(dev, D3DSAMP_ADDRESSV);
								dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
								dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

								ctx.set_texture_transform(dev, &scaleMatrix);
								ctx.save_tss(dev, D3DTSS_TEXTURETRANSFORMFLAGS);
								dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
							}

							// ignore 'beneath'
							else
							{
								ctx.modifiers.do_not_render = true;
							}
						}
					}
				}
			}
#endif
		}

		/*if (ctx.info.shader_name.starts_with("Black"))
		{
			int break_me = 0;
		}*/

		// no longer set cam transforms in 'main_module::on_renderview'
		// setting them there causes meshes rendered with shaders to lag behind
		dev->SetTransform(D3DTS_WORLD, &ctx.info.buffer_state.m_Transform[0]);
		dev->SetTransform(D3DTS_VIEW, &ctx.info.buffer_state.m_Transform[1]);
		dev->SetTransform(D3DTS_PROJECTION, &ctx.info.buffer_state.m_Transform[2]);

		// hack for runtime hack: https://github.com/xoxor4d/dxvk-remix/commit/3867843a68db7ec8a5ab603a250689cca1505970
		/*if (static bool runtime_hack_once = false; !runtime_hack_once)
		{
			runtime_hack_once = true;
			set_remix_emissive_intensity(dev, ctx, 0.0f);
		}*/

		// shader: VertexLitGeneric (infected - player model - viewmodel - dynamic props)
		// > models/weapons/melee/crowbar
		// > models/props_junk/wood_palletcrate001a
		// shader: Refract_DX90
		// > vgui/hud/scope_sniper_ul
		if (mesh->m_VertexFormat == 0xa0003)
		{
		}
	}


	HOOK_RETN_PLACE_DEF(cmeshdx8_renderpass_pre_draw_retn_addr);
	void __declspec(naked) cmeshdx8_renderpass_pre_draw_stub()
	{
		__asm
		{
			pushad;
			push	eax; // CPrimList
			push	edi; // CMeshDX8
			call	cmeshdx8_renderpass_pre_draw;
			add		esp, 8;
			popad;

			// og code
			mov     eax, [edi + 0x40];
			test    eax, eax;
			jmp		cmeshdx8_renderpass_pre_draw_retn_addr;
		}
	}

	renderer::renderer()
	{
		p_this = this;

		shared::utils::hook(RENDERER_BASE + 0x3C5E7, cmeshdx8_renderpass_pre_draw_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(cmeshdx8_renderpass_pre_draw_retn_addr, RENDERER_BASE + 0x3C5EC);

		m_initialized = true;
		std::cout << "[RENDERER] loaded\n";
	}
}
