#pragma once

namespace mods::anno1404::game
{
	struct camera_handle
	{
		char pad_0x0000[0x90]; //0x0000
		float fov_readonly; //0x0090 
		char pad_0x0094[0x24]; //0x0094
		float camera_pos[3]; //0x00B8 
		char pad_0x00C4[0x20]; //0x00C4
		D3DXMATRIX viewMatrix; //0x00E4 
		D3DXMATRIX unkProjMatrix; //0x0124 
		D3DXMATRIX unkViewMatrix; //0x0164 
		D3DXMATRIX unkProjMatrix2; //0x01A4 
		D3DXMATRIX projMatrix; //0x01E4 
		D3DXMATRIX unkProjMatrix3; //0x0224 
		char pad_0x0264[0x220]; //0x0264
	};

	struct cam_struct
	{
		char pad_0x0000[0xAB8]; //0x0000
		camera_handle* cam; //0x0AB8 
		char pad_0x0ABC[0x584]; //0x0ABC
	}; //Size=0x1040

	struct towards_cam2
	{
		cam_struct* cam_struct_ptr;
	};

	struct towards_cam1
	{
		char pad_0x0000[0x29C]; //0x0000
		towards_cam2* towards_cam2_ptr; //0x029C 
	};

	struct engine_interface
	{
		char pad_0x0000[0xFC]; //0x0000
		towards_cam1* towards_cam_ptr; //0x00FC 
	};

	struct engine_interface_obj
	{
		engine_interface* ptr;
	};
}
