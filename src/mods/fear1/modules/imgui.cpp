#include "std_include.hpp"
#include "imgui.hpp"

#include "shared/imgui/imgui_helper.hpp"
#include "shared/imgui/font_awesome_solid_900.hpp"
#include "shared/imgui/font_defines.hpp"
#include "shared/imgui/font_opensans.hpp"

// Allow us to directly call the ImGui WndProc function.
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

#define SPACING_INDENT_BEGIN ImGui::Spacing(); ImGui::Indent()
#define SPACING_INDENT_END ImGui::Spacing(); ImGui::Unindent()
#define TT(TXT) ImGui::SetItemTooltipBlur((TXT));

#define SET_CHILD_WIDGET_WIDTH			ImGui::SetNextItemWidth(ImGui::CalcWidgetWidthForChild(80.0f));
#define SET_CHILD_WIDGET_WIDTH_MAN(V)	ImGui::SetNextItemWidth(ImGui::CalcWidgetWidthForChild((V)));

namespace mods::fear1
{
	WNDPROC g_game_wndproc = nullptr;
	
	LRESULT __stdcall wnd_proc_hk(HWND window, UINT message_type, WPARAM wparam, LPARAM lparam)
	{
		bool pass_msg_to_game = false;

		if (imgui::get()->input_message(message_type, wparam, lparam, pass_msg_to_game) /*&& !pass_msg_to_game*/) {
			return true;
		}

		/*if (imgui::get()->is_imgui_game_input_allowed()) {
			if (message_type == WM_MOUSEMOVE) {
				return true;
			}
		}*/

		//game::console(); printf("MSG 0x%x -- w: 0x%x -- l: 0x%x\n", message_type, wparam, lparam);
		return CallWindowProc(g_game_wndproc, window, message_type, wparam, lparam);
	}

	bool imgui::input_message(const UINT message_type, const WPARAM wparam, const LPARAM lparam, [[maybe_unused]] bool& inout_pass_msg_to_game)
	{
		if (message_type == WM_KEYUP && wparam == VK_F4) 
		{
			const auto& io = ImGui::GetIO();
			if (!io.MouseDown[1]) 
			{
				m_menu_active = !m_menu_active;

				if (game::game_client)
				{
					// freezes game input but keeps mouse locked ...
					/*auto some_glob = shared::utils::hook::call<void* (__cdecl)()>(game::game_client_addr + 0xBAD0)();
					*((BYTE*)some_glob + 76) = !m_menu_active;*/

					// later useful for flashlight - keep!
					/*auto g_pPlayerMgr = reinterpret_cast<void*>(game::game_client_addr + 0x1ACAC4);
					shared::utils::hook::call<void(__fastcall)(void* this_ptr, int unused, bool bAllowInput, bool bRestoreBackupAngles)>
						(game::game_client_addr + 0xAFDC0)(g_pPlayerMgr, 0, !m_menu_active, true);*/

					// calls the actual pause function but still locks the mouse (PauseGameActual) // usage: 0x10062A2D
					int asd = *reinterpret_cast<int*>(0x101A4FE8);
					(*(void(__thiscall**)(int, int, int))(*(DWORD*)asd + 0x7C))(asd, m_menu_active, true);

					// only center cursor when ingame (not main menu)
					//auto x = *reinterpret_cast<int*>(*(DWORD*)(0x56FA34) + 0x18);
					if (*reinterpret_cast<int*>(*(DWORD*)(0x56FA34) + 0x18))
					{
						// set CV_CursorCenter cvar used in the msg loop to center the cursor
						game::center_cursor(!m_menu_active);//
					}

					// set CV_CursorCenter cvar used in the msg loop to center the cursor
					//game::center_cursor(!m_menu_active);
				}
			}

			else {
				ImGui_ImplWin32_WndProcHandler(shared::globals::main_window, message_type, wparam, lparam);
			}
		}

		//if (game::game_client)
		//{
		//	//auto xasd = (DWORD)game::game_client + 0xBAD0; 
		//	// utils::hook::call<BOOL(__stdcall)(HWND, UINT, WPARAM, LPARAM)>(0x57BB20)(hWnd, Msg, wParam, lParam);
		//	auto some_glob = shared::utils::hook::call<void*(__cdecl)()>(game::game_client_addr + 0xBAD0)();
		//	//auto some_glob = reinterpret_cast<void*>(game::game_client + 0xBAD0);
		//	*((BYTE*)some_glob + 76) = !m_menu_active;
		//}

		if (m_menu_active)
		{
			auto& io = ImGui::GetIO();

			//if (!(message_type == WM_MOUSEMOVE || message_type == WM_NCMOUSEMOVE)) {
				ImGui_ImplWin32_WndProcHandler(shared::globals::main_window, message_type, wparam, lparam);
			//}

			// enable game input if no imgui window is hovered and right mouse is held
			if (!m_im_window_hovered && io.MouseDown[1])
			{
				ImGui::SetWindowFocus(); // unfocus input text
				m_im_allow_game_input = true;

				int asd = *reinterpret_cast<int*>(0x101A4FE8);
				(*(void(__thiscall**)(int, int, int))(*(DWORD*)asd + 0x7C))(asd, false, false);

				// we cant use the mouse anyway for whatever reason
				//game::center_cursor(true); // set CV_CursorCenter cvar used in the msg loop to center the cursor

				//io.MouseDrawCursor = false;
				return false;
			}

			// ^ wait until mouse is up and call set_cursor_always_visible once
			if (m_im_allow_game_input && !io.MouseDown[1])
			{
				m_im_allow_game_input = false;

				int asd = *reinterpret_cast<int*>(0x101A4FE8);
				(*(void(__thiscall**)(int, int, int))(*(DWORD*)asd + 0x7C))(asd, true, true);

				// we cant use the mouse anyway for whatever reason
				//game::center_cursor(false); // set CV_CursorCenter cvar used in the msg loop to center the cursor

				//io.MouseDrawCursor = true;
				return false;
			}

			//if (io.WantCaptureMouse)
			//{
			//	m_im_allow_game_input = false;
			//	//inout_pass_msg_to_game = true;
			//	return true;
			//}
		}
		else {
			m_im_allow_game_input = false; // always reset if there is no imgui window open
		}

		return m_menu_active;
	}

