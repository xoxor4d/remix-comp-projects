#include "std_include.hpp"
#include "renderer.hpp"

#include "game_settings.hpp"
#include "imgui.hpp"
#include "interfaces.hpp"
#include "map_settings.hpp"
#include "remix_lights.hpp"
#include "shared/common/flags.hpp"
#include "shared/common/remix.hpp"

namespace mods::blackmesa
{
	namespace cmd
	{
		bool model_info_vis = false;
		bool unbake_model_info_vis = false;
		bool ms_unbake_info = false;
		std::unordered_set<std::string> ms_unbake_info_logged_strings;
	}

	bool renderer::world2screen(const Vector& in, Vector& out)
	{
		auto& matrix = interfaces::get()->m_engine->world_to_screen_matrix();

		out.x = in.Dot(matrix.m[0]) + matrix.m[0][3];
		out.y = in.Dot(matrix.m[1]) + matrix.m[1][3];
		out.z = 0.0f;

		const float perspective_div = in.Dot(matrix.m[3]) + matrix.m[3][3];
		if (perspective_div < 0.001f)
		{
			out.x *= 100000.0f;
			out.y *= 100000.0f;
			return false;
		}

		out.x /= perspective_div;
		out.y /= perspective_div;

		int screen_w, screen_h;
		interfaces::get()->m_engine->get_screen_size(screen_w, screen_h);

		out.x = ((float)screen_w / 2.0f) + (out.x * (float)screen_w) / 2.0f;
		out.y = ((float)screen_h / 2.0f) - (out.y * (float)screen_h) / 2.0f;

		return true;
	}

	bool has_materialvar(game::IMaterialInternal* cmat, const char* var_name, game::IMaterialVar** out_var = nullptr)
	{
		bool found = false;
		const auto var = cmat->vftable->FindVar(cmat, nullptr, var_name, &found, false);

		if (out_var) {
			*out_var = var;
		}

		return found;
	}

	void cmeshdx8_renderpass_pre_draw(game::CMeshDX8* mesh, [[maybe_unused]] /*CPrimList**/ std::uint32_t primlist)
	{
		const auto dev = game::get_d3d_device();

		IDirect3DVertexBuffer9* buffer9 = nullptr;
		UINT stride = 0;
		{
			UINT ofs = 0; dev->GetStreamSource(0, &buffer9, &ofs, &stride);
		}

#if DEBUG
		const auto& im = imgui::get();
#endif

		auto& ctx = renderer::primctx;
		const auto shaderapi = game::get_shaderapi();

		if (ctx.get_info_for_pass(shaderapi))
		{
			{
				if (mesh->m_VertexFormat == 0x480133 || mesh->m_VertexFormat == 0x80133) 
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

									ctx.modifiers.as_water = true;
									ctx.modifiers.og_mesh_z_offset = ms.water_offset_base;

									if (game_settings::get()->enable_dual_layered_water.get_as<bool>())
									{
										ctx.modifiers.dual_render_with_specified_texture = true;
										ctx.modifiers.dual_render_texture_z_offset = ms.water_offset_top;
										ctx.modifiers.dual_render_texture = tex_addons::water_temp; //shaderapi->vtbl->GetD3DTexture(shaderapi, nullptr, ctx.info.buffer_state.m_BoundTexture[2]);
									}

									// assign flowmap
									IDirect3DBaseTexture9* tex = shaderapi->vtbl->GetD3DTexture(shaderapi, nullptr, ctx.info.buffer_state.m_BoundTexture[1]);
									if (tex)
									{
										ctx.save_texture(dev, 0);
										dev->SetTexture(0, tex);
										dev->SetTexture(1, nullptr);
										dev->SetTexture(2, nullptr);
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
			}
		}

		//if (ctx.info.shader_name.contains("Water"))
		//{
		//	int break_me = 0; 
		//}

		// some metal textures just wont load?
		/*if (ctx.info.material_name.contains("metal/r"))
		{
			IDirect3DBaseTexture9* tex = nullptr;
			dev->GetTexture(0, &tex);
			int x = 1;

			if (const auto basemap2 = shaderapi->vtbl->GetD3DTexture(shaderapi, nullptr, ctx.info.buffer_state.m_BoundTexture[im->m_debug_int]);
					basemap2)
			{
				ctx.save_texture(dev, 0);
				dev->SetTexture(0, basemap2);
			}
		}*/

		dev->SetTransform(D3DTS_WORLD, &ctx.info.buffer_state.m_Transform[0]);
		dev->SetTransform(D3DTS_VIEW, &ctx.info.buffer_state.m_Transform[1]);
		dev->SetTransform(D3DTS_PROJECTION, &ctx.info.buffer_state.m_Transform[2]);

		if (ctx.info.shader_name == "GBFast") {
			ctx.modifiers.do_not_render = true; // no longer needed ig
		}
		else if (ctx.info.shader_name == "GBLight") {
			ctx.modifiers.do_not_render = true; // no longer needed ig
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

			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);

#if DEBUG
			if (im->m_debug_disable_rendering[0]) ctx.modifiers.do_not_render = true;
#endif
		}

		// VertexLitGeneric
		// > models/props_lab/panel_safe_top_scroll
		else if (mesh->m_VertexFormat == 0x82087) 
		{
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);

			if (ctx.info.buffer_state.m_Transform[0] == shared::globals::IDENTITY)
			{
				auto wrld = &renderer::get()->m_unbake_transforms_p2w_transform;
				dev->SetTransform(D3DTS_WORLD, wrld);
			}

#if DEBUG
			if (im->m_debug_disable_rendering[1]) ctx.modifiers.do_not_render = true;
#endif
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
			if (ctx.info.shader_name.starts_with("WriteZ")) {
				ctx.modifiers.do_not_render = true;
			}
			/*else
			{
				int x = 1;
			}*/

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

				//ctx.save_vs(dev);
				//dev->SetVertexShader(nullptr);

				ctx.save_rs(dev, D3DRS_FOGENABLE);
				dev->SetRenderState(D3DRS_FOGENABLE, FALSE);

				// this fixes the sky on intros or when no vgui is being drawn
				dev->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);

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

#if DEBUG
				if (im->m_debug_disable_rendering[2]) ctx.modifiers.do_not_render = true;
#endif
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

#if DEBUG
			if (im->m_debug_disable_rendering[3]) ctx.modifiers.do_not_render = true;
#endif
		}

		// also renders text 
		// UnlitGeneric
		// > console/background01_widescreen
		else if (mesh->m_VertexFormat == 0x80107)
		{
			//ctx.save_vs(dev);
			//dev->SetVertexShader(nullptr);

			if (ctx.info.material_name.starts_with("part") && ctx.info.material_name.contains("smoke1_additive"))
			{
				const std::string_view map_name = interfaces::get()->m_engine->get_level_name();
				if (map_name == "maps/bm_c1a0a.bsp")
				{
					ctx.modifiers.do_not_render = true;
				}
				else
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

					ctx.save_vs(dev);
					dev->SetVertexShader(nullptr);
				}
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
			}

