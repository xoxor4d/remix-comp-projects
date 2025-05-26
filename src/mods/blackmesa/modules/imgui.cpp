#include "std_include.hpp"
#include "imgui.hpp"

#include "shared/common/flags.hpp"
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

namespace mods::blackhawkdown
{
	WNDPROC g_game_wndproc = nullptr;
	
	LRESULT __stdcall wnd_proc_hk(HWND window, UINT message_type, WPARAM wparam, LPARAM lparam)
	{
		bool pass_msg_to_game = false;

		if (imgui::get()->input_message(message_type, wparam, lparam, pass_msg_to_game) /*&& !pass_msg_to_game*/) {
			return true;
		}

		//game::console(); printf("MSG 0x%x -- w: 0x%x -- l: 0x%x\n", message_type, wparam, lparam);
		return CallWindowProc(g_game_wndproc, window, message_type, wparam, lparam);
	}

	bool imgui::input_message(const UINT message_type, const WPARAM wparam, const LPARAM lparam, [[maybe_unused]] bool& inout_pass_msg_to_game)
	{
		/*if (message_type == WM_KEYUP && wparam == VK_F5) {
			imgui::get()->m_dbg_use_fake_camera = !imgui::get()->m_dbg_use_fake_camera;
		}

		if (message_type == WM_KEYUP && wparam == VK_F6) {
			imgui::get()->m_dbg_set_world_transforms = !imgui::get()->m_dbg_set_world_transforms;
		}*/

		if (message_type == WM_KEYUP && wparam == VK_F4) 
		{
			const auto& io = ImGui::GetIO();
			if (!io.MouseDown[1]) {
				shared::globals::imgui_menu_open = !shared::globals::imgui_menu_open;
			}

			else {
				ImGui_ImplWin32_WndProcHandler(shared::globals::main_window, message_type, wparam, lparam);
			}
		}

		if (shared::globals::imgui_menu_open)
		{
			auto& io = ImGui::GetIO();

			//if (!(message_type == WM_MOUSEMOVE || message_type == WM_NCMOUSEMOVE)) {
				ImGui_ImplWin32_WndProcHandler(shared::globals::main_window, message_type, wparam, lparam);
			//}

			// enable game input if no imgui window is hovered and right mouse is held
			if (!m_im_window_hovered && io.MouseDown[1])
			{
				ImGui::SetWindowFocus(); // unfocus input text
				shared::globals::imgui_allow_input_bypass = true;
				return false;
			}

			// ^ wait until mouse is up and call set_cursor_always_visible once
			if (shared::globals::imgui_allow_input_bypass && !io.MouseDown[1])
			{
				shared::globals::imgui_allow_input_bypass = false;
				return false;
			}
		}
		else {
			shared::globals::imgui_allow_input_bypass = false; // always reset if there is no imgui window open
		}

		return shared::globals::imgui_menu_open;
	}

	// ------

