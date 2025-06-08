#pragma once

namespace mods::gh3
{
	class imgui final : public shared::common::loader::component_module
	{
	public:
		imgui();

		static inline imgui* p_this = nullptr;
		static imgui* get() { return p_this; }

		static void on_present();

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


		bool m_dbg_disable_shaders = false;

		bool m_dbg_texture_stage1_hack = false;
		bool m_dbg_disable_stage0 = false;
		int m_dbg_texture_stage_index = 1;
		int m_dbg_textureflags_index = 0;
		int m_dbg_textureflags_index2 = 0;
		/*DWORD m_dbg_textureflags[12] =
		{
			0x00800100, 0x02000200, 0x01000100, 0x04000400, 0x00800080, 0x00400080, 0x00400040, 0x02000400, 0x00200040, 0x00200020, 0x01000200, 0x08000800
		};*/

		DWORD m_dbg_textureflags[9] =
		{
			0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000
		};

		bool m_dbg_use_fake_camera = false;
		float m_dbg_camera_pos[3] = { 0.0f, 0.0f, 1.0f }; // X, Y, Z
		float m_dbg_camera_yaw = 0.0f;   // Rotation around Y (degrees)
		float m_dbg_camera_pitch = 0.0f; // Rotation around X (degrees, downward tilt)
		float m_dbg_camera_fov = 60.0f;         // Vertical FOV in degrees
		float m_dbg_camera_aspect = 1.777f;     // 16:9 aspect ratio
		float m_dbg_camera_near_plane = 1.0f;   // Near clipping plane
		float m_dbg_camera_far_plane = 1000.0f; // Far clipping plane

		static bool is_initialized()
		{
			if (const auto im = imgui::get(); im && im->m_initialized){
				return true;
			}
			return false;
		}

	private:
		void tab_general();
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