#if DEBUG
			if (im->m_debug_disable_rendering[4]) ctx.modifiers.do_not_render = true;
#endif
		}

		// VertexLitGeneric
		// > models/props_inbound/inbound_door
		else if (mesh->m_VertexFormat == 0xa2083)
		{
			if (ctx.info.shader_name.starts_with("Eye"))
			{
				if (const auto basemap2 = shaderapi->vtbl->GetD3DTexture(shaderapi, nullptr, ctx.info.buffer_state.m_BoundTexture[1]); basemap2)
				{
					ctx.save_texture(dev, 0);
					dev->SetTexture(0, basemap2);
				}
			}

			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);

#if DEBUG
			if (im->m_debug_disable_rendering[5]) ctx.modifiers.do_not_render = true;
#endif
		}

		// Sprite_DX9
		// > materials/sprites/glow_rendermode_9
		else if (mesh->m_VertexFormat == 0x80105)
		{
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);

#if DEBUG
			if (im->m_debug_disable_rendering[6]) ctx.modifiers.do_not_render = true;
#endif
		}

		// VertexLitGeneric
		// > models/props_am/am_lobby_blastdoors_frame
		else if (  mesh->m_VertexFormat == 0xa0003
				|| mesh->m_VertexFormat == 0xa0403)
				//|| mesh->m_VertexFormat == 0xa2087)
		{
			//ctx.modifiers.do_not_render = true;
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);

			//if (ctx.info.buffer_state.m_Transform[0] == shared::globals::IDENTITY)
			//{
			//	// holds identity or transposed poseToMesh on unbaked meshes (MapSettings [UNBAKE]) - see R_StudioSoftwareProcessMesh_hk
			//	auto wrld = &renderer::get()->m_unbake_transforms_p2w_transform;

			//	if (*wrld != shared::globals::IDENTITY)
			//	{
			//		int y = 0;
			//		dev->SetTransform(D3DTS_WORLD, wrld);
			//	}

			//	int x = 1;
			//	//dev->SetTransform(D3DTS_WORLD, wrld);

			//}

			// UnlitTwoTexture_DX9 + vol_light...
			if (ctx.info.shader_name.starts_with("Un") && ctx.info.material_name.contains("vol_light")) {
				ctx.modifiers.do_not_render = true;
			} 

#if DEBUG
			if (im->m_debug_disable_rendering[7]) ctx.modifiers.do_not_render = true;
#endif
		}


		else if (mesh->m_VertexFormat == 0xa2087) // ?
		{
			//ctx.modifiers.do_not_render = true;

			//if (ctx.info.buffer_state.m_Transform[0] == shared::globals::IDENTITY)
			{
				auto wrld = &renderer::get()->m_unbake_transforms_p2w_transform;
				dev->SetTransform(D3DTS_WORLD, wrld);
			}

			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);

#if DEBUG
			if (im->m_debug_disable_rendering[8]) ctx.modifiers.do_not_render = true;
#endif
		}

		// Spritecard
		// > particle/water/watersplash_001a
		else if (mesh->m_VertexFormat == 0x114900105)
		{
#if DEBUG
			if (im->m_debug_disable_rendering[9]) ctx.modifiers.do_not_render = true;
#endif
		}

		// Spritecard
		// > particle/vistasmokev1/vistasmokev1
		else if (mesh->m_VertexFormat == 0x24914900105)
		{
#if DEBUG
			if (im->m_debug_disable_rendering[10]) ctx.modifiers.do_not_render = true;
#endif
		}

		// Refract_DX90
		// > particle/warp1_warp
		else if (mesh->m_VertexFormat == 0x80137)
		{
#if DEBUG
			if (im->m_debug_disable_rendering[11]) ctx.modifiers.do_not_render = true;
#endif
		}

		// Water_DX9_HDR
		// > liquids/slime
		else if (mesh->m_VertexFormat == 0x80133)
		{
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);

