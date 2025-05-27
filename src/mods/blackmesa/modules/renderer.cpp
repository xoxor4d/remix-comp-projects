#include "std_include.hpp"
#include "renderer.hpp"

#include "shared/common/flags.hpp"
#include "shared/common/remix.hpp"

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

		// shader: GBFast
		// > __gbufferfast_brush00
		// > __gbfastpropwrite000

		/*if (ctx.info.material_name.contains("eye"))
		{
			int x = 1; 
		}*/

		if (ctx.info.shader_name == "GBFast") {
			ctx.modifiers.do_not_render = true;
		}
		else if (ctx.info.shader_name == "GBLight") {
			ctx.modifiers.do_not_render = true;
		}

		// EyeRefract_dx9
		else if (mesh->m_VertexFormat == 0x80103)
		{
			if (ctx.info.shader_name.starts_with("Eye"))
			{
				if (const auto basemap2 = shaderapi->vtbl->GetD3DTexture(shaderapi, nullptr, ctx.info.buffer_state.m_BoundTexture[1]); basemap2)
				{
					ctx.save_texture(dev, 0);
					dev->SetTexture(0, basemap2);
				}
			}

			ctx.modifiers.do_not_render = false;
		}

		// VertexLitGeneric
		// > models/props_lab/panel_safe_top_scroll
		else if (mesh->m_VertexFormat == 0x82087) 
		{
			ctx.modifiers.do_not_render = false;
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);
		}

		// WriteZ_DX9
		// > engine/writez
		// screenspace_general_dx9
		// > dev/lumcompare
		// Engine_Post_dx9
		// > dev/engine_post_nxtgen
		// Sky_HDR_DX9
		// > skybox/sky_st_day_01rt
		else if (mesh->m_VertexFormat == 0x80101)
		{
			ctx.modifiers.do_not_render = false; 

			// FIRST "UI/HUD" elem (remix injection triggers here)
			// -> fullscreen color transitions (damage etc.) and also "enables" the crosshair
			if (ctx.info.shader_name.starts_with("Engine_")) // Engine_Post
			{
				// do not fog HUD elements :D
				dev->SetRenderState(D3DRS_FOGENABLE, FALSE);

				// yep this is bad .. todo: view+0x3C to 0x48 = m_FadeColorRGBA
				DWORD* view = reinterpret_cast<DWORD*>(*(DWORD*)(CLIENT_BASE + 0x5390A0));
				void*** g_ViewEffects = reinterpret_cast<void***>(view);
				char r, g, b, a;
				char blend;
				((void(__thiscall*)(void***, char*, char*, char*, char*, char*))(*g_ViewEffects)[2])(g_ViewEffects, &r, &g, &b, &a, &blend);

				const float rr = (float)r * 0.0039215689f;
				const float gg = (float)g * 0.0039215689f;
				const float bb = (float)b * 0.0039215689f;
				const float aa = (float)a * 0.0039215689f;

				ctx.save_vs(dev);
				dev->SetVertexShader(nullptr);
				dev->SetPixelShader(nullptr); // needed

				ctx.save_texture(dev, 0);
				dev->SetTexture(0, nullptr); // disable bound texture

				ctx.save_rs(dev, D3DRS_ALPHABLENDENABLE);
				dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

				ctx.save_rs(dev, D3DRS_BLENDOP);
				dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);

				ctx.save_rs(dev, D3DRS_SRCBLEND);
				dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);

				ctx.save_rs(dev, D3DRS_DESTBLEND);
				dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

				ctx.save_rs(dev, D3DRS_ZWRITEENABLE);
				dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

				ctx.save_rs(dev, D3DRS_ZENABLE);
				dev->SetRenderState(D3DRS_ZENABLE, FALSE);

				dev->SetTransform(D3DTS_WORLD, &ctx.info.buffer_state.m_Transform[0]);
				dev->SetTransform(D3DTS_VIEW, &ctx.info.buffer_state.m_Transform[1]);
				dev->SetTransform(D3DTS_PROJECTION, &ctx.info.buffer_state.m_Transform[2]);

				struct CUSTOMVERTEX
				{
					float x, y, z, rhw;
					D3DCOLOR color;
				};

				auto color = D3DCOLOR_COLORVALUE(rr, gg, bb, aa);
				const auto w = (float)ctx.info.buffer_state.m_Viewport.Width + 0.5f;
				const auto h = (float)ctx.info.buffer_state.m_Viewport.Height + 0.5f;

				CUSTOMVERTEX vertices[] =
				{
					{ -0.5f, -0.5f, 0.0f, 1.0f, color }, // tl
					{     w, -0.5f, 0.0f, 1.0f, color }, // tr
					{ -0.5f,     h, 0.0f, 1.0f, color }, // bl
					{     w,     h, 0.0f, 1.0f, color }  // br
				};

				dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
				dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(CUSTOMVERTEX));

				// do not render the original mesh
				ctx.modifiers.do_not_render = true;
			}
			else if (ctx.info.shader_name.contains("Sky")) 
			{
				ctx.modifiers.do_not_render = false;
				// assign basemap2 to textureslot 0
				/*if (const auto basemap2 = shaderapi->vtbl->GetD3DTexture(shaderapi, nullptr, ctx.info.buffer_state.m_BoundTexture[13]);
					basemap2)
				{
					ctx.save_texture(dev, 0);
					dev->SetTexture(0, basemap2);
					int x = 1;

					ctx.save_vs(dev);
					dev->SetVertexShader(nullptr);
				}*/
			}
		}

		// LightmappedGeneric
		// > metal/ibeam_blue
		else if (  mesh->m_VertexFormat == 0x2480133
				|| mesh->m_VertexFormat == 0x480103 /*decals*/
				|| mesh->m_VertexFormat == 0x480107 /*decals*/)
		{
			//ctx.modifiers.do_not_render = true;

			if (ctx.info.shader_name == "WorldVertexTransition_DX9")
			{
				ctx.save_texture(dev, 0); // helps with culling issue
				ctx.modifiers.dual_render_with_basetexture2 = true;
			}

			// m_BoundTexture[7]  = first blend colormap
			// m_BoundTexture[12] = second blend colormap

			// if envmap		:: VERTEX_TANGENT_S | VERTEX_TANGENT_T | VERTEX_NORMAL is set
			// if basetex2		:: vertex color is set
			// if bumpmap		:: tc count = 3 ... else 2

			// texcoord0 : base texcoord
			// texcoord1 : lightmap texcoord
			// texcoord2 : lightmap texcoord offset

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
			ctx.save_vs(dev); 
			dev->SetVertexShader(nullptr);
		}

		// also renders text 
		// UnlitGeneric
		// > console/background01_widescreen
		else if (mesh->m_VertexFormat == 0x80107)
		{
			ctx.modifiers.do_not_render = false; 
			//ctx.save_vs(dev);
			//dev->SetVertexShader(nullptr);
		}

		// VertexLitGeneric
		// > models/props_inbound/inbound_door
		else if (mesh->m_VertexFormat == 0xa2083)
		{
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);
		}

		// Sprite_DX9
		// > materials/sprites/glow_rendermode_9
		else if (mesh->m_VertexFormat == 0x80105)
		{
			ctx.modifiers.do_not_render = false; 
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);
		}

		// VertexLitGeneric
		// > models/props_am/am_lobby_blastdoors_frame
		else if (  mesh->m_VertexFormat == 0xa0003
				|| mesh->m_VertexFormat == 0xa2087)
		{
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);
		}

		// Spritecard
		// > particle/water/watersplash_001a
		else if (mesh->m_VertexFormat == 0x114900105)
		{
			ctx.modifiers.do_not_render = false; 
		}

		// Spritecard
		// > particle/vistasmokev1/vistasmokev1
		else if (mesh->m_VertexFormat == 0x24914900105)
		{
			ctx.modifiers.do_not_render = false;
		}

		// Refract_DX90
		// > particle/warp1_warp
		else if (mesh->m_VertexFormat == 0x80137)
		{
			ctx.modifiers.do_not_render = false;
		}

		// Water_DX9_HDR
		// > liquids/slime
		else if (mesh->m_VertexFormat == 0x80133)
		{
			ctx.modifiers.do_not_render = false;
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);
		}

		// Cable_DX9
		// > cable/steel
		else if (mesh->m_VertexFormat == 0x480135)
		{
			ctx.modifiers.do_not_render = false;
			//ctx.save_texture(dev, 0);
			//dev->SetTexture(0, tex_addons::black);
		}

		// ShatteredGlass
		// > glass/glasswindowbreak070b
		else if (mesh->m_VertexFormat == 0x2480103)
		{
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);
		}

		// Lightmapped_4WayBlend
		// > maps/bm_c2a4a/nature/blend_4way_bnc_rock_-2208_32_100
		else if (mesh->m_VertexFormat == 0x48013b
			|| mesh->m_VertexFormat == 0x248013b)
		{
			// rendering a second surface works but its not blended with vertex alpha
#if 0
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

				ctx.save_texture(dev, 0);

				if (const auto basemap2 = shaderapi->vtbl->GetD3DTexture(shaderapi, nullptr, ctx.info.buffer_state.m_BoundTexture[2]);
					basemap2)
				{
					ctx.modifiers.dual_render_texture = basemap2; 
				}

				ctx.modifiers.dual_render_with_specified_texture = true;
				ctx.modifiers.dual_render_with_specified_texture_blend_diffuse = true;
			}
