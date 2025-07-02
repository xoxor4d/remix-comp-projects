#pragma once

namespace mods::blackmesa
{
	class imgui final : public shared::common::loader::component_module
	{
	public:
		imgui();

		static inline imgui* p_this = nullptr;
		static imgui* get() { return p_this; }

		static void on_present();
		static void on_map_load();

		void devgui();
		bool input_message(UINT message_type, WPARAM wparam, LPARAM lparam, bool& inout_pass_msg_to_game);

		bool m_initialized_device = false;

		void style_xo();

		ImVec4 ImGuiCol_ButtonGreen = ImVec4(0.3f, 0.4f, 0.05f, 0.7f);
		ImVec4 ImGuiCol_ButtonYellow = ImVec4(0.4f, 0.3f, 0.1f, 0.8f);
		ImVec4 ImGuiCol_ButtonRed = ImVec4(0.48f, 0.15f, 0.15f, 1.00f);
		ImVec4 ImGuiCol_ContainerBackground = ImVec4(0.220f, 0.220f, 0.220f, 0.863f);
		ImVec4 ImGuiCol_ContainerBorder = ImVec4(0.099f, 0.099f, 0.099f, 0.901f);

		Vector m_debug_vector = { 0.0f, 0.0f, 0.0f };
		Vector m_debug_vector2 = { 0.0f, 0.0f, 0.0f };

		bool m_debug_disable_rendering[64] = {};
		bool m_debug_disable_unbake = false;

		bool m_light_edit_mode = false;
		bool m_debugvis_live = false;
		bool m_debugvis_radius = true;
		bool m_debugvis_shaping = true;
		bool m_debugvis_attach_bounds = true;
		float m_debugvis_cone_height = 60.0f;
		int m_debugvis_cone_steps = 3u;

		int m_debug_int = {};

		static bool is_initialized()
		{
			if (const auto im = get(); im && im->m_initialized) {
				return true;
			}
			return false;
		}

	private:
		void tab_general();
		void tab_game_settings();
		void tab_map_settings();
		bool m_im_window_focused = false;
		bool m_im_window_hovered = false;
		std::string m_devgui_custom_footer_content;

		bool m_initialized = false;

		static void questionmark(const char* desc)
		{
			ImGui::TextDisabled("(?)");
			if (ImGui::BeginItemTooltip())
			{
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted(desc);
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}
		}
	};
}