#if DEBUG
			if (im->m_debug_disable_rendering[12]) ctx.modifiers.do_not_render = true;
#endif
		}

		// Cable_DX9
		// > cable/steel
		else if (mesh->m_VertexFormat == 0x480135)
		{
			//ctx.save_texture(dev, 0);
			//dev->SetTexture(0, tex_addons::black);

#if DEBUG
			if (im->m_debug_disable_rendering[13]) ctx.modifiers.do_not_render = true;
#endif
		}

		// ShatteredGlass
		// > glass/glasswindowbreak070b
		else if (mesh->m_VertexFormat == 0x2480103)
		{
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);

#if DEBUG
			if (im->m_debug_disable_rendering[14]) ctx.modifiers.do_not_render = true;
#endif
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

#if DEBUG
			if (im->m_debug_disable_rendering[15]) ctx.modifiers.do_not_render = true;
#endif
		}

		// LightmappedGeneric
		// > nature/blend_caverock
		else if (mesh->m_VertexFormat == 0x2480137)
		{
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);

#if DEBUG
			if (im->m_debug_disable_rendering[16]) ctx.modifiers.do_not_render = true;
#endif
		}

		// VertexLitGeneric
		// > models/props_xen/rocks/crystals/xen_c4a2a_crystal_1a
		else if (mesh->m_VertexFormat == 0xa2483)
		{
			//ctx.modifiers.do_not_render = true;
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);

#if DEBUG
			if (im->m_debug_disable_rendering[17]) ctx.modifiers.do_not_render = true;
#endif
			}

		// VertexLitGeneric
		// > decals/smscorch1model
		else if (mesh->m_VertexFormat == 0x82181
				|| mesh->m_VertexFormat == 0x3080101)
		{
			//ctx.modifiers.do_not_render = true;
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);

#if DEBUG
			if (im->m_debug_disable_rendering[18]) ctx.modifiers.do_not_render = true;
#endif
		}

		// DecalModulate_dx9
		// > decals/bms_bloodsplatterfloor_002
		else if (mesh->m_VertexFormat == 0x3080103)
		{
			//ctx.modifiers.do_not_render = true;
			ctx.save_vs(dev);
			dev->SetVertexShader(nullptr);

#if DEBUG
			if (im->m_debug_disable_rendering[19]) ctx.modifiers.do_not_render = true;
#endif
		}

#if DEBUG
		else {
			auto break_me = 0;   
		}

		if (im->m_debug_disable_rendering[63]) ctx.modifiers.do_not_render = true;