#endif
			ctx.save_vs(dev); 
			dev->SetVertexShader(nullptr);
		}

		// LightmappedGeneric
		// > nature/blend_caverock
		else if (mesh->m_VertexFormat == 0x2480137)
		{
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);
		}

		// VertexLitGeneric
		// > decals/smscorch1model
		else if (mesh->m_VertexFormat == 0x82181)
		{
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);
		}

		else 
		{
			auto break_me = 0;   
		}
		//ctx.modifiers.do_not_render = true;  
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



	//void cmeshdx8_renderpass_post_draw(std::uint32_t num_prims_rendered)
	void cmeshdx8_renderpass_post_draw([[maybe_unused]] void* device_ptr, D3DPRIMITIVETYPE type, std::int32_t base_vert_index, std::uint32_t min_vert_index, std::uint32_t num_verts, std::uint32_t start_index, std::uint32_t prim_count)
	{
		const auto dev = game::get_d3d_device();
		const auto shaderapi = game::get_shaderapi();
		auto& ctx = renderer::primctx;

		// 0 = Gamma 1.0 (fixes dark albedo) :: 1 = Gamma 2.2
		dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, ctx.modifiers.with_high_gamma ? 1u : 0u);

		// do not render next surface if set
		if (!ctx.modifiers.do_not_render)
		{
			if (ctx.modifiers.og_mesh_z_offset != 0.0f)
			{
				ctx.info.buffer_state.m_Transform[0].m[3][2] += ctx.modifiers.og_mesh_z_offset;
				dev->SetTransform(D3DTS_WORLD, &ctx.info.buffer_state.m_Transform[0]);
			}

			dev->DrawIndexedPrimitive(type, base_vert_index, min_vert_index, num_verts, start_index, prim_count);

			// restore transform
			if (ctx.modifiers.og_mesh_z_offset != 0.0f)
			{
				ctx.info.buffer_state.m_Transform[0].m[3][2] -= ctx.modifiers.og_mesh_z_offset;
				dev->SetTransform(D3DTS_WORLD, &ctx.info.buffer_state.m_Transform[0]);
			}
		}

		// render the current surface a second time (alpha blended) if set
		// only works with shaders using basemap2 in sampler7
		if (ctx.modifiers.dual_render_with_basetexture2)
		{
			// check if basemap2 is assigned
			if (ctx.info.buffer_state.m_BoundTexture[7])
			{
				// save texture, renderstates and texturestates

				IDirect3DBaseTexture9* og_tex0 = nullptr;
				dev->GetTexture(0, &og_tex0);

				DWORD og_alphablend = {};
				dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &og_alphablend);

				DWORD og_alphaop = {}, og_alphaarg1 = {}, og_alphaarg2 = {};
				dev->GetTextureStageState(0, D3DTSS_ALPHAOP, &og_alphaop);
				dev->GetTextureStageState(0, D3DTSS_ALPHAARG1, &og_alphaarg1);
				dev->GetTextureStageState(0, D3DTSS_ALPHAARG2, &og_alphaarg2);

				DWORD og_colorop = {}, og_colorarg1 = {}, og_colorarg2 = {};
				dev->GetTextureStageState(0, D3DTSS_COLOROP, &og_colorop);
				dev->GetTextureStageState(0, D3DTSS_COLORARG1, &og_colorarg1);
				dev->GetTextureStageState(0, D3DTSS_COLORARG2, &og_colorarg2);

				DWORD og_srcblend = {}, og_destblend = {};
				dev->GetRenderState(D3DRS_SRCBLEND, &og_srcblend);
				dev->GetRenderState(D3DRS_DESTBLEND, &og_destblend);


				// assign basemap2 to textureslot 0
				if (const auto basemap2 = shaderapi->vtbl->GetD3DTexture(shaderapi, nullptr, ctx.info.buffer_state.m_BoundTexture[7]);
					basemap2)
				{
					dev->SetTexture(0, basemap2);
				}

				// enable blending
				dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);

				// picking up / moving a cube affects this and causes flickering on the blended surface
				dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

				// can be used to lighten up the albedo and add a little more alpha
				dev->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(0, 0, 0, 30));
				dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
				dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
				dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD);

				// add a little more alpha
				dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
				dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
				dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_ADD);

				//ctx.info.buffer_state.m_Transform[0].m[3][2] += 0.05f;
				dev->SetTransform(D3DTS_WORLD, &ctx.info.buffer_state.m_Transform[0]);

				// draw second surface 
				dev->DrawIndexedPrimitive(type, base_vert_index, min_vert_index, num_verts, start_index, prim_count);

				// restore texture, renderstates and texturestates
				dev->SetTexture(0, og_tex0);
				dev->SetRenderState(D3DRS_ALPHABLENDENABLE, og_alphablend);
				dev->SetRenderState(D3DRS_SRCBLEND, og_srcblend);
				dev->SetRenderState(D3DRS_DESTBLEND, og_destblend);
				dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, og_alphaarg1);
				dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, og_alphaarg2);
				dev->SetTextureStageState(0, D3DTSS_ALPHAOP, og_alphaop);
				dev->SetTextureStageState(0, D3DTSS_COLORARG1, og_colorarg1);
				dev->SetTextureStageState(0, D3DTSS_COLORARG2, og_colorarg2);
				dev->SetTextureStageState(0, D3DTSS_COLOROP, og_colorop);
			}
		}

		if (ctx.modifiers.dual_render_with_specified_texture)
		{
			// save og texture
			IDirect3DBaseTexture9* og_tex0 = nullptr;
			dev->GetTexture(0, &og_tex0);

			// set new texture
			dev->SetTexture(0, ctx.modifiers.dual_render_texture);

			// BLEND ADD mode
			if (ctx.modifiers.dual_render_with_specified_texture_blend_add) 
			{
				ctx.save_rs(dev, D3DRS_ALPHABLENDENABLE);
				dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

				ctx.save_rs(dev, D3DRS_BLENDOP);
				dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);

				ctx.save_rs(dev, D3DRS_SRCBLEND);
				dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);

				ctx.save_rs(dev, D3DRS_DESTBLEND);
				dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

				ctx.save_rs(dev, D3DRS_ZWRITEENABLE);
				dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

				ctx.save_rs(dev, D3DRS_ZENABLE);
				dev->SetRenderState(D3DRS_ZENABLE, FALSE);

				shared::common::remix::set_texture_category(dev, REMIXAPI_INSTANCE_CATEGORY_BIT_WORLD_MATTE | REMIXAPI_INSTANCE_CATEGORY_BIT_IGNORE_OPACITY_MICROMAP);
			}

			if (ctx.modifiers.dual_render_with_specified_texture_blend_diffuse)
			{
				ctx.save_rs(dev, D3DRS_ALPHABLENDENABLE);
				dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

				ctx.save_rs(dev, D3DRS_BLENDOP);
				dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);

				ctx.save_rs(dev, D3DRS_SRCBLEND);
				dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);

				ctx.save_rs(dev, D3DRS_DESTBLEND);
				dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

				ctx.save_tss(dev, D3DTSS_COLOROP);
				dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);

				ctx.save_tss(dev, D3DTSS_COLORARG1);
				dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

				ctx.save_tss(dev, D3DTSS_COLORARG2);
				dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

				ctx.save_tss(dev, D3DTSS_ALPHAOP);
				dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

				ctx.save_tss(dev, D3DTSS_ALPHAARG1);
				dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

				ctx.save_tss(dev, D3DTSS_ALPHAARG2);
				dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
			}

			if (ctx.modifiers.dual_render_texture_z_offset != 0.0f)
			{
				ctx.info.buffer_state.m_Transform[0].m[3][2] += ctx.modifiers.dual_render_texture_z_offset;
				dev->SetTransform(D3DTS_WORLD, &ctx.info.buffer_state.m_Transform[0]);
			}

			if (ctx.modifiers.as_water)
			{
				shared::common::remix::set_texture_hash(dev, shared::utils::string_hash32(ctx.info.material_name));
			}

			// re-draw surface
			dev->DrawIndexedPrimitive(type, base_vert_index, min_vert_index, num_verts, start_index, prim_count);

			// restore texture
			dev->SetTexture(0, og_tex0);
		}

		//add_light_to_texture_color_restore();

		// reset prim/pass modifications
		renderer::primctx.restore_all(dev);
		renderer::primctx.reset_context();
		dev->SetFVF(NULL);
	}

	HOOK_RETN_PLACE_DEF(cmeshdx8_renderpass_post_draw_retn_addr);
	void __declspec(naked) cmeshdx8_renderpass_post_draw_stub()
	{
		__asm
		{
			// og code
			mov     ecx, [eax];
			push    dword ptr[edi + 0x50];
			push    dword ptr[edi + 0x44];
			push    eax;
			call	cmeshdx8_renderpass_post_draw; // instead of 'edx' (DrawIndexedPrimitive)
			add		esp, 0x1C;
			jmp		cmeshdx8_renderpass_post_draw_retn_addr;
		}
	}



	renderer::renderer()
	{
		p_this = this;

		shared::utils::hook(RENDERER_BASE + 0x3C5E7, cmeshdx8_renderpass_pre_draw_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(cmeshdx8_renderpass_pre_draw_retn_addr, RENDERER_BASE + 0x3C5EC);

		shared::utils::hook(RENDERER_BASE + 0x3C718, cmeshdx8_renderpass_post_draw_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(cmeshdx8_renderpass_post_draw_retn_addr, RENDERER_BASE + 0x3C727);

		// C_FuncAreaPortalWindow::DrawModel :: disable drawing Area Portal Brushmodels
		shared::utils::hook::conditional_jump_to_jmp(CLIENT_BASE + 0xC0210);


		m_initialized = true;
		std::cout << "[RENDERER] loaded\n";
	}
}