	void cont_general_quickcommands()
	{
		const auto& im = imgui::get();

		{
			static float cont_cull_height = 0.0f;
			cont_cull_height = ImGui::Widget_ContainerWithCollapsingTitle("Camera", cont_cull_height, [&]
			{
				auto lod_multi = reinterpret_cast<int*>(0x9F20B8);
				SET_CHILD_WIDGET_WIDTH_MAN(140.0f); if (ImGui::SliderInt("LOD Multiplier", lod_multi, 0, 20)) {
					*lod_multi = std::clamp(*lod_multi, 0, 20);
				} TT("Higher values will keep higher LOD on for longer.");

				//ImGui::Checkbox("Use Fake Camera", &im->m_dbg_use_fake_camera);
				//SET_CHILD_WIDGET_WIDTH_MAN(140.0f); ImGui::SliderFloat3("Camera Position (X, Y, Z)", im->m_dbg_camera_pos, -200.0f, 200.0f);
				//SET_CHILD_WIDGET_WIDTH_MAN(140.0f); ImGui::SliderFloat("Yaw (Y-axis)", &im->m_dbg_camera_yaw, -180.0f, 180.0f);
				//SET_CHILD_WIDGET_WIDTH_MAN(140.0f); ImGui::SliderFloat("Pitch (X-axis)", &im->m_dbg_camera_pitch, -90.0f, 90.0f);

				//// Projection matrix adjustments
				//SET_CHILD_WIDGET_WIDTH_MAN(140.0f); ImGui::SliderFloat("FOV", &im->m_dbg_camera_fov, 10.0f, 120.0f);
				//SET_CHILD_WIDGET_WIDTH_MAN(140.0f); ImGui::SliderFloat("Aspect Ratio", &im->m_dbg_camera_aspect, 0.5f, 2.5f);
				//SET_CHILD_WIDGET_WIDTH_MAN(140.0f); ImGui::SliderFloat("Near Plane", &im->m_dbg_camera_near_plane, 0.1f, 10.0f);
				//SET_CHILD_WIDGET_WIDTH_MAN(140.0f); ImGui::SliderFloat("Far Plane", &im->m_dbg_camera_far_plane, 100.0f, 2000.0f);

			}, true, ICON_FA_ELLIPSIS_H, &im->ImGuiCol_ContainerBackground, & im->ImGuiCol_ContainerBorder);
		}

#if DEBUG
		{
			static float cont_debug_height = 0.0f;
			cont_debug_height = ImGui::Widget_ContainerWithCollapsingTitle("DEBUG Build Section", cont_debug_height, [&]
				{
					ImGui::DragFloat3("Debug Vector", &im->m_debug_vector.x, 0.01f);
					ImGui::DragFloat3("Debug Vector 2", &im->m_debug_vector2.x, 0.1f);

					ImGui::Spacing(0, 6);

					const auto coloredit_flags = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_Float;

					SET_CHILD_WIDGET_WIDTH_MAN(140.0f); ImGui::ColorEdit4("ContainerBg", &im->ImGuiCol_ContainerBackground.x, coloredit_flags);
					SET_CHILD_WIDGET_WIDTH_MAN(140.0f); ImGui::ColorEdit4("ContainerBorder", &im->ImGuiCol_ContainerBorder.x, coloredit_flags);

					SET_CHILD_WIDGET_WIDTH_MAN(140.0f); ImGui::ColorEdit4("ButtonGreen", &im->ImGuiCol_ButtonGreen.x, coloredit_flags);
					SET_CHILD_WIDGET_WIDTH_MAN(140.0f); ImGui::ColorEdit4("ButtonYellow", &im->ImGuiCol_ButtonYellow.x, coloredit_flags);
					SET_CHILD_WIDGET_WIDTH_MAN(140.0f); ImGui::ColorEdit4("ButtonRed", &im->ImGuiCol_ButtonRed.x, coloredit_flags);

				}, true, ICON_FA_ELLIPSIS_H, &im->ImGuiCol_ContainerBackground, &im->ImGuiCol_ContainerBorder);
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

		bool old_active_state = shared::globals::imgui_menu_open;
		if (!ImGui::Begin("Devgui", &shared::globals::imgui_menu_open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse/*, &shared::imgui::draw_window_blur_callback*/))
		{
			ImGui::End();
			return;
		}

		// HACK when using the menu close button instead of the hotkey to close the devgui
		// :: use logic in 'imgui::input_message' to toggle the menu by sending a msg
		if (old_active_state != shared::globals::imgui_menu_open && !shared::globals::imgui_menu_open)
		{ 
			shared::globals::imgui_menu_open = true; // we have to re-set this back to true. We would instantly reopen the gui otherwise
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
					DWORD og_srgb_samp, og_srgb_write;
					dev->GetSamplerState(0, D3DSAMP_SRGBTEXTURE, &og_srgb_samp);
					dev->GetRenderState(D3DRS_SRGBWRITEENABLE, &og_srgb_write);
					dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 1);
					dev->SetRenderState(D3DRS_SRGBWRITEENABLE, 1);

					ImGui_ImplDX9_NewFrame();
					ImGui_ImplWin32_NewFrame();
					ImGui::NewFrame();

					auto& io = ImGui::GetIO();

					if (shared::globals::imgui_menu_open) 
					{
						*game::g_free_mouse = 1;
						io.MouseDrawCursor = true;
						im->devgui();
					}
					else 
					{
						*game::g_free_mouse = 0;
						io.MouseDrawCursor = false;
					}

					shared::globals::imgui_is_rendering = true;
					ImGui::EndFrame();
					ImGui::Render();
					ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
					shared::globals::imgui_is_rendering = false;

					// restore
					dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, og_srgb_samp);
					dev->SetRenderState(D3DRS_SRGBWRITEENABLE, og_srgb_write);
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

		std::cout << "[IMGUI] loaded\n";
	}
}