#endif
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
				//set_remix_texture_hash(dev, ctx, utils::string_hash32(ctx.info.material_name));

				const auto& ms = map_settings::get_map_settings();
				const auto& scale_setting = ms.water_uv_bottom_scale;

				// scale water uv
				D3DXMATRIX scale_matrix;

				if (!shared::utils::float_equal(scale_setting, 0.0f)) // use scale of parent (bottom) water surface if 0
				{
					// restore
					ctx.restore_texture_stage_state(dev, D3DTSS_TEXTURETRANSFORMFLAGS);

					// scale water uv
					D3DXMatrixScaling(&scale_matrix, 1.5f * scale_setting, 1.5f * scale_setting, 1.0f);

					ctx.set_texture_transform(dev, &scale_matrix);
					ctx.save_tss(dev, D3DTSS_TEXTURETRANSFORMFLAGS);
					dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
				}

				if (ms.water_scale_top != 1.0f)
				{
					// scale water surface
					D3DXMatrixScaling(&scale_matrix, ms.water_scale_top, ms.water_scale_top, 1.0f);
					ctx.info.buffer_state.m_Transform[0] = scale_matrix * ctx.info.buffer_state.m_Transform[0];
					dev->SetTransform(D3DTS_WORLD, &ctx.info.buffer_state.m_Transform[0]);
				}
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

	// draw 'nocull' map_setting marker meshes
	void renderer::draw_nocull_markers()
	{
		const auto& ms = map_settings::get_map_settings();

		// early out - nope -> always render a single tri to register tex_addon texture
		if (ms.map_markers.empty()) {
			return;
		}

		struct vertex { D3DXVECTOR3 position; D3DCOLOR color; float tu, tv; };
		const auto dev = game::get_d3d_device();

		// save & restore after drawing
		IDirect3DVertexShader9* og_vs = nullptr;
		dev->GetVertexShader(&og_vs);
		dev->SetVertexShader(nullptr);

		IDirect3DBaseTexture9* og_tex = nullptr;
		dev->GetTexture(0, &og_tex);
		dev->SetTexture(0, tex_addons::white);

		DWORD og_blend;
		dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &og_blend);

		D3DXMATRIX og_tex_transform = {};
		dev->GetTransform(D3DTS_TEXTURE0, &og_tex_transform);

		dev->SetTransform(D3DTS_TEXTURE0, &shared::globals::IDENTITY);
		dev->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

		DWORD og_ff;
		dev->GetFVF(&og_ff);
		dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);

		DWORD og_colop, og_colarg1, og_colarg2;
		dev->GetTextureStageState(0, D3DTSS_COLOROP, &og_colop);
		dev->GetTextureStageState(0, D3DTSS_COLORARG1, &og_colarg1);
		dev->GetTextureStageState(0, D3DTSS_COLORARG2, &og_colarg2);

		dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

		for (auto& m : ms.map_markers)
		{
			// pre_recursive_world_node
			if (m.is_hidden) {
				continue;
			}

			const float f_index = static_cast<float>(m.index);
			const vertex mesh_verts[4] =
			{
				D3DXVECTOR3(-4.1337f - (f_index * 0.01f), -4.1337f - (f_index * 0.01f), 0), D3DCOLOR_COLORVALUE(f_index * 0.001f, f_index * 0.001f, 0.0f,			  1.0f), 0.0f, f_index / 100.0f,
				D3DXVECTOR3( 4.1337f + (f_index * 0.01f), -4.1337f - (f_index * 0.01f), 0), D3DCOLOR_COLORVALUE(0.0f,			  f_index * 0.001f, 0.0f,			  1.0f), f_index / 100.0f, 0.0,
				D3DXVECTOR3( 4.1337f + (f_index * 0.01f),  4.1337f + (f_index * 0.01f), 0), D3DCOLOR_COLORVALUE(0.0f,			  0.0f,				f_index * 0.001f, 1.0f), 0.0f, f_index / 100.0f,
				D3DXVECTOR3(-4.1337f - (f_index * 0.01f),  4.1337f + (f_index * 0.01f), 0), D3DCOLOR_COLORVALUE(f_index,		  0.0f,				f_index * 0.001f, 1.0f), 0.0f, f_index / 100.0f,
			};

			D3DXMATRIX scale_matrix, rotation_x, rotation_y, rotation_z, mat_rotation, mat_translation, world;

			D3DXMatrixScaling(&scale_matrix, m.scale.x, m.scale.y, m.scale.z);
			D3DXMatrixRotationX(&rotation_x, m.rotation.x); // pitch
			D3DXMatrixRotationY(&rotation_y, m.rotation.y); // yaw
			D3DXMatrixRotationZ(&rotation_z, m.rotation.z); // roll
			mat_rotation = rotation_z * rotation_y * rotation_x; // combine rotations (order: Z * Y * X)

			D3DXMatrixTranslation(&mat_translation, m.origin.x, m.origin.y, m.origin.z);
			world = scale_matrix * mat_rotation * mat_translation;

			// set remix texture hash ~req. dxvk-runtime changes - not really needed
			//dev->SetRenderState((D3DRENDERSTATETYPE)150, 100 + m.index);

			dev->SetTransform(D3DTS_WORLD, &world);

			DWORD og_srcblend, og_destblend, og_alphaop, og_alphaarg1, og_alphaarg2;
			if (m.is_blend_marker)
			{
				dev->GetRenderState(D3DRS_SRCBLEND, &og_srcblend);
				dev->GetRenderState(D3DRS_DESTBLEND, &og_destblend);
				dev->GetTextureStageState(0, D3DTSS_ALPHAOP, &og_alphaop);
				dev->GetTextureStageState(0, D3DTSS_ALPHAARG1, &og_alphaarg1);
				dev->GetTextureStageState(0, D3DTSS_ALPHAARG2, &og_alphaarg2);

				dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
				dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
				dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
				dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

				const vertex mesh_verts_alpha[4] =
				{
					D3DXVECTOR3(-8.1337f - (f_index * 0.01f), -8.1337f - (f_index * 0.01f), 0), D3DCOLOR_COLORVALUE(f_index * 0.001f, f_index * 0.001f, 0.0f,			  1.0f), 0.0f, f_index / 100.0f,
					D3DXVECTOR3( 8.1337f + (f_index * 0.01f), -8.1337f - (f_index * 0.01f), 0), D3DCOLOR_COLORVALUE(0.0f,			  f_index * 0.001f, 0.0f,			  1.0f), f_index / 100.0f, 0.0,
					D3DXVECTOR3( 8.1337f + (f_index * 0.01f),  8.1337f + (f_index * 0.01f), 0), D3DCOLOR_COLORVALUE(0.0f,			  0.0f,				f_index * 0.001f, 1.0f), 0.0f, f_index / 100.0f,
					D3DXVECTOR3(-8.1337f - (f_index * 0.01f),  8.1337f + (f_index * 0.01f), 0), D3DCOLOR_COLORVALUE(f_index,		  0.0f,				f_index * 0.001f, 0.0f), 0.0f, f_index / 100.0f,
				};

				dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, mesh_verts_alpha, sizeof(vertex));

				dev->SetRenderState(D3DRS_SRCBLEND, og_srcblend);
				dev->SetRenderState(D3DRS_DESTBLEND, og_destblend);
				dev->SetTextureStageState(0, D3DTSS_ALPHAOP, og_alphaop);
				dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, og_alphaarg1);
				dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, og_alphaarg2);
			}
			else
			{
				dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, mesh_verts, sizeof(vertex));
			}
		}

		// restore
		dev->SetVertexShader(og_vs);
		dev->SetTexture(0, og_tex);
		dev->SetRenderState(D3DRS_ALPHABLENDENABLE, og_blend);

		dev->SetTextureStageState(0, D3DTSS_COLOROP, og_colop);
		dev->SetTextureStageState(0, D3DTSS_COLORARG1, og_colarg1);
		dev->SetTextureStageState(0, D3DTSS_COLORARG2, og_colarg2);

		dev->SetFVF(og_ff);
		dev->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
	}

	void __fastcall tbl_hk::model_renderer::DrawModelExecute::Detour(void* ecx, void* edx, const game::DrawModelState_t& state, const game::ModelRenderInfo_t& pInfo, shared::matrix3x4_t* pCustomBoneToWorld)
	{
		// draw nocull markers before drawing the first model - no particular reason besides that we dont want to draw them before rendering the sky
		if (*game::get_current_view_id() != game::VIEW_3DSKY && !renderer::get()->m_drew_model)
		{
			renderer::draw_nocull_markers();
			renderer::get()->m_drew_model = true;
		}

		bool ignore = false;
		const auto& hmsettings = map_settings::get_map_settings().hide_models;

		for (const auto& hide_mdl_with_radius : hmsettings.radii)
		{
			if (pInfo.pModel->radius == hide_mdl_with_radius)
			{
				ignore = true;
				break;
			}
		}

		if (!ignore && !hmsettings.substrings.empty())
		{
			const auto mdl_string = std::string_view(pInfo.pModel->szPathName);
			for (const auto& hide_mdl_with_substr : hmsettings.substrings)
			{
				if (mdl_string.contains(hide_mdl_with_substr))
				{
					ignore = true;
					break;
				}
			}
		}

		// check for attached lights
		remix_lights::on_draw_model_exec(pInfo);

		if (!ignore)
		{
			// draw the model
			tbl_hk::model_renderer::table.original<FN>(Index)(ecx, edx, state, pInfo, pCustomBoneToWorld);

			if (cmd::model_info_vis)
			{
				if (game::get_current_view_origin()->DistToSqr(pInfo.origin) < 1000.0f * 1000.0f)
				{
					game::debug_add_text_overlay(&pInfo.origin.x, pInfo.pModel->szPathName, 0, 1.0f, 1.0f, 1.0f, 1.0f);
					game::debug_add_text_overlay(&pInfo.origin.x, shared::utils::va("Radius: %.7f", pInfo.pModel->radius), 1, 1.0f, 1.0f, 1.0f, 1.0f);
					game::debug_add_text_overlay(&pInfo.origin.x, shared::utils::va("Origin: %.7f %.7f %.7f", pInfo.origin.x, pInfo.origin.y, pInfo.origin.z), 2, 1.0f, 1.0f, 1.0f, 1.0f);
				}
			}
		}
		else
		{
			if (cmd::model_info_vis)
			{
				if (game::get_current_view_origin()->DistToSqr(pInfo.origin) < 1000.0f * 1000.0f)
				{
					game::debug_add_text_overlay(&pInfo.origin.x, "#IGNORED#", 0, 1.0f, 0.6f, 0.6f, 0.6f);
					game::debug_add_text_overlay(&pInfo.origin.x, pInfo.pModel->szPathName, 1, 1.0f, 0.6f, 0.6f, 0.6f);
					game::debug_add_text_overlay(&pInfo.origin.x, shared::utils::va("Radius: %.7f", pInfo.pModel->radius), 2, 1.0f, 0.6f, 0.6f, 0.6f);
				}
			}
		}
	}

	namespace unbake_transform
	{
#if 0
		void transpose_matrix3x4_to_d3dxmatrix(const shared::matrix3x4_t& src, D3DXMATRIX& dest)
		{
			dest.m[0][0] = src.m_flMatVal[0][0];
			dest.m[0][1] = src.m_flMatVal[1][0];
			dest.m[0][2] = src.m_flMatVal[2][0];
			dest.m[0][3] = 0.0f;

			dest.m[1][0] = src.m_flMatVal[0][1];
			dest.m[1][1] = src.m_flMatVal[1][1];
			dest.m[1][2] = src.m_flMatVal[2][1];
			dest.m[1][3] = 0.0f;

			dest.m[2][0] = src.m_flMatVal[0][2];
			dest.m[2][1] = src.m_flMatVal[1][2];
			dest.m[2][2] = src.m_flMatVal[2][2];
			dest.m[2][3] = 0.0f;

			dest.m[3][0] = src.m_flMatVal[0][3];
			dest.m[3][1] = src.m_flMatVal[1][3];
			dest.m[3][2] = src.m_flMatVal[2][3];
			dest.m[3][3] = 1.0f;
		}

		shared::matrix3x4_t og_pose = {}; 
		void R_StudioDrawPoints_hk([[maybe_unused]] void* mesh_data, game::mstudiomodel_t* sub_model) // studiomeshdata_t* mesh_data
		{
			auto& unbake_transform = renderer::get()->m_unbake_transforms_on_next_static_prop;
			unbake_transform = false; // always reset

			if (imgui::get()->m_debug_disable_unbake) {
				return;
			}

			const auto model_str = std::string_view(sub_model->name);

			if (cmd::ms_unbake_info) {
				cmd::ms_unbake_info_logged_strings.insert(model_str);
			}

			if (const auto& unbake_model_names = map_settings::get_map_settings().unbake_models;
				!unbake_model_names.empty())
			{
				for (const auto& unbake_mdl_str : unbake_model_names)
				{
					if (model_str.contains(unbake_mdl_str))
					{
						unbake_transform = true;
						break;
					}
				}
			}
		}

		DWORD R_StudioDrawPoints_pSubModel_addr = 0u;
		HOOK_RETN_PLACE_DEF(R_StudioDrawPoints_retn_addr);
		void __declspec(naked) R_StudioDrawPoints_stub()
		{
			__asm
			{
				mov		R_StudioDrawPoints_pSubModel_addr, eax; // save addr
				mov     eax, [esi + 0xB8]; // og

				pushad;
				push	R_StudioDrawPoints_pSubModel_addr;
				push	eax;
				call	R_StudioDrawPoints_hk;
				add		esp, 8;
				popad;

				// og
				mov     eax, [esi + 0xB8];
				jmp		R_StudioDrawPoints_retn_addr;
			}
		}

		// do not bake position/normals into vertices of "dynamic" static props
		void R_StudioSoftwareProcessMesh_hk(shared::matrix3x4_t* pose_to_world)
		{
			og_pose = *pose_to_world;

			if (imgui::get()->m_debug_disable_unbake) {
				return;
			}

			auto& unbake_transform = renderer::get()->m_unbake_transforms_on_next_static_prop;
			if (!unbake_transform)
			{
				renderer::get()->m_unbake_transforms_p2w_transform = shared::globals::IDENTITY;
				return;
			}

			auto& wrld = renderer::get()->m_unbake_transforms_p2w_transform;
			transpose_matrix3x4_to_d3dxmatrix(*pose_to_world, wrld);

			pose_to_world->m_flMatVal[0][0] = 1.0f;
			pose_to_world->m_flMatVal[0][1] = 0.0f;
			pose_to_world->m_flMatVal[0][2] = 0.0f;
			pose_to_world->m_flMatVal[0][3] = 0.0f; // transform x

			pose_to_world->m_flMatVal[1][0] = 0.0f;
			pose_to_world->m_flMatVal[1][1] = 1.0f;
			pose_to_world->m_flMatVal[1][2] = 0.0f;
			pose_to_world->m_flMatVal[1][3] = 0.0f; // transform y

			pose_to_world->m_flMatVal[2][0] = 0.0f;
			pose_to_world->m_flMatVal[2][1] = 0.0f;
			pose_to_world->m_flMatVal[2][2] = 1.0f;
			pose_to_world->m_flMatVal[2][3] = 0.0f; // transform z
		}

		// works but issues
		//HOOK_RETN_PLACE_DEF(R_StudioSoftwareProcessMesh_retn_addr);
		//void __declspec(naked) R_StudioSoftwareProcessMesh_stub()
		//{
		//	__asm
		//	{
		//		push    esi;
		//		push    edi;
		//		mov     ecx, [ebx + 8];
		//		mov     eax, [ebx + 0xC]; // poseToWorld

		//		pushad;
		//		push	eax;
		//		call	R_StudioSoftwareProcessMesh_hk;
		//		add		esp, 4;
		//		popad;

		//		mov     ecx, [ebx + 8];
		//		mov     eax, [ebx + 0xC]; // poseToWorld
		//		jmp		R_StudioSoftwareProcessMesh_retn_addr;
		//	}
		//}

		HOOK_RETN_PLACE_DEF(R_StudioSoftwareProcessMesh_retn_addr);
		void __declspec(naked) R_StudioSoftwareProcessMesh_stub()
		{
			__asm
			{

				mov		eax, dword ptr[esi + 0xA0]; // poseToWorld
				pushad;
				push	eax;
				call	R_StudioSoftwareProcessMesh_hk;
				add		esp, 4;
				popad;

				push    dword ptr[esi + 0xA0];
				jmp		R_StudioSoftwareProcessMesh_retn_addr;
			}
		}


		void R_StudioSoftwareProcessMesh_Restore_hk(shared::matrix3x4_t* pose_to_world) {
			*pose_to_world = og_pose;
		}

		// works but issue
		//void __declspec(naked) R_StudioSoftwareProcessMesh_Restore_stub()
		//{
		//	__asm
		//	{
		//		pushad;

		//		mov     eax, [ebx + 0xC];
		//		push    eax;
		//		call	R_StudioSoftwareProcessMesh_Restore_hk;
		//		add		esp, 4;
		//		popad;

		//		// og
		//		mov     esp, ebx;
		//		pop     ebx;
		//		retn;
		//	}
		//}

		void __declspec(naked) R_StudioSoftwareProcessMesh_Restore_stub()
		{
			__asm
			{
				pushad;
				mov		eax, dword ptr[esi + 0xA0]; // poseToWorld
				push    eax;
				call	R_StudioSoftwareProcessMesh_Restore_hk;
				add		esp, 4;
				popad;

				// og
				pop     edi;
				pop     esi;
				mov     esp, ebp;
				pop     ebp;
				retn    0x28;
			}
		}
		void R_StudioRenderFinal_hk() {
			renderer::get()->m_unbake_transforms_p2w_transform = shared::globals::IDENTITY;
		}

		void __declspec(naked) R_StudioRenderFinal_stub()
		{
			__asm
			{
				pushad;
				call	R_StudioRenderFinal_hk;
				popad;

				// og
				mov     eax, edi;
				pop     edi;
				pop     esi;
				pop     ebx;
				pop     ebp;
				retn    0x28;
			}
		}
#endif







		struct studiohdr_t
		{
			int id;
			int version;
			int checksum;
			char name[64];
			int length;
			Vector eyeposition;
			Vector illumposition;
			Vector hull_min;
			Vector hull_max;
			Vector view_bbmin;
			Vector view_bbmax;
			int flags;
			int numbones;
			int boneindex;
		}; STATIC_ASSERT_OFFSET(studiohdr_t, numbones, 0x9C);

		struct CStudioRender
		{
			//char pad[0xB0];
			char pad[0x6C];
			shared::matrix3x4_t m_StaticPropRootToWorld;
			shared::matrix3x4_t* m_pBoneToWorld;
			shared::matrix3x4_t* m_PoseToWorld;
			int pad2[3];
			studiohdr_t* m_pStudioHdr;
			game::mstudiomodel_t* sub_model;
		}; STATIC_ASSERT_OFFSET(CStudioRender, m_pStudioHdr, 0xB0);

		int R_StudioDrawStaticMesh_hk(CStudioRender* studio)
		{
			if (imgui::get()->m_debug_disable_unbake) {
				return 0;
			}

			const auto model_str = std::string_view(studio->sub_model->name);

			bool requires_unbake = false;
			const auto& unbake_model_names = map_settings::get_map_settings().unbake_models;

			// check for unbake checksums
			if (!unbake_model_names.checksums.empty())
			{
				for (const auto& unbake_mdl_checksum : unbake_model_names.checksums)
				{
					if (unbake_mdl_checksum == studio->m_pStudioHdr->checksum)
					{
						requires_unbake = true;
						break;
					}
				}
			}

			// check for unbake strings if no checksum matched
			if (!requires_unbake && !unbake_model_names.strings.empty())
			{
				for (const auto& unbake_mdl_str : unbake_model_names.strings)
				{
					if (model_str.contains(unbake_mdl_str))
					{
						requires_unbake = true;
						break;
					}
				}
			}

			if (cmd::ms_unbake_info)
			{
				std::string str = std::string(model_str) + " --- checksum: " + shared::utils::to_hex_string(studio->m_pStudioHdr->checksum);
				cmd::ms_unbake_info_logged_strings.insert(str);
			}

			if (cmd::unbake_model_info_vis)
			{
				const Vector org = { studio->m_PoseToWorld->m_flMatVal[0][3], studio->m_PoseToWorld->m_flMatVal[1][3], studio->m_PoseToWorld->m_flMatVal[2][3] };
				if (game::get_current_view_origin()->DistToSqr(org) < 1000.0f * 1000.0f)
				{
					if (requires_unbake)
					{
						game::debug_add_text_overlay(&org.x, "#UNBAKED#", 0, 1.0f, 0.6f, 0.6f, 0.6f);
					}
					game::debug_add_text_overlay(&org.x, studio->sub_model->name, 1, 1.0f, 1.0f, 1.0f, 1.0f);
					game::debug_add_text_overlay(&org.x, shared::utils::va("Checksum: %sf", shared::utils::to_hex_string(studio->m_pStudioHdr->checksum).c_str()), 2, 1.0f, 1.0f, 1.0f, 1.0f);
				}
			}

			/*if (studio->m_pStudioHdr->numbones <= 1) 
			{
				return 0;
			}*/

			return requires_unbake;
		}

		HOOK_RETN_PLACE_DEF(R_StudioDrawStaticMesh_og_retn_addr);
		HOOK_RETN_PLACE_DEF(R_StudioDrawStaticMesh_nop_retn_addr);
		void __declspec(naked) R_StudioDrawStaticMesh_stub()
		{
			__asm
			{
				pushad;
				push	edi; // CStudioRender
				call	R_StudioDrawStaticMesh_hk;
				add		esp, 4;

				cmp		eax, 1;
				je		SKIP_CHECK;	// jmp if eax = 1
				popad;

				// og
				mov     eax, [edi + 4];
				test    byte ptr[eax + 0x24], 2;
				jmp		R_StudioDrawStaticMesh_og_retn_addr;

			SKIP_CHECK:
				popad;

				mov     eax, [edi + 4]; // og
				jmp		R_StudioDrawStaticMesh_nop_retn_addr;
			}
		}
	}

	// called from imgui::on_present
	void renderer::on_present()
	{
		if (cmd::ms_unbake_info)
		{
			cmd::ms_unbake_info = false;
			std::filesystem::create_directories(shared::globals::root_path + "\\rtx_comp\\logs\\");

			std::ofstream file;
			file.open((shared::globals::root_path + "\\rtx_comp\\logs\\mapsettings_unbake_info.log").c_str());

			file << "MapSettings [UNBAKE] : Logfile containing names of models that were drawn in the capture frame." << "\n\n";

			for (const auto& str : cmd::ms_unbake_info_logged_strings) {
				file << str << "\n";
			}

			file.close();
			cmd::ms_unbake_info_logged_strings.clear();
		}
	}

	game::ConCommand xo_debug_toggle_model_info_cmd{};
	void xo_debug_toggle_model_info_fn()
	{
		cmd::model_info_vis = !cmd::model_info_vis;
	}

	game::ConCommand xo_debug_toggle_unbake_model_info_cmd{};
	void xo_debug_toggle_unbake_model_info_fn()
	{
		cmd::unbake_model_info_vis = !cmd::unbake_model_info_vis;
	}

	game::ConCommand xo_mapsettings_get_unbake_info_cmd{};
	void xo_mapsettings_get_unbake_info_fn()
	{
		cmd::ms_unbake_info = true;
	}

	renderer::renderer()
	{
		p_this = this;

		tbl_hk::model_renderer::_interface = shared::utils::module_interface.get<tbl_hk::model_renderer::IVModelRender*>("engine.dll", "VEngineModel016");
		XASSERT(tbl_hk::model_renderer::table.init(tbl_hk::model_renderer::_interface) == false);
		XASSERT(tbl_hk::model_renderer::table.hook(&tbl_hk::model_renderer::DrawModelExecute::Detour, tbl_hk::model_renderer::DrawModelExecute::Index) == false);

		shared::utils::hook(RENDERER_BASE + 0x3C5E7, cmeshdx8_renderpass_pre_draw_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(cmeshdx8_renderpass_pre_draw_retn_addr, RENDERER_BASE + 0x3C5EC);

		shared::utils::hook(RENDERER_BASE + 0x3C718, cmeshdx8_renderpass_post_draw_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(cmeshdx8_renderpass_post_draw_retn_addr, RENDERER_BASE + 0x3C727);

		// C_FuncAreaPortalWindow::DrawModel :: disable drawing Area Portal Brushmodels
		shared::utils::hook::conditional_jump_to_jmp(CLIENT_BASE + 0xC0210);

		// Sky_HDR_DX9 :: GetFallbackShader -> ignore dxlevel and always fall back to Sky_DX9
		shared::utils::hook::conditional_jump_to_jmp(STDSHADERDX9_BASE + 0x7B1EE);

		// Water_DX9_HDR :: GetFallbackShader ^
		shared::utils::hook::nop(STDSHADERDX9_BASE + 0x93A35, 5);

		// Shader_WorldEnd - skip render flashlight stuff before drawing the sky
		shared::utils::hook::conditional_jump_to_jmp(ENGINE_BASE + 0x10D060);
		shared::utils::hook::conditional_jump_to_jmp(ENGINE_BASE + 0x10D07C);


#if 0
		// Remove world-position baking for vertices of "dynamic" static props and use SetTransform(WORLD) to transform them into the world.
		// This results in:
		// - affected mesh instances having the same (remix) hash
		// - stable hashes for some non-animated props (cube)

		// CStudioRender::R_StudioRenderFinal -> 
		// CStudioRender::R_StudioDrawPoints -> 
		// CStudioRender::R_StudioDrawMesh -> 
		// CStudioRender::R_StudioDrawStaticMesh ->
		// CStudioRender::R_StudioSoftwareProcessMesh -> 
		// CProcessMeshWrapper<0,0,0>::R_StudioSoftwareProcessMesh (hooked)
		// :: transpose pPoseToWorld and use it as world-transform in 'cmeshdx8_renderpass_pre_draw'
		// :: set pPoseToWorld to identity to remove position/normal baking


		// works but issues
		//shared::utils::hook(STUDIORENDER_BASE + 0x15C86, unbake_transform::R_StudioSoftwareProcessMesh_stub, HOOK_JUMP).install()->quick(); // y
		//HOOK_RETN_PLACE(unbake_transform::R_StudioSoftwareProcessMesh_retn_addr, STUDIORENDER_BASE + 0x15C8E); // y

		shared::utils::hook::nop(STUDIORENDER_BASE + 0x1BBA1, 6);
		shared::utils::hook(STUDIORENDER_BASE + 0x1BBA1, unbake_transform::R_StudioSoftwareProcessMesh_stub, HOOK_JUMP).install()->quick(); // y
		HOOK_RETN_PLACE(unbake_transform::R_StudioSoftwareProcessMesh_retn_addr, STUDIORENDER_BASE + 0x1BBA7); // y


		// restore pPoseToWorld after building the mesh ^
		// works but issues
		//shared::utils::hook(STUDIORENDER_BASE + 0x1600E, unbake_transform::R_StudioSoftwareProcessMesh_Restore_stub, HOOK_JUMP).install()->quick(); // y hope

		shared::utils::hook(STUDIORENDER_BASE + 0x1BBB4, unbake_transform::R_StudioSoftwareProcessMesh_Restore_stub, HOOK_JUMP).install()->quick(); // y hope



		// CStudioRender::R_StudioRenderFinal
		// :: some meshes are made up of multiple submodels or body parts, so 'cmeshdx8_renderpass_pre_draw' gets called multiple times
		// :: we need to set the modified world-transform back to identity after we are done rendering the mesh to not affect subsequent meshes
		shared::utils::hook(STUDIORENDER_BASE + 0x118F7, unbake_transform::R_StudioRenderFinal_stub, HOOK_JUMP).install()->quick(); // y

		// CStudioRender::R_StudioDrawPoints
		// :: get info about the current mesh and decide if we will be fixing the baked transform or not
		shared::utils::hook::nop(STUDIORENDER_BASE + 0x109AE, 6); // y
		shared::utils::hook(STUDIORENDER_BASE + 0x109AE, unbake_transform::R_StudioDrawPoints_stub, HOOK_JUMP).install()->quick(); // y
		HOOK_RETN_PLACE(unbake_transform::R_StudioDrawPoints_retn_addr, STUDIORENDER_BASE + 0x109B4); // y
#endif

		shared::utils::hook::nop(STUDIORENDER_BASE + 0x10AC0, 7);
		shared::utils::hook(STUDIORENDER_BASE + 0x10AC0, unbake_transform::R_StudioDrawStaticMesh_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(unbake_transform::R_StudioDrawStaticMesh_og_retn_addr, STUDIORENDER_BASE + 0x10AC7);
		HOOK_RETN_PLACE(unbake_transform::R_StudioDrawStaticMesh_nop_retn_addr, STUDIORENDER_BASE + 0x10ACD);



		game::con_add_command(&xo_debug_toggle_model_info_cmd, "xo_debug_toggle_model_info", xo_debug_toggle_model_info_fn, "Toggle model name and radius visualizations");
		game::con_add_command(&xo_debug_toggle_unbake_model_info_cmd, "xo_debug_toggle_unbake_model_info", xo_debug_toggle_unbake_model_info_fn, "Draw model name checksums for [UNBAKE] (mapsettings)");

		game::con_add_command(&xo_mapsettings_get_unbake_info_cmd, "xo_mapsettings_get_unbake_info", xo_mapsettings_get_unbake_info_fn, "This log names of drawn models in the current frame to a logfile in portal2-rtx/logs/. Useful for MapSettings : [UNBAKE]");

		m_initialized = true;
		std::cout << "[RENDERER] loaded\n";
	}
}