	// ------

	void cont_general_quickcommands()
	{
		const auto& im = imgui::get();

		{
			static float cont_var_height = 0.0f;
			cont_var_height = ImGui::Widget_ContainerWithCollapsingTitle("Game Variables", cont_var_height, [&]
				{
					static bool prevent_overrides = false;
					if (ImGui::Checkbox("Disable constant var overrides", &prevent_overrides)) {
						shared::utils::hook::set<BYTE>(0x5009CA, prevent_overrides ? 0xEB : 0x74);
					} TT("Disables the per frame override of game variables. Some of the below options can not be tweaked without this.");

					{ auto* var = reinterpret_cast<bool*>(0x56D124); ImGui::Checkbox("FogEnable", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D214); ImGui::Checkbox("DrawTranslucent", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D274); ImGui::Checkbox("DrawWorldModelPhysicsDims", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D28C); ImGui::Checkbox("DrawWorldTree", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D2A4); ImGui::Checkbox("DrawGroupedBatchesOnly", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D2BC); ImGui::Checkbox("DisableBatchGrouping", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D2D4); ImGui::Checkbox("DrawSkyPortals", var); }

					{ auto* var = reinterpret_cast<bool*>(0x56D2EC); ImGui::Checkbox("VisDrawPortals", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D304); ImGui::Checkbox("VisLock", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D31C); ImGui::Checkbox("VisDrawFrustum", var); }
					{ auto* var = reinterpret_cast<int*>(0x56D334); ImGui::DragInt("VisMaxSectorDepth", var); }

					{ auto* var = reinterpret_cast<bool*>(0x56D34C); ImGui::Checkbox("VisDisableWhenOutside", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D364); ImGui::Checkbox("VisDrawModelDims", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D37C); ImGui::Checkbox("VisDrawLightDims", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D394); ImGui::Checkbox("VisDrawWorldModelDims", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D3AC); ImGui::Checkbox("VisDrawCustomRenderDims", var); }

					{ auto* var = reinterpret_cast<bool*>(0x56D3C4); ImGui::Checkbox("VisDrawWorldBlocks", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D3DC); ImGui::Checkbox("VisDrawSubWorldBlocks", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D3F4); ImGui::Checkbox("VisDrawSectorDims", var); }

					{ auto* var = reinterpret_cast<float*>(0x56D6C4); ImGui::DragFloat("NearZ", var); }
					{ auto* var = reinterpret_cast<float*>(0x56D6DC); ImGui::DragFloat("FarZ", var); }

					{ auto* var = reinterpret_cast<int*>(0x56D844); ImGui::DragInt("ModelLODOffset", var); }
					{ auto* var = reinterpret_cast<float*>(0x56D85C); ImGui::DragFloat("ModelLODDistanceScale", var); }
					{ auto* var = reinterpret_cast<float*>(0x56D874); ImGui::DragFloat("ModelLODDistanceBias", var); }

					{ auto* var = reinterpret_cast<bool*>(0x56D88C); ImGui::Checkbox("ModelDebug_DrawSkeleton", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D8A4); ImGui::Checkbox("DrawModels", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D8BC); ImGui::Checkbox("DrawWorld", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D8D4); ImGui::Checkbox("DrawSky", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D8EC); ImGui::Checkbox("DrawWorldModels", var); }

					{ auto* var = reinterpret_cast<bool*>(0x56D904); ImGui::Checkbox("DrawCustomRender", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D91C); ImGui::Checkbox("DrawFogVolumes", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D934); ImGui::Checkbox("DrawModelDecals", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56D94C); ImGui::Checkbox("Wireframe", var); }

					{ auto* var = reinterpret_cast<bool*>(0x56D964); ImGui::Checkbox("DisablePrimitiveRendering", var); }
					{ auto* var = reinterpret_cast<float*>(0x56D9AC); ImGui::DragFloat("SkyFarZ", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56DA54); ImGui::Checkbox("ShaderDisablePreshaders", var); }
					{ auto* var = reinterpret_cast<bool*>(0x56DA6C); ImGui::Checkbox("ShaderDisableCompiled", var); }

					{ auto* var = reinterpret_cast<int*>(0x56DAB4); ImGui::DragInt("LODMaterials", var); }

				}, false, ICON_FA_ELLIPSIS_H, &im->ImGuiCol_ContainerBackground, &im->ImGuiCol_ContainerBorder);
		}

		{
			static float cont_cull_height = 0.0f;
			cont_cull_height = ImGui::Widget_ContainerWithCollapsingTitle("Culling Tweaks", cont_cull_height, [&]
			{
				{
					auto* var = reinterpret_cast<bool*>(0x56D31C); // VisDrawFrustum
					ImGui::Checkbox("Draw everything in Frustum (VisDrawFrustum var)", var); TT("This will draw everything infront of the player");

					// set num planes for a culling func from 5 to 1 if VisDrawFrustum is enabled
					shared::utils::hook::set<BYTE>(0x5190DC + 1, var ? 1u : 5u);
				}

				ImGui::DragFloat("Extend frustum bounding box", &im->m_debug_vector.y, 0.01f);
				TT("Needs 'Draw everything in Frustum'\nIncreases the frustum bounding box to also render objects behind the player.");

			}, true, ICON_FA_ELLIPSIS_H, &im->ImGuiCol_ContainerBackground, & im->ImGuiCol_ContainerBorder);
		}

		{
			static float cont_other_height = 0.0f;
			cont_other_height = ImGui::Widget_ContainerWithCollapsingTitle("Other Tweaks", cont_other_height, [&]
				{
					ImGui::Checkbox("Use Custom FOV", &im->m_viewmodel_use_custom_proj);
					ImGui::BeginDisabled(!im->m_viewmodel_use_custom_proj);
					{
						ImGui::SliderFloat("FOV", &im->m_viewmodel_proj_fov, 10.0f, 120.0f);
						ImGui::EndDisabled();
					}

				}, true, ICON_FA_ELLIPSIS_H, &im->ImGuiCol_ContainerBackground, &im->ImGuiCol_ContainerBorder);
		}

		//const auto cont_bg_color = im->ImGuiCol_ContainerBackground + ImVec4(0.05f, 0.05f, 0.05f, 0.0f);

		//ImGui::Spacing(0, 12);
		//ImGui::PushFont(shared::imgui::font::BOLD_LARGE);
		//ImGui::SeparatorText(" Culling Tweaks ");
		//ImGui::PopFont();
		//ImGui::Spacing(0, 4);

		//static float cont_height = 0.0f;
		//cont_height = ImGui::Widget_ContainerWithDropdownShadow(cont_height, [&]
		//	{
		//		{
		//			auto* var = reinterpret_cast<bool*>(0x56D31C); // VisDrawFrustum
		//			ImGui::Checkbox("Draw everything in Frustum (VisDrawFrustum var)", var); TT("This will draw everything infront of the player");

		//			// set num planes for a culling func from 5 to 1 if VisDrawFrustum is enabled
		//			*reinterpret_cast<BYTE*>(0x5190DC + 1) = var ? 1u : 5u;
		//		}

		//		ImGui::DragFloat("Extend frustum bounding box", &im->m_debug_vector.y, 0.01f);
		//		TT("Needs 'Draw everything in Frustum'\nIncreases the frustum bounding box to also render objects behind the player.");

		//	}, &cont_bg_color, &im->ImGuiCol_ContainerBorder);

		/*ImGui::SeparatorText("Custom Viewmodel Projection");
		ImGui::PushID("VMProj");
		ImGui::Checkbox("Use Custom FOV", &im->m_viewmodel_use_custom_proj);
		ImGui::SliderFloat("FOV", &im->m_viewmodel_proj_fov, 10.0f, 120.0f);
		ImGui::SliderFloat("Near Plane", &im->m_viewmodel_proj_near_plane, 0.1f, 10.0f);
		ImGui::SliderFloat("Far Plane", &im->m_viewmodel_proj_far_plane, 100.0f, 2000.0f);
		ImGui::PopID();*/

#if DEBUG
		{
			ImGui::Spacing(0, 8);
			if (ImGui::CollapsingHeader("DEBUG Build Section", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				ImGui::DragFloat3("Debug Vector", &im->m_debug_vector.x, 0.01f);
				ImGui::DragFloat3("Debug Vector 2", &im->m_debug_vector2.x, 0.1f);

				ImGui::Spacing(0, 6);

				const auto coloredit_flags = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_Float;

				SET_CHILD_WIDGET_WIDTH; ImGui::ColorEdit4("ContainerBg", &im->ImGuiCol_ContainerBackground.x, coloredit_flags);
				SET_CHILD_WIDGET_WIDTH; ImGui::ColorEdit4("ContainerBorder", &im->ImGuiCol_ContainerBorder.x, coloredit_flags);

				SET_CHILD_WIDGET_WIDTH; ImGui::ColorEdit4("ButtonGreen", &im->ImGuiCol_ButtonGreen.x, coloredit_flags);
				SET_CHILD_WIDGET_WIDTH; ImGui::ColorEdit4("ButtonYellow", &im->ImGuiCol_ButtonYellow.x, coloredit_flags);
				SET_CHILD_WIDGET_WIDTH; ImGui::ColorEdit4("ButtonRed", &im->ImGuiCol_ButtonRed.x, coloredit_flags);
			}
		}
#endif
	}

	void imgui::tab_general()
	{
		// quick commands
		{
			//static float cont_quickcmd_height = 0.0f;
			//cont_quickcmd_height = ImGui::Widget_ContainerWithCollapsingTitle("Quick Commands", cont_quickcmd_height, cont_general_quickcommands,
			//	true, ICON_FA_TERMINAL, &ImGuiCol_ContainerBackground, &ImGuiCol_ContainerBorder);

			cont_general_quickcommands();
		}
	}

	void imgui::devgui()
	{
		ImGui::SetNextWindowSize(ImVec2(900, 800), ImGuiCond_FirstUseEver);

		bool old_active_state = m_menu_active;
		if (!ImGui::Begin("Devgui", &m_menu_active, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse/*, &shared::imgui::draw_window_blur_callback*/))
		{
			ImGui::End();
			return;
		}

		// HACK when using the menu close button instead of the hotkey to close the devgui
		// :: use logic in 'imgui::input_message' to toggle the menu by sending a msg
		if (old_active_state != m_menu_active && !m_menu_active) 
		{ 
			m_menu_active = true; // we have to re-set this back to true. We would instantly reopen the gui otherwise
			SendMessage(shared::globals::main_window, WM_KEYUP, VK_F5, 0);
		}

		m_im_window_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
		m_im_window_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);

		static bool im_demo_menu = false;
		if (im_demo_menu) {
			ImGui::ShowDemoWindow(&im_demo_menu);
		}

#define ADD_TAB(NAME, FUNC) \
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0)));			\
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 8));			\
	if (ImGui::BeginTabItem(NAME)) {																		\
		ImGui::PopStyleVar(1);																				\
		if (ImGui::BeginChild("##child_" NAME, ImVec2(0, ImGui::GetContentRegionAvail().y - 38), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_AlwaysVerticalScrollbar )) {	\
			FUNC(); ImGui::EndChild();																		\
		} else {																							\
			ImGui::EndChild();																				\
		} ImGui::EndTabItem();																				\
	} else { ImGui::PopStyleVar(1); } ImGui::PopStyleColor();

		// ---------------------------------------

		const auto col_top = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.0f));
		const auto col_bottom = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.4f));
		const auto col_border = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.8f));
		const auto pre_tabbar_spos = ImGui::GetCursorScreenPos() - ImGui::GetStyle().WindowPadding;

		ImGui::GetWindowDrawList()->AddRectFilledMultiColor(pre_tabbar_spos, pre_tabbar_spos + ImVec2(ImGui::GetWindowWidth(), 40.0f),
			col_top, col_top, col_bottom, col_bottom);

		ImGui::GetWindowDrawList()->AddLine(pre_tabbar_spos + ImVec2(0, 40.0f), pre_tabbar_spos + ImVec2(ImGui::GetWindowWidth(), 40.0f),
			col_border, 1.0f);

		ImGui::SetCursorScreenPos(pre_tabbar_spos + ImVec2(12,8));

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 8));
		ImGui::PushStyleColor(ImGuiCol_TabSelected, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		if (ImGui::BeginTabBar("devgui_tabs"))
		{
			ImGui::PopStyleColor();
			ImGui::PopStyleVar(1);
			ADD_TAB("General", tab_general);
			ImGui::EndTabBar();
		}
		else {
			ImGui::PopStyleColor();
			ImGui::PopStyleVar(1);
		}
#undef ADD_TAB

		{
			ImGui::Separator();
			//ImGui::Spacing();

			const char* movement_hint_str = "Press and Hold the Right Mouse Button outside ImGui to allow for Game Input ";
			const auto avail_width = ImGui::GetContentRegionAvail().x;
			float cur_pos = avail_width - 54.0f;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			{
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);
				const auto spos = ImGui::GetCursorScreenPos();
				ImGui::TextUnformatted(m_devgui_custom_footer_content.c_str());
				ImGui::SetCursorScreenPos(spos);
				m_devgui_custom_footer_content.clear();
			}
			

			ImGui::SetCursorPos(ImVec2(cur_pos, ImGui::GetCursorPosY() + 2.0f));
			if (ImGui::Button("Demo", ImVec2(50, 0))) {
				im_demo_menu = !im_demo_menu;
			}

			ImGui::SameLine();
			cur_pos = cur_pos - ImGui::CalcTextSize(movement_hint_str).x - 6.0f;
			ImGui::SetCursorPosX(cur_pos);
			ImGui::TextUnformatted(movement_hint_str);
		}
		ImGui::PopStyleVar(1);
		ImGui::End();
	}

	void imgui::on_present()
	{
		if (auto* im = imgui::get(); im)
		{
			if (const auto dev = shared::globals::d3d_device; dev)
			{
				if (!im->m_initialized_device)
				{
					ImGui_ImplDX9_Init(dev);
					im->m_initialized_device = true;
				}

				if (im->m_initialized_device)
				{
					// fix imgui colors / background if no hud elem is visible
					dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 1);
					dev->SetRenderState(D3DRS_SRGBWRITEENABLE, 1);

					ImGui_ImplDX9_NewFrame();
					ImGui_ImplWin32_NewFrame();
					ImGui::NewFrame();

					auto& io = ImGui::GetIO();

					if (im->m_menu_active) {
						io.MouseDrawCursor = true;
						im->devgui();
					} else {
						io.MouseDrawCursor = false;
					}

					im->m_is_rendering = true;
					ImGui::EndFrame();
					ImGui::Render();
					ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
					im->m_is_rendering = false;
				}
			}
		}
	}

	void imgui::style_xo()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.Alpha = 1.0f;
		style.DisabledAlpha = 0.5f;

		style.WindowPadding = ImVec2(8.0f, 10.0f);
		style.FramePadding = ImVec2(7.0f, 6.0f);
		style.ItemSpacing = ImVec2(3.0f, 3.0f);
		style.ItemInnerSpacing = ImVec2(3.0f, 8.0f);
		style.IndentSpacing = 0.0f;
		style.ColumnsMinSpacing = 10.0f;
		style.ScrollbarSize = 10.0f;
		style.GrabMinSize = 10.0f;

		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.FrameBorderSize = 1.0f;
		style.TabBorderSize = 0.0f;

		style.WindowRounding = 0.0f;
		style.ChildRounding = 2.0f;
		style.FrameRounding = 4.0f;
		style.PopupRounding = 2.0f;
		style.ScrollbarRounding = 2.0f;
		style.GrabRounding = 1.0f;
		style.TabRounding = 2.0f;
		
		style.CellPadding = ImVec2(5.0f, 4.0f);

		auto& colors = style.Colors;
		colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.26f, 0.26f, 0.26f, 0.78f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.28f, 0.28f, 0.28f, 0.92f);
		colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.23f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.17f, 0.25f, 0.27f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.07f, 0.39f, 0.47f, 0.59f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.98f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 0.98f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 0.98f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.39f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.54f, 0.54f, 0.54f, 0.47f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.78f, 0.78f, 0.78f, 0.33f);
		colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.39f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.31f);
		colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.45f, 0.45f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.25f, 0.27f, 0.78f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.17f, 0.25f, 0.27f, 0.78f);
		colors[ImGuiCol_Separator] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.15f, 0.52f, 0.66f, 0.30f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.30f, 0.69f, 0.84f, 0.39f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.43f, 0.43f, 0.43f, 0.51f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.07f, 0.39f, 0.47f, 0.59f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.30f, 0.69f, 0.84f, 0.39f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.19f, 0.53f, 0.66f, 0.39f);
		colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.37f);
		colors[ImGuiCol_TabSelected] = ImVec4(0.11f, 0.39f, 0.51f, 0.64f);
		colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.10f, 0.34f, 0.43f, 0.30f);
		colors[ImGuiCol_TabDimmed] = ImVec4(0.00f, 0.00f, 0.00f, 0.16f);
		colors[ImGuiCol_TabDimmedSelected] = ImVec4(1.00f, 1.00f, 1.00f, 0.24f);
		colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 0.35f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 1.00f, 1.00f, 0.35f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.11f, 0.42f, 0.51f, 0.35f);
		colors[ImGuiCol_TextLink] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.00f, 0.51f, 0.39f, 0.31f);
		colors[ImGuiCol_NavCursor] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.56f);

		// custom colors
		ImGuiCol_ButtonGreen = ImVec4(0.3f, 0.4f, 0.05f, 0.7f);
		ImGuiCol_ButtonYellow = ImVec4(0.4f, 0.3f, 0.1f, 0.8f);
		ImGuiCol_ButtonRed = ImVec4(0.48f, 0.15f, 0.15f, 1.00f);
		ImGuiCol_ContainerBackground = ImVec4(0.220f, 0.220f, 0.220f, 0.863f);
		ImGuiCol_ContainerBorder = ImVec4(0.099f, 0.099f, 0.099f, 0.901f);
	}

	void init_fonts()
	{
		using namespace shared::imgui::font;

		auto merge_icons_with_latest_font = [](const float& font_size, const bool font_data_owned_by_atlas = false)
			{
				static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0 };

				ImFontConfig icons_config;
				icons_config.MergeMode = true;
				icons_config.PixelSnapH = true;
				icons_config.FontDataOwnedByAtlas = font_data_owned_by_atlas;

				ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), font_size, &icons_config, icons_ranges);
			};

		ImGuiIO& io = ImGui::GetIO();

		io.Fonts->AddFontFromMemoryCompressedTTF(opensans_bold_compressed_data, opensans_bold_compressed_size, 18.0f);
		merge_icons_with_latest_font(12.0f, false);

		io.Fonts->AddFontFromMemoryCompressedTTF(opensans_bold_compressed_data, opensans_bold_compressed_size, 17.0f);
		merge_icons_with_latest_font(12.0f, false);

		io.Fonts->AddFontFromMemoryCompressedTTF(opensans_regular_compressed_data, opensans_regular_compressed_size, 18.0f);
		io.Fonts->AddFontFromMemoryCompressedTTF(opensans_regular_compressed_data, opensans_regular_compressed_size, 16.0f);

		ImFontConfig font_cfg;
		font_cfg.FontDataOwnedByAtlas = false;

		io.FontDefault = io.Fonts->AddFontFromMemoryCompressedTTF(opensans_regular_compressed_data, opensans_regular_compressed_size, 17.0f, &font_cfg);
		merge_icons_with_latest_font(17.0f, false);
	}

	imgui::imgui()
	{
		p_this = this;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		init_fonts();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.MouseDrawCursor = true;

		//io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;
		//io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

		style_xo();

		ImGui_ImplWin32_Init(shared::globals::main_window);
		g_game_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(shared::globals::main_window, GWLP_WNDPROC, LONG_PTR(wnd_proc_hk)));

		printf("[Module] imgui loaded.\n");
	}
}
