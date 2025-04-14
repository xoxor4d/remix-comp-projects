#include "std_include.hpp"
#include "patches.hpp"

#include "imgui.hpp"
#include "shared/common/remix_api.hpp"

namespace mods::anno1404
{
	void begin_scene_cb()
	{
		D3DXMATRIX view_matrix
		(
			1.0f, 0.0f, 0.0f, 0.0f,    
			0.0f, 0.447f, 0.894f, 0.0f,
			0.0f, -0.894f, 0.447f, 0.0f,
			0.0f, 100.0f, -50.0f, 1.0f
		);

		D3DXMATRIX proj_matrix
		(
			1.359f, 0.0f, 0.0f, 0.0f,
			0.0f, 2.414f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.001f, 1.0f,
			0.0f, 0.0f, -1.0f, 0.0f
		);

		const auto& im = imgui::get();

		// Construct view matrix
		D3DXMATRIX rotation, translation;
		D3DXMatrixRotationYawPitchRoll(&rotation,
			D3DXToRadian(im->m_dbg_camera_yaw),   // Yaw in radians
			D3DXToRadian(im->m_dbg_camera_pitch), // Pitch in radians
			0.0f);                      // No roll for simplicity

		D3DXMatrixTranslation(&translation,
			-im->m_dbg_camera_pos[0], // Negate for camera (moves world opposite)
			-im->m_dbg_camera_pos[1],
			-im->m_dbg_camera_pos[2]);

		D3DXMatrixMultiply(&view_matrix, &rotation, &translation);

		// Alternative: Use look-at if preferred
		// D3DXVECTOR3 eye(camera_pos[0], camera_pos[1], camera_pos[2]);
		// D3DXVECTOR3 at(0, 0, 0); // Target at origin, adjust as needed
		// D3DXVECTOR3 up(0, 1, 0);
		// D3DXMatrixLookAtLH(&view_matrix, &eye, &at, &up);

		// Construct projection matrix
		D3DXMatrixPerspectiveFovLH(&proj_matrix,
			D3DXToRadian(im->m_dbg_camera_fov), // FOV in radians
			im->m_dbg_camera_aspect,
			im->m_dbg_camera_near_plane,
			im->m_dbg_camera_far_plane);


		shared::globals::d3d_device->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
		shared::globals::d3d_device->SetTransform(D3DTS_VIEW, &view_matrix);
		shared::globals::d3d_device->SetTransform(D3DTS_PROJECTION, &proj_matrix);
	}

	void end_scene_cb()
	{

	}

	void present_scene_cb()
	{
		imgui::get()->on_present();
	}

	patches::patches()
	{
		p_this = this;
		shared::common::remix_api::initialize(begin_scene_cb, end_scene_cb, present_scene_cb);
		printf("[Module] patches loaded.\n");
	}
}
