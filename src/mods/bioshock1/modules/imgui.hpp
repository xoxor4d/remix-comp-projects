#pragma once

namespace mods::bioshock1
{
	class imgui final : public component_module
	{
	public:
		imgui();

		static inline imgui* p_this = nullptr;
		static imgui* get() { return p_this; }

		static void on_present();

		void devgui();
		bool input_message(UINT message_type, WPARAM wparam, LPARAM lparam, bool& inout_pass_msg_to_game);

		//bool m_menu_active = false;
		bool m_initialized_device = false;
		//bool m_is_rendering = false;

		void style_xo();

		ImVec4 ImGuiCol_ButtonGreen = ImVec4(0.3f, 0.4f, 0.05f, 0.7f);
		ImVec4 ImGuiCol_ButtonYellow = ImVec4(0.4f, 0.3f, 0.1f, 0.8f);
		ImVec4 ImGuiCol_ButtonRed = ImVec4(0.48f, 0.15f, 0.15f, 1.00f);
		ImVec4 ImGuiCol_ContainerBackground = ImVec4(0.220f, 0.220f, 0.220f, 0.863f);
		ImVec4 ImGuiCol_ContainerBorder = ImVec4(0.099f, 0.099f, 0.099f, 0.901f);

		Vector m_debug_vector = { 0.0f, 0.0f, 0.0f };
		Vector m_debug_vector2 = { 0.0f, 0.0f, 0.0f };

		// world projection matrix parameters
		bool  m_world_use_custom_proj = false;
		float m_world_proj_fov = 90.0f;         // vertical FOV in degrees
		float m_world_proj_aspect = 1.777f;     // 16:9 aspect ratio

		// viewmodel projection matrix parameters
		bool m_viewmodel_use_custom_proj = false;
		float m_viewmodel_proj_fov = 90.0f;         // vertical FOV in degrees
		float m_viewmodel_proj_aspect = 1.777f;     // 16:9 aspect ratio
		float m_viewmodel_proj_near_plane = 1.0f;   // near clipping plane
		float m_viewmodel_proj_far_plane = 1000.0f; // far clipping plane

		bool m_anti_culling_tweak1 = true;
		bool m_anti_culling_tweak2 = false;

		/*bool is_imgui_game_input_allowed() const {
			return m_im_allow_game_input;
		}*/

	private:
		void tab_general();
		bool m_im_window_focused = false;
		bool m_im_window_hovered = false;
		//bool m_im_allow_game_input = false;
		std::string m_devgui_custom_footer_content;

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
