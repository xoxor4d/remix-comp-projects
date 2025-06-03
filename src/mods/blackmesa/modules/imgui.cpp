#include "std_include.hpp"
#include "imgui.hpp"

#include "game_settings.hpp"
#include "imgui_internal.h"
#include "interfaces.hpp"
#include "map_settings.hpp"
#include "renderer.hpp"
#include "shared/common/flags.hpp"
#include "shared/common/remix_vars.hpp"
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

namespace imgui_helper
{
	enum Widget_UnorderedSetModifierFlags : std::uint8_t
	{
		Widget_UnorderedSetModifierFlags_Leaf = 0,
		Widget_UnorderedSetModifierFlags_Area = 1 << 0,
	};

	void Widget_UnorderedSetModifier(const char* id, Widget_UnorderedSetModifierFlags flag, std::unordered_set<std::uint32_t>& set, char* buffer, std::uint32_t buffer_len)
	{
		const auto txt_input_full = "Add/Remove..";
		const auto txt_input_full_width = ImGui::CalcTextSize(txt_input_full).x;
		const auto txt_input_min = "...";
		const auto txt_input_min_width = ImGui::CalcTextSize(txt_input_min).x;
	
		const bool narrow = ImGui::GetContentRegionAvail().x < 100.0f;

		ImGui::PushID(id);
	
		if (!narrow) 
		{
			if (ImGui::Button("-##Remove"))
			{
				shared::imgui::get_and_remove_integers_from_set(buffer, set, buffer_len, true);
				mods::blackmesa::trigger_vis_logic();
			}
			ImGui::SetCursorScreenPos(ImVec2(ImGui::GetItemRectMax().x, ImGui::GetItemRectMin().y));
		}
	
		const auto spos = ImGui::GetCursorScreenPos();

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (narrow ? 0.0f : 40.0f));
		if (ImGui::InputText("##Input", buffer, buffer_len, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll)) {
			shared::imgui::get_and_add_integers_to_set(buffer, set, buffer_len, true);
		}

		ImGui::SetCursorScreenPos(spos);
		if (!buffer[0])
		{
			const auto min_content_area_width = ImGui::GetContentRegionAvail().x - 40.0f;
			ImVec2 pos = ImGui::GetCursorScreenPos() + ImVec2(8.0f, ImGui::CalcTextSize("A").y * 0.45f);
			if (min_content_area_width > txt_input_full_width) {
				ImGui::GetWindowDrawList()->AddText(pos, ImGui::GetColorU32(ImGuiCol_TextDisabled), txt_input_full);
			}
			else if (min_content_area_width > txt_input_min_width) {
				ImGui::GetWindowDrawList()->AddText(pos, ImGui::GetColorU32(ImGuiCol_TextDisabled), txt_input_min);
			}
		}
	
		if (narrow) 
		{
			// next line :>
			ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
			if (ImGui::Button("-##Remove"))
			{
				shared::imgui::get_and_remove_integers_from_set(buffer, set, buffer_len, true);
				mods::blackmesa::trigger_vis_logic();
			}
		}

		ImGui::SetCursorScreenPos(ImVec2(ImGui::GetItemRectMax().x, ImGui::GetItemRectMin().y));
		if (ImGui::Button("+##Add")) {
			shared::imgui::get_and_add_integers_to_set(buffer, set, buffer_len, true);
		}
		ImGui::SetCursorScreenPos(ImVec2(ImGui::GetItemRectMax().x + 1.0f, ImGui::GetItemRectMin().y));
		if (ImGui::Button("P##Picker"))
		{
			const auto c_str = shared::utils::va("%d", flag == Widget_UnorderedSetModifierFlags_Leaf ? mods::blackmesa::g_current_leaf : mods::blackmesa::g_current_area);
			shared::imgui::get_and_add_integers_to_set((char*)c_str, set);
			mods::blackmesa::trigger_vis_logic();
		}
		ImGui::SetItemTooltipBlur(flag == Widget_UnorderedSetModifierFlags_Leaf ? "Pick Current Leaf" : "Pick Current Area");
		ImGui::PopID();
	}
}

