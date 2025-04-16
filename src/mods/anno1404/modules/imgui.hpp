#pragma once

namespace mods::anno1404
{
	class imgui final : public component_module
	{
	public:
		imgui();

		static inline imgui* p_this = nullptr;
		static imgui* get() { return p_this; }

		static void on_present();

		void devgui();
		bool input_message(UINT message_type, WPARAM wparam, LPARAM lparam);

		bool m_is_rendering = false;
		bool m_menu_active = false;
		bool m_initialized_device = false;

		void style_xo();

		ImVec4 ImGuiCol_ButtonGreen = ImVec4(0.3f, 0.4f, 0.05f, 0.7f);
		ImVec4 ImGuiCol_ButtonYellow = ImVec4(0.4f, 0.3f, 0.1f, 0.8f);
		ImVec4 ImGuiCol_ButtonRed = ImVec4(0.48f, 0.15f, 0.15f, 1.00f);
		ImVec4 ImGuiCol_ContainerBackground = ImVec4(0.220f, 0.220f, 0.220f, 0.863f);
		ImVec4 ImGuiCol_ContainerBorder = ImVec4(0.099f, 0.099f, 0.099f, 0.901f);

		Vector m_debug_vector = { 0.0f, 0.0f, 0.0f };
		Vector m_debug_vector2 = { 0.0f, 0.0f, 0.0f };

		bool m_dbg_use_fake_camera = false;

		// View matrix parameters
		float m_dbg_camera_pos[3] = { 0.0f, 0.0f, 1.0f }; // X, Y, Z
		float m_dbg_camera_yaw = 0.0f;   // Rotation around Y (degrees)
		float m_dbg_camera_pitch = 0.0f; // Rotation around X (degrees, downward tilt)

		// Projection matrix parameters
		float m_dbg_camera_fov = 60.0f;         // Vertical FOV in degrees
		float m_dbg_camera_aspect = 1.777f;     // 16:9 aspect ratio
		float m_dbg_camera_near_plane = 1.0f;   // Near clipping plane
		float m_dbg_camera_far_plane = 1000.0f; // Far clipping plane

		bool is_imgui_game_input_allowed() const {
			return m_im_allow_game_input;
		}

	private:
		void tab_general();
		bool m_im_window_focused = false;
		bool m_im_window_hovered = false;
		bool m_im_allow_game_input = false;
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