namespace mods::blackmesa
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

	void center_cursor()
	{
		RECT rect;
		if (GetClientRect(shared::globals::main_window, &rect))
		{
			POINT center;
			center.x = (rect.right - rect.left) / 2;
			center.y = (rect.bottom - rect.top) / 2;

			ClientToScreen(shared::globals::main_window, &center);
			SetCursorPos(center.x, center.y);
		}
	}

	bool imgui::input_message(const UINT message_type, const WPARAM wparam, const LPARAM lparam, [[maybe_unused]] bool& inout_pass_msg_to_game)
	{
		if (message_type == WM_KEYUP && wparam == VK_F5) 
		{
			const auto& io = ImGui::GetIO();
			if (!io.MouseDown[1]) 
			{
				shared::globals::imgui_menu_open = !shared::globals::imgui_menu_open;

				// reset cursor to center when closing the menu to not affect player angles
				if (interfaces::get()->m_surface->is_cursor_visible() && !shared::globals::imgui_menu_open)
				{
					center_cursor();
					SendMessage(shared::globals::main_window, WM_ACTIVATEAPP, TRUE, 0);
					SendMessage(shared::globals::main_window, WM_MOUSEACTIVATE, TRUE, 0);
				}

				interfaces::get()->m_surface->set_cursor_always_visible(shared::globals::imgui_menu_open);
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
				// center cursor and only call set_cursor_always_visible once 
				if (!shared::globals::imgui_allow_input_bypass)
				{
					center_cursor();
					interfaces::get()->m_surface->set_cursor_always_visible(false);
				}

				ImGui::SetWindowFocus(); // unfocus input text
				shared::globals::imgui_allow_input_bypass = true;
				return false;
			}

			// ^ wait until mouse is up and call set_cursor_always_visible once
			if (shared::globals::imgui_allow_input_bypass && !io.MouseDown[1])
			{
				shared::globals::imgui_allow_input_bypass = false;
				interfaces::get()->m_surface->set_cursor_always_visible(true);
				return false;
			}
		}
		else {
			shared::globals::imgui_allow_input_bypass = false; // always reset if there is no imgui window open
		}

		return shared::globals::imgui_menu_open;
	}

	// ------

	void cont_gamesettings_renderer_settings()
	{
		const auto gs = game_settings::get();
		ImGui::Checkbox("Enable LOD Forcing", gs->lod_forcing.get_as<bool*>()); TT(gs->lod_forcing.get_tooltip_string().c_str());

		if (ImGui::Checkbox("Enable 3D Skybox (very unstable)", gs->enable_3d_sky.get_as<bool*>())) {
			cross_handle_map_and_game_settings();
		} TT(gs->enable_3d_sky.get_tooltip_string().c_str());

		ImGui::Checkbox("Enable dual layered water", gs->enable_dual_layered_water.get_as<bool*>()); 
		TT(gs->enable_dual_layered_water.get_tooltip_string().c_str());

		SET_CHILD_WIDGET_WIDTH_MAN(120.0f);
		auto gs_nocull_dist_ptr = game_settings::get()->default_nocull_distance.get_as<float*>();
		if (ImGui::DragFloat("Def. NoCull Dist", gs_nocull_dist_ptr, 0.5f, 0.0f, FLT_MAX, "%.2f"))
		{
			*gs_nocull_dist_ptr = *gs_nocull_dist_ptr < 0.0f ? 0.0f : *gs_nocull_dist_ptr;
			//map_settings::get_map_settings().default_nocull_dist = *gs_nocull_dist_ptr;
		}
		TT(gs->default_nocull_distance.get_tooltip_string().c_str());
	}

	void cont_gamesettings_flashlight()
	{
		const auto gs = game_settings::get();

		ImGui::Widget_PrettyDragVec3("Offsets Player", gs->flashlight_offset_player.get_as<float*>(), true, 120.0f, 0.1f, -1000.0f, 1000.0f, "F", "H", "V");
		TT(gs->flashlight_offset_player.get_tooltip_string().c_str());

		SET_CHILD_WIDGET_WIDTH_MAN(120);
		ImGui::DragFloat("Intensity", gs->flashlight_intensity.get_as<float*>(), 0.1f);

		SET_CHILD_WIDGET_WIDTH_MAN(120);
		ImGui::DragFloat("Radius", gs->flashlight_radius.get_as<float*>(), 0.05f);

		SET_CHILD_WIDGET_WIDTH_MAN(120);
		ImGui::DragFloat("Spot Angle", gs->flashlight_angle.get_as<float*>(), 0.05f);

		SET_CHILD_WIDGET_WIDTH_MAN(120);
		ImGui::DragFloat("Spot Softness", gs->flashlight_softness.get_as<float*>(), 0.001f);

		SET_CHILD_WIDGET_WIDTH_MAN(120);
		ImGui::DragFloat("Spot Expo", gs->flashlight_expo.get_as<float*>(), 0.001f);

		ImGui::SeparatorText("  Inner Flashlight  ");

		SET_CHILD_WIDGET_WIDTH_MAN(120);
		ImGui::DragFloat("Inner Spot Angle", gs->flashlight_angle_inner.get_as<float*>(), 0.05f);

		SET_CHILD_WIDGET_WIDTH_MAN(120);
		ImGui::DragFloat("Inner Intensity", gs->flashlight_intensity_inner.get_as<float*>(), 0.1f);

		SET_CHILD_WIDGET_WIDTH_MAN(120);
		ImGui::DragFloat("Inner Spot Softness", gs->flashlight_softness_inner.get_as<float*>(), 0.001f);
	}

	void cont_gamesettings_quick_cmd()
	{
		if (ImGui::Button("Save Current Settings", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0))) {
			game_settings::write_toml();
		}

		ImGui::SameLine();
		if (ImGui::Button("Reload GameSettings", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		{
			if (!ImGui::IsPopupOpen("Reload GameSettings?")) {
				ImGui::OpenPopup("Reload GameSettings?");
			}
		}

		// popup
		if (ImGui::BeginPopupModal("Reload GameSettings?", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
		{
			shared::imgui::draw_background_blur();
			ImGui::Spacing(0.0f, 0.0f);

			const auto half_width = ImGui::GetContentRegionMax().x * 0.5f;
			auto line1_str = "You'll loose all unsaved changes if you continue!   ";
			auto line2_str = "To save your changes, use:";
			auto line3_str = "Save Current Settings";

			ImGui::Spacing();
			ImGui::SetCursorPosX(5.0f + half_width - (ImGui::CalcTextSize(line1_str).x * 0.5f));
			ImGui::TextUnformatted(line1_str);

			ImGui::Spacing();
			ImGui::SetCursorPosX(5.0f + half_width - (ImGui::CalcTextSize(line2_str).x * 0.5f));
			ImGui::TextUnformatted(line2_str);

			ImGui::PushFont(shared::imgui::font::BOLD);
			ImGui::SetCursorPosX(5.0f + half_width - (ImGui::CalcTextSize(line3_str).x * 0.5f));
			ImGui::TextUnformatted(line3_str);
			ImGui::PopFont();

			ImGui::Spacing(0, 8);
			ImGui::Spacing(0, 0); ImGui::SameLine();

			ImVec2 button_size(half_width - 6.0f - ImGui::GetStyle().WindowPadding.x, 0.0f);
			if (ImGui::Button("Reload", button_size))
			{
				game_settings::xo_gamesettings_update_fn();
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine(0, 6.0f);
			if (ImGui::Button("Cancel", button_size)) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void imgui::tab_game_settings()
	{
		// quick commands
		{
			static float cont_quickcmd_height = 0.0f;
			cont_quickcmd_height = ImGui::Widget_ContainerWithCollapsingTitle("Quick Commands", cont_quickcmd_height, cont_gamesettings_quick_cmd,
				true, ICON_FA_TERMINAL, &ImGuiCol_ContainerBackground, &ImGuiCol_ContainerBorder);
		}

		// renderer related settings
		{
			static float cont_rendersettings_height = 0.0f;
			cont_rendersettings_height = ImGui::Widget_ContainerWithCollapsingTitle("Renderer Related Settings", cont_rendersettings_height, cont_gamesettings_renderer_settings,
				true, ICON_FA_CAMERA, &ImGuiCol_ContainerBackground, &ImGuiCol_ContainerBorder);
		}

		// flashlight
		{
			static float cont_flashlight_height = 0.0f;
			cont_flashlight_height = ImGui::Widget_ContainerWithCollapsingTitle("Flashlight", cont_flashlight_height, cont_gamesettings_flashlight,
				true, ICON_FA_LIGHTBULB, &ImGuiCol_ContainerBackground, &ImGuiCol_ContainerBorder);
		}
	}

	void cont_general_quickcommands()
	{
#if DEBUG
		const auto& im = imgui::get();
		{
			static float cont_debug_height = 0.0f;
			cont_debug_height = ImGui::Widget_ContainerWithCollapsingTitle("DEBUG Build Section", cont_debug_height, [&]
				{
					ImGui::DragFloat3("Debug Vector", &im->m_debug_vector.x, 0.01f);
					ImGui::DragFloat3("Debug Vector 2", &im->m_debug_vector2.x, 0.1f);
					if (ImGui::SliderInt("Debug Int", &im->m_debug_int, 0, 13)) {
						im->m_debug_int = std::clamp(im->m_debug_int, 0, 13);
					}

					ImGui::Spacing(0, 6);

					for (auto i = 0; i < sizeof(im->m_debug_disable_rendering); i++)
					{
						ImGui::PushID(i);
						ImGui::Checkbox(shared::utils::va("Disable Rendering [%d]", i), &im->m_debug_disable_rendering[i]);
						ImGui::PopID();
					}

					ImGui::Checkbox("Disable Unbaking", &im->m_debug_disable_unbake);

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

	bool reload_mapsettings_popup()
	{
		bool result = false;
		if (ImGui::BeginPopupModal("Reload MapSettings?", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
		{
			shared::imgui::draw_background_blur();
			const auto half_width = ImGui::GetContentRegionMax().x * 0.5f;
			auto line1_str = "You'll loose all unsaved changes if you continue!";
			auto line2_str = "Use the copy to clipboard buttons and manually update  ";
			auto line3_str = "the map_settings.toml file if you've made changes.";

			ImGui::Spacing();
			ImGui::SetCursorPosX(5.0f + half_width - (ImGui::CalcTextSize(line1_str).x * 0.5f));
			ImGui::TextUnformatted(line1_str);

			ImGui::Spacing();
			ImGui::SetCursorPosX(5.0f + half_width - (ImGui::CalcTextSize(line2_str).x * 0.5f));
			ImGui::TextUnformatted(line2_str);
			ImGui::SetCursorPosX(5.0f + half_width - (ImGui::CalcTextSize(line3_str).x * 0.5f));
			ImGui::TextUnformatted(line3_str);

			ImGui::Spacing(0, 8);
			ImGui::Spacing(0, 0); ImGui::SameLine();

			ImVec2 button_size(half_width - 6.0f - ImGui::GetStyle().WindowPadding.x, 0.0f);
			if (ImGui::Button("Reload", button_size))
			{
				result = true;
				map_settings::reload();
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine(0, 6);
			if (ImGui::Button("Cancel", button_size)) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		return result;
	}

	bool reload_mapsettings_button_with_popup(const char* ID)
	{
		ImGui::PushFont(shared::imgui::font::BOLD);
		if (ImGui::Button(shared::utils::va("Reload MapSettings  %s##%s", ICON_FA_REDO, ID), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		{
			if (!ImGui::IsPopupOpen("Reload MapSettings?")) {
				ImGui::OpenPopup("Reload MapSettings?");
			}
		}
		ImGui::PopFont();

		return reload_mapsettings_popup();
	}

	void cont_mapsettings_general()
	{
		auto& ms = map_settings::get_map_settings();
		const auto gs = game_settings::get();

		ImGui::PushFont(shared::imgui::font::BOLD);
		if (ImGui::Button("Reload rtx.conf    " ICON_FA_REDO, ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0)))
		{
			if (!ImGui::IsPopupOpen("Reload RtxConf?")) {
				ImGui::OpenPopup("Reload RtxConf?");
			}
		} ImGui::PopFont();

		// popup
		if (ImGui::BeginPopupModal("Reload RtxConf?", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
		{
			shared::imgui::draw_background_blur();
			ImGui::Spacing(0.0f, 0.0f);

			const auto half_width = ImGui::GetContentRegionMax().x * 0.5f;
			auto line1_str = "This will reload the rtx.conf file and re-apply all of it's variables.  ";
			auto line3_str = "(excluding texture hashes)";

			ImGui::Spacing();
			ImGui::SetCursorPosX(5.0f + half_width - (ImGui::CalcTextSize(line1_str).x * 0.5f));
			ImGui::TextUnformatted(line1_str);

			ImGui::PushFont(shared::imgui::font::BOLD);
			ImGui::SetCursorPosX(5.0f + half_width - (ImGui::CalcTextSize(line3_str).x * 0.5f));
			ImGui::TextUnformatted(line3_str);
			ImGui::PopFont();

			ImGui::Spacing(0, 8);
			ImGui::Spacing(0, 0); ImGui::SameLine();

			ImVec2 button_size(half_width - 6.0f - ImGui::GetStyle().WindowPadding.x, 0.0f);
			if (ImGui::Button("Reload", button_size))
			{
				shared::common::remix_vars::xo_vars_parse_options_fn();
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine(0, 6.0f);
			if (ImGui::Button("Cancel", button_size)) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::SameLine();
		reload_mapsettings_button_with_popup("General");

		ImGui::Spacing(0, 6);

		{
			SET_CHILD_WIDGET_WIDTH_MAN(120.0f);
			auto gs_nocull_dist_ptr = gs->default_nocull_distance.get_as<float*>();
			if (ImGui::DragFloat("Def. NoCull Dist", gs_nocull_dist_ptr, 0.5f, 0.0f, FLT_MAX, "%.2f")) {
				*gs_nocull_dist_ptr = *gs_nocull_dist_ptr < 0.0f ? 0.0f : *gs_nocull_dist_ptr;
			} TT(gs->default_nocull_distance.get_tooltip_string().c_str());
		}

		ImGui::Spacing(0, 6);

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		ImGui::TableHeaderDropshadow();
		const bool water_header_state = ImGui::CollapsingHeader("Water Settings");
		ImGui::PopStyleVar();

		if (water_header_state)
		{
			SET_CHILD_WIDGET_WIDTH_MAN(120.0f);
			if (ImGui::DragFloat("UV Scale##Water", &ms.water_uv_scale, 0.05f, 0.01f, FLT_MAX, "%.2f")) {
				ms.water_uv_scale = std::clamp(ms.water_uv_scale, 0.0f, FLT_MAX);
			}

			SET_CHILD_WIDGET_WIDTH_MAN(120.0f);
			if (ImGui::DragFloat("UV Bottom Scale##Water", &ms.water_uv_bottom_scale, 0.05f, 0.01f, FLT_MAX, "%.2f")) {
				ms.water_uv_bottom_scale = std::clamp(ms.water_uv_bottom_scale, 0.0f, FLT_MAX);
			}

			SET_CHILD_WIDGET_WIDTH_MAN(120.0f);
			ImGui::DragFloat("Top Layer Scale", &ms.water_scale_top, 0.05f, -100.0f, 100.0f, "%.2f");
			TT("This can can scale the dual rendered layer (usually the animated surface)");

			SET_CHILD_WIDGET_WIDTH_MAN(120.0f);
			ImGui::DragFloat("Top Layer Offset", &ms.water_offset_top, 0.05f, -100.0f, 100.0f, "%.2f");
			TT("This can offset the dual rendered water mesh along the Z-Axis (usually the animated surface)");

			SET_CHILD_WIDGET_WIDTH_MAN(120.0f);
			ImGui::DragFloat("Base Layer Offset", &ms.water_offset_base, 0.05f, -100.0f, 100.0f, "%.2f");
			TT("This can offset the original water mesh along the Z-Axis (usually the surface defining water color)");
		}
	}

	void cont_mapsettings_marker_manipulation()
	{
		auto& markers = map_settings::get_map_settings().map_markers;
		ImGui::PushFont(shared::imgui::font::BOLD);
		if (ImGui::Button("Copy All Markers to Clipboard   " ICON_FA_SAVE, ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0)))
		{
			ImGui::LogToClipboard();
			ImGui::LogText("%s", map_settings::build_map_marker_string_for_current_map(markers).c_str());
			ImGui::LogFinish();
		} ImGui::PopFont();

		ImGui::SameLine();
		reload_mapsettings_button_with_popup("MapMarker");
		//ImGui::Spacing(0, 4);

		constexpr auto in_buflen = 1024u;
		static char in_area_buf[in_buflen], in_nleaf_buf[in_buflen];
		static map_settings::marker_settings_s* selection = nullptr;

		//
		// MARKER TABLE

		ImGui::TableHeaderDropshadow();
		if (ImGui::BeginTable("MarkerTable", 8,
			ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ContextMenuInBody |
			ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_ScrollY, ImVec2(0, 380)))
		{
			ImGui::TableSetupScrollFreeze(0, 1); // make top row always visible
			ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoHide, 12.0f);
			ImGui::TableSetupColumn("Num", ImGuiTableColumnFlags_NoResize, 24.0f);
			ImGui::TableSetupColumn("Areas", ImGuiTableColumnFlags_WidthStretch, 80.0f);
			ImGui::TableSetupColumn("Comment", ImGuiTableColumnFlags_WidthStretch, 200.0f);
			ImGui::TableSetupColumn("Pos", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide, 200.0f);
			ImGui::TableSetupColumn("Rot", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide, 180.0f);
			ImGui::TableSetupColumn("Scale", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide, 130.0f);
			ImGui::TableSetupColumn("##Delete", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoClip, 16.0f);
			ImGui::TableHeadersRow();

			bool selection_matches_any_entry = false;
			map_settings::marker_settings_s* marked_for_deletion = nullptr;

			for (auto i = 0u; i < markers.size(); i++)
			{
				auto& m = markers[i];

				// default selection
				if (!selection) {
					selection = &m;
				}

				ImGui::TableNextRow();

				// save Y offset
				const auto save_row_min_y_pos = ImGui::GetCursorScreenPos().y - ImGui::GetStyle().FramePadding.y + ImGui::GetStyle().CellPadding.y;

				// handle row background color for selected entry
				const bool is_selected = selection && selection == &m;
				if (is_selected) {
					ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_TableRowBgAlt));
				}

				// -
				ImGui::TableNextColumn();
				if (!is_selected) // only selectable if not selected
				{
					ImGui::Style_InvisibleSelectorPush(); // never show selection - we use tablebg
					if (ImGui::Selectable(shared::utils::va("%d", i), false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, ImVec2(0, 22 + ImGui::GetStyle().CellPadding.y * 1.0f))) {
						selection = &m;
					}
					ImGui::Style_InvisibleSelectorPop();

					if (ImGui::IsItemHovered()) {
						ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.6f)));///*ImGui::GetColorU32(ImGuiCol_TableRowBgAlt)*/);
					}
				}
				else {
					ImGui::Text("%d", i); // if selected
				}

				if (selection && selection == &m) {
					selection_matches_any_entry = true; // check that the selection ptr is up to date
				}

				// - marker num
				ImGui::TableNextColumn();
				ImGui::Text("%d", m.index);

				// - Area Input
				ImGui::TableNextColumn();

				if (is_selected) {
					imgui_helper::Widget_UnorderedSetModifier("MarkerArea", imgui_helper::Widget_UnorderedSetModifierFlags_Area, selection->areas, in_area_buf, in_buflen);
				}

				ImGui::Spacing();
				ImGui::TextWrapped_IntegersFromUnorderedSet(m.areas);
				ImGui::Spacing();

				// - comment
				ImGui::TableNextColumn();
				ImGui::TextWrapped(m.comment.c_str());

				const auto row_max_y_pos = ImGui::GetItemRectMax().y;

				// - pos
				ImGui::TableNextColumn(); ImGui::Spacing();
				ImGui::Text("%.2f, %.2f, %.2f", m.origin.x, m.origin.y, m.origin.z);

				// - rot
				ImGui::TableNextColumn(); ImGui::Spacing();
				ImGui::Text("%.2f, %.2f, %.2f", m.rotation.x, m.rotation.y, m.rotation.z);

				// - scale
				ImGui::TableNextColumn(); ImGui::Spacing();
				ImGui::Text("%.2f, %.2f, %.2f", m.scale.x, m.scale.y, m.scale.z);

				// Delete Button
				ImGui::TableNextColumn();
				{
					ImGui::Style_DeleteButtonPush();
					ImGui::PushID((int)i);

					const auto btn_size = ImVec2(16, is_selected ? (row_max_y_pos - save_row_min_y_pos) : 25.0f);
					if (ImGui::Button("x##Marker", btn_size))
					{
						marked_for_deletion = &m;
						trigger_vis_logic();
					}

					ImGui::Style_DeleteButtonPop();
					ImGui::PopID();
				}

			} // end for loop

			if (!selection_matches_any_entry)
			{
				for (auto& m : markers)
				{
					if (selection && selection == &m)
					{
						selection_matches_any_entry = true;
						break;
					}
				}

				if (!selection_matches_any_entry) {
					selection = nullptr;
				}
			}
			else if (selection) {
				game::debug_add_text_overlay(&selection->origin.x, "[ImGui] Selected Marker", 0, 0.8f, 1.0f, 0.3f, 0.8f);
			}

			// remove entry
			if (marked_for_deletion)
			{
				for (auto it = markers.begin(); it != markers.end(); ++it)
				{
					if (&*it == marked_for_deletion)
					{
						markers.erase(it);
						selection = nullptr;
						break;
					}
				}
			}
			ImGui::EndTable();
		}

		ImGui::Style_ColorButtonPush(imgui::get()->ImGuiCol_ButtonGreen, true);
		if (ImGui::Button("++ Marker"))
		{
			std::uint32_t free_marker = 0u;
			for (auto i = 0u; i < markers.size(); i++)
			{
				if (markers[i].index == free_marker)
				{
					free_marker++;
					i = 0u; // restart loop
				}
			}

			markers.emplace_back(map_settings::marker_settings_s{
					free_marker, *game::get_current_view_origin() - Vector(0,0,1)
				});

			selection = &markers.back();
		}
		ImGui::Style_ColorButtonPop();

		if (selection)
		{
			ImGui::SameLine();
			ImGui::Style_ColorButtonPush(imgui::get()->ImGuiCol_ButtonYellow, true);
			if (ImGui::Button("Duplicate Current Marker"))
			{
				markers.emplace_back(map_settings::marker_settings_s{
					.index = selection->index,
					.origin = selection->origin,
					.rotation = selection->rotation,
					.scale = selection->scale,
					.areas = selection->areas,
					});

				selection = &markers.back();
			}
			ImGui::Style_ColorButtonPop();
		}

		ImGui::SameLine();
		ImGui::BeginDisabled(!selection);
		{
			if (ImGui::Button("TP to Marker")) {
				interfaces::get()->m_engine->execute_client_cmd_unrestricted(shared::utils::va("sv_cheats 1; noclip; setpos %.2f %.2f %.2f", selection->origin.x, selection->origin.y, selection->origin.z - 40.0f));
			}

			ImGui::SameLine();
			if (ImGui::Button("TP Marker to Player", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
			{
				selection->origin = *game::get_current_view_origin();
				selection->origin.z -= 1.0f;
			}
			ImGui::EndDisabled();
		}

		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::SeparatorText("Modify Marker");

		ImGui::Spacing();
		ImGui::Spacing();

		if (selection)
		{
			int temp_num = (int)selection->index;

			SET_CHILD_WIDGET_WIDTH;
			if (ImGui::DragInt("Number", &temp_num, 0.1f, 0))
			{
				if (temp_num < 0) {
					temp_num = 0;
				}
				selection->index = (std::uint32_t)temp_num;
			}

			//ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.6f, 0.5f));
			ImGui::Widget_PrettyDragVec3("Origin", &selection->origin.x, true, 80.0f, 0.5f,
				-FLT_MAX, FLT_MAX, "X", "Y", "Z");
			//ImGui::PopStyleVar();

			// RAD2DEG -> DEG2RAD 
			Vector temp_rot = { RAD2DEG(selection->rotation.x), RAD2DEG(selection->rotation.y), RAD2DEG(selection->rotation.z) };

			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.6f, 0.5f));
			if (ImGui::Widget_PrettyDragVec3("Rotation", &temp_rot.x, true, 80.0f, 0.1f,
				-360.0f, 360.0f, "Rx", "Ry", "Rz"))
			{
				selection->rotation = { DEG2RAD(temp_rot.x), DEG2RAD(temp_rot.y), DEG2RAD(temp_rot.z) };
			} ImGui::PopStyleVar();

			{
				ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.6f, 0.5f));
				ImGui::Widget_PrettyDragVec3("Scale", &selection->scale.x, true, 80.0f, 0.01f,
					-FLT_MAX, FLT_MAX, "Sx", "Sy", "Sz");
				ImGui::PopStyleVar();
			}

			SET_CHILD_WIDGET_WIDTH;
			ImGui::InputText("Comment", &selection->comment);
		} // selection

		ImGui::Spacing();
	}

	void cont_mapsettings_culling_manipulation()
	{
		auto& areas = map_settings::get_map_settings().area_settings;
		ImGui::PushFont(shared::imgui::font::BOLD);
		if (ImGui::Button("Copy Settings to Clipboard   " ICON_FA_SAVE "##Cull", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0)))
		{
			ImGui::LogToClipboard();
			ImGui::LogText("%s", map_settings::build_culling_overrides_string_for_current_map(areas).c_str());
			ImGui::LogFinish();
		} ImGui::PopFont();

		ImGui::SameLine();
		reload_mapsettings_button_with_popup("Cull");
		//ImGui::Spacing(0, 4);

		static map_settings::area_overrides_s* area_selection = nullptr;
		static map_settings::area_overrides_s* area_selection_old = nullptr;
		area_selection_old = area_selection; // we compare at the end of the table

		constexpr auto in_buflen = 1024u;
		static char in_leafs_buf[in_buflen], in_areas_buf[in_buflen],
			in_twk_in_leafs_buf[in_buflen], in_twk_areas_buf[in_buflen], in_twk_force_leafs_buf[in_buflen],
			in_hide_leafs_buf[in_buflen], in_hide_areas_buf[in_buflen], in_hide_nleafs_buf[in_buflen];

		// # CULL TABLE
		constexpr auto cull_table_num_columns = 2;
		ImGui::TableHeaderDropshadow();

		if (ImGui::BeginTable("CullTable", cull_table_num_columns, ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
			ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_ScrollY, ImVec2(0, 480)))
		{
			ImGui::TableSetupScrollFreeze(0, 1); // make top row always visible
			ImGui::TableSetupColumn("Area", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoHide, 32.0f);
			ImGui::TableSetupColumn("No Cull Distance", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch);

			const char* cull_table_tooltips[cull_table_num_columns] =
			{
				"The Area the player has to be in to trigger any override functionality.\n",
				"Different anti/culling modes and additional settings for various use-cases."
			};

			ImGui::TableHeadersRowWithTooltip(cull_table_tooltips);

			bool area_selection_matches_any_entry = false;
			auto row_num = 0u;

			for (auto& [area_num, a] : areas)
			{
				ImGui::TableNextRow();

				// default selection
				if (!area_selection) {
					area_selection = &a;
				}

				// save Y offset
				const auto area_table_first_row_y_pos = ImGui::GetCursorScreenPos().y - ImGui::GetStyle().FramePadding.y + ImGui::GetStyle().CellPadding.y;

				// handle row background color for selected entry
				const bool is_area_selected = area_selection && area_selection == &a;
				const bool player_is_in_area = mods::blackmesa::g_player_current_area_override && g_player_current_area_override == &a;

				// -
				ImGui::TableNextColumn();

				float first_col_width = ImGui::GetCursorScreenPos().x;
				float start_y = ImGui::GetCursorScreenPos().y; // save row start of selector at the end of a row

				if (is_area_selected) {
					ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_TableRowBgAlt));
				}

				// set background for first column - highlight current area
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
					player_is_in_area ? ImGui::GetColorU32(ImGuiCol_DragDropTarget) : ImGui::GetColorU32(ImGuiCol_TableHeaderBg));

				// - Area
				const auto ar_num_str = shared::utils::va("%d", (int)area_num);
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x * 0.5f - ImGui::CalcTextSize(ar_num_str).x * 0.5f));
				ImGui::TextUnformatted(ar_num_str);

				if (is_area_selected) {
					area_selection_matches_any_entry = true; // check that the selection ptr is up to date
				}

				// Mode
				ImGui::TableNextColumn();

				// width of first col
				first_col_width = ImGui::GetCursorScreenPos().x - first_col_width;

				ImGui::PushID((int)area_num);
				{
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat("##NocullDist", &a.nocull_distance, 0.1f, 0.0f, FLT_MAX, "%.0f"))
					{
						a.nocull_distance = a.nocull_distance < 0.0f ? 0.0f : a.nocull_distance;
						trigger_vis_logic();
					} TT("NoCull Distance - Radius around the player where nothing will get culled.")
				}

				auto row_max_y_pos = ImGui::GetItemRectMax().y;
				ImGui::PopID();

				row_max_y_pos = std::max(row_max_y_pos, ImGui::GetItemRectMax().y);

				if (!is_area_selected)
				{
					ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, start_y - 2.0f));
					const float content_height = row_max_y_pos - area_table_first_row_y_pos;

					ImGuiWindow* window = ImGui::GetCurrentWindow();
					const float min_x = window->ParentWorkRect.Min.x + first_col_width;
					const float max_x = window->ParentWorkRect.Max.x;

					const auto saved_parent_work_rect_min_x = window->ParentWorkRect.Min.x;
					window->ParentWorkRect.Min.x += first_col_width;

					ImGui::Style_InvisibleSelectorPush();
					if (ImGui::Selectable(shared::utils::va("##CullAreaSelector%d", area_num), false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(max_x - min_x, content_height))) {
						area_selection = &a;
					}
					ImGui::Style_InvisibleSelectorPop();

					if (ImGui::IsItemHovered()) {
						ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.6f)));///*ImGui::GetColorU32(ImGuiCol_TableRowBgAlt)*/);
					}

					window->ParentWorkRect.Min.x = saved_parent_work_rect_min_x;
				}

				row_num++;
			} // table end for loop

			if (!area_selection_matches_any_entry)
			{
				// re-check for new selection (moving towards the start of the table)
				for (auto& [a_num, a] : areas)
				{
					if (area_selection && area_selection == &a)
					{
						area_selection_matches_any_entry = true;
						break;
					}
				}

				if (!area_selection_matches_any_entry) {
					area_selection = nullptr;
				}
			}

			ImGui::EndTable();
		} // table end

		bool was_area_removed = false;
		const auto it = areas.find(g_current_area);
		const auto can_area_be_added = it == areas.end();
		{
			ImGui::BeginDisabled(!can_area_be_added);
			ImGui::Style_ColorButtonPush(imgui::get()->ImGuiCol_ButtonGreen, true);
			if (ImGui::Button("Add Current Area##Cull", ImVec2(ImGui::GetContentRegionAvail().x * (area_selection ? 0.5f : 1.0f), 0)))
			{
				areas.emplace((std::uint32_t)g_current_area, map_settings::area_overrides_s{
						.nocull_distance = map_settings::get_map_settings().default_nocull_dist,
						.area_index = (std::uint32_t)g_current_area,
					});
			}
			ImGui::Style_ColorButtonPop();
			ImGui::EndDisabled();
		}

		if (area_selection)
		{
			ImGui::SameLine();
			ImGui::Style_ColorButtonPush(imgui::get()->ImGuiCol_ButtonRed, true);
			if (ImGui::Button("X Remove Selected Area Entry##Cull", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
			{
				// if selection = the area the player is in
				if ((int)area_selection->area_index == g_current_area)
				{
					areas.erase(it);
					g_player_current_area_override = nullptr;
				}
				else {
					areas.erase(area_selection->area_index);
				}

				was_area_removed = true;
			}
			ImGui::Style_ColorButtonPop();
		}

		ImGui::Spacing();
	}

	void cont_mapsettings_confvar()
	{
		const auto& vars = shared::common::remix_vars::get();

		ImGui::PushFont(shared::imgui::font::BOLD);
		if (ImGui::Button("Reset Vars to Level State   " ICON_FA_REPLY_ALL "##ConfvarReset", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0))) {
			vars.reset_all_modified(true);
		} ImGui::PopFont(); TT("This resets all remix vars back to level state (same as when map loads)");

		ImGui::SameLine();
		reload_mapsettings_button_with_popup("Confvar");
		ImGui::Spacing(0, 2);

		// we have no info about settings changed via the in-game remix menu so this is not of much use rn
		/*ImGui::PushFont(common::imgui::font::BOLD);
		if (ImGui::Button("Copy Changed Vars to Clipboard   " ICON_FA_SAVE, ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0)))
		{
			ImGui::LogToClipboard();
			for (auto& v : vars->options)
			{
				if (v.second.modified) {
					ImGui::LogText("%s", vars->get_config_string_for_option(v).c_str());
				}
			}

			ImGui::LogFinish();
		} ImGui::PopFont();
		ImGui::SameLine();*/

		static std::string conf_str1, conf_str2;
		static float conf1_transition_time = 0.0f, conf2_transition_time = 0.0f;
		static shared::common::remix_vars::EASE_TYPE conf1_mode = shared::common::remix_vars::EASE_TYPE_SIN_IN, conf2_mode = shared::common::remix_vars::EASE_TYPE_SIN_IN;
		static std::vector<std::string> configs;
		static bool loaded_configs = false;

		ImGui::PushFont(shared::imgui::font::BOLD);
		if (ImGui::Button("Refresh Configs   " ICON_FA_REDO, ImVec2(ImGui::GetContentRegionAvail().x, 0)) || !loaded_configs)
		{
			configs.clear();
			if (!shared::globals::root_path.empty())
			{
				std::string conf_path = shared::globals::root_path + "\\rtx_comp\\map_configs\\";
				if (std::filesystem::exists(conf_path))
				{
					for (const auto& d : std::filesystem::directory_iterator(conf_path))
					{
						if (d.path().extension() == ".conf")
						{
							auto file = std::filesystem::path(d.path());
							configs.push_back(file.filename().string());
						}
					}
				}
				loaded_configs = true;
			}
		}
		ImGui::PopFont();
		ImGui::Spacing(0, 6);

		{

			ImGui::TableHeaderDropshadow();
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);
			ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(ImGuiCol_FrameBgActive));
			if (ImGui::BeginListBox("##listbox1", ImVec2(ImGui::GetContentRegionAvail().x, 130.0f)))
			{
				for (const auto& str : configs)
				{
					const bool is_selected = conf_str1 == str;
					if (ImGui::Selectable(str.c_str(), is_selected)) {
						conf_str1 = str;
					}

					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::Spacing(0, 4);

			SET_CHILD_WIDGET_WIDTH_MAN(120.0f);
			if (ImGui::DragFloat("Transition Time##1", &conf1_transition_time, 0.005f, 0.0f)) {
				conf1_transition_time = std::clamp(conf1_transition_time, 0.0f, FLT_MAX);
			}

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
			if (ImGui::BeginCombo("##ModeSelector1", shared::common::remix_vars::EASE_TYPE_STR[conf1_mode], ImGuiComboFlags_None))
			{
				for (std::uint32_t n = 0u; n < (std::uint32_t)IM_ARRAYSIZE(shared::common::remix_vars::EASE_TYPE_STR); n++)
				{
					const bool is_selected = conf1_mode == n;
					if (ImGui::Selectable(shared::common::remix_vars::EASE_TYPE_STR[n], is_selected)) {
						conf1_mode = (shared::common::remix_vars::EASE_TYPE)n;
					}

					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}

				}
				ImGui::EndCombo();
			}

			ImGui::SameLine();
			ImGui::BeginDisabled(conf_str1.empty());

			const auto btn_to_label_size = ImGui::CalcWidgetWidthForChild(120.0f);
			if (ImGui::Button("Trigger##1", ImVec2(btn_to_label_size, 0)))
			{
				std::string conf_name = conf_str1;
				if (!conf_name.ends_with(".conf")) {
					conf_name += ".conf";
				}

				vars.parse_and_apply_conf_with_lerp(
					conf_name,
					shared::utils::string_hash64(conf_name),
					conf1_mode,
					conf1_transition_time);
			}
			ImGui::EndDisabled();
		}

		ImGui::Spacing(0, 8);
		ImGui::Separator();
		ImGui::Spacing(0, 6);

		{
			ImGui::TableHeaderDropshadow();
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);
			ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(ImGuiCol_FrameBgActive));
			if (ImGui::BeginListBox("##listbox2", ImVec2(ImGui::GetContentRegionAvail().x, 130.0f)))
			{
				for (const auto& str : configs)
				{
					const bool is_selected = conf_str2 == str;
					if (ImGui::Selectable(str.c_str(), is_selected)) {
						conf_str2 = str;
					}

					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::Spacing(0, 4);

			SET_CHILD_WIDGET_WIDTH_MAN(120.0f);
			if (ImGui::DragFloat("Transition Time##2", &conf2_transition_time, 0.005f, 0.0f)) {
				conf2_transition_time = std::clamp(conf2_transition_time, 0.0f, FLT_MAX);
			}

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
			if (ImGui::BeginCombo("##ModeSelector2", shared::common::remix_vars::EASE_TYPE_STR[conf2_mode], ImGuiComboFlags_None))
			{
				for (std::uint32_t n = 0u; n < (std::uint32_t)IM_ARRAYSIZE(shared::common::remix_vars::EASE_TYPE_STR); n++)
				{
					const bool is_selected = conf2_mode == n;
					if (ImGui::Selectable(shared::common::remix_vars::EASE_TYPE_STR[n], is_selected)) {
						conf2_mode = (shared::common::remix_vars::EASE_TYPE)n;
					}

					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}

				}
				ImGui::EndCombo();
			}

			ImGui::SameLine();
			ImGui::BeginDisabled(conf_str2.empty());

			const auto btn_to_label_size = ImGui::CalcWidgetWidthForChild(120.0f);
			if (ImGui::Button("Trigger##2", ImVec2(btn_to_label_size, 0)))
			{
				std::string conf_name = conf_str2;
				if (!conf_name.ends_with(".conf")) {
					conf_name += ".conf";
				}

				vars.parse_and_apply_conf_with_lerp(
					conf_name,
					shared::utils::string_hash64(conf_name),
					conf2_mode,
					conf2_transition_time);
			}
			ImGui::EndDisabled();
		}

		ImGui::Spacing(0, 4);
	}

	void imgui::tab_map_settings()
	{
		// general settings
		{
			static float cont_general_height = 0.0f;
			cont_general_height = ImGui::Widget_ContainerWithCollapsingTitle("General Settings", cont_general_height, cont_mapsettings_general,
				true, ICON_FA_ELLIPSIS_H, &ImGuiCol_ContainerBackground, &ImGuiCol_ContainerBorder);
		}

		ImGui::Spacing(0, 6.0f);
		ImGui::SeparatorText("The following settings do NOT auto-save.");
		ImGui::TextDisabled("Export to clipboard and override the settings manually!");
		ImGui::Spacing(0, 6.0f);

		// marker manipulation
		{
			static float cont_marker_manip_height = 0.0f;
			cont_marker_manip_height = ImGui::Widget_ContainerWithCollapsingTitle("Marker Manipulation", cont_marker_manip_height, cont_mapsettings_marker_manipulation,
				false, ICON_FA_DICE_D6, &ImGuiCol_ContainerBackground, &ImGuiCol_ContainerBorder);
		}

		// culling manipulation
		{
			static float cont_cull_manip_height = 0.0f;
			cont_cull_manip_height = ImGui::Widget_ContainerWithCollapsingTitle("Culling Manipulation", cont_cull_manip_height, cont_mapsettings_culling_manipulation,
				false, ICON_FA_EYE_SLASH, &ImGuiCol_ContainerBackground, &ImGuiCol_ContainerBorder);
		}

		// config vars
		{
			static float cont_vars_height = 0.0f;
			cont_vars_height = ImGui::Widget_ContainerWithCollapsingTitle("Configvars / Transitions", cont_vars_height, cont_mapsettings_confvar,
				false, ICON_FA_PAINT_BRUSH, &ImGuiCol_ContainerBackground, &ImGuiCol_ContainerBorder);
		}

		m_devgui_custom_footer_content = "Area: " + std::to_string(g_current_area) + "\nLeaf: " + std::to_string(g_current_leaf);
	}

	void imgui::devgui()
	{
		ImGui::SetNextWindowSize(ImVec2(900, 800), ImGuiCond_FirstUseEver);

		bool old_active_state = shared::globals::imgui_menu_open;
		if (!ImGui::Begin("Devgui", &shared::globals::imgui_menu_open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse, &shared::imgui::draw_window_blur_callback))
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

#if DEBUG
			ADD_TAB("Dev", tab_general);
#endif
			ADD_TAB("Game Settings", tab_game_settings);
			ADD_TAB("Map Settings", tab_map_settings);
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
		// yea .. does not fit in here but yolo
		renderer::on_present();

		if (auto* im = imgui::get(); im)
		{
			if (const auto dev = game::get_d3d_device(); dev)
			{
				if (!im->m_initialized_device)
				{
					ImGui_ImplDX9_Init(dev);
					im->m_initialized_device = true;
				}

				if (im->m_initialized_device)
				{
					// needed for blur
					shared::globals::d3d_device = game::get_d3d_device();

					// fix imgui colors / background if no hud elem is visible
					DWORD og_srgb_samp, og_srgb_write;
					dev->GetSamplerState(0, D3DSAMP_SRGBTEXTURE, &og_srgb_samp);
					dev->GetRenderState(D3DRS_SRGBWRITEENABLE, &og_srgb_write);
					dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 1);
					dev->SetRenderState(D3DRS_SRGBWRITEENABLE, 1);

					ImGui_ImplDX9_NewFrame();
					ImGui_ImplWin32_NewFrame();
					ImGui::NewFrame();

					if (shared::globals::imgui_menu_open) {
						im->devgui();
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

	using present_fn = long(__stdcall*)(IDirect3DDevice9*, RECT*, RECT*, HWND, RGNDATA*); present_fn present_original = {};
	long __stdcall present_hk(IDirect3DDevice9* device, RECT* source_rect, RECT* dest_rect, HWND dest_window_override, RGNDATA* dirty_region)
	{
		imgui::on_present();
		return present_original(device, source_rect, dest_rect, dest_window_override, dirty_region);
	}

	using reset_fn = long(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*); reset_fn reset_original = {};
	long __stdcall reset_hk(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* present_parameters)
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();
		const auto result = reset_original(device, present_parameters);
		ImGui_ImplDX9_CreateDeviceObjects();
		return result;
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

		auto get_virtual = [](void* _class, unsigned int index) {
			return static_cast<unsigned int>((*static_cast<int**>(_class))[index]);
			};

		const auto dev = game::get_d3d_device();
		MH_CreateHook(reinterpret_cast<void*>(get_virtual(dev, 17)), present_hk, reinterpret_cast<void**>(&present_original));
		MH_CreateHook(reinterpret_cast<void*>(get_virtual(dev, 16)), reset_hk, reinterpret_cast<void**>(&reset_original));
		MH_EnableHook(MH_ALL_HOOKS);

		m_initialized = true;
		std::cout << "[IMGUI] loaded\n";
	}
}
