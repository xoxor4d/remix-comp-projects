#pragma once

namespace mods::bioshock1::game
{
	struct UD3DRenderDevice;
	struct render_glob
	{
		char pad_0x0000[0x80]; //0x0000
		__int32 ViewportWidth; //0x0080 
		__int32 ViewportHeight; //0x0084 
		char pad_0x0088[0x1E4]; //0x0088
		UD3DRenderDevice* RenDev; //0x026C 
		char pad_0x0270[0x10]; //0x0270
		D3DXMATRIX worldMatrix; //0x0280 
		D3DXMATRIX viewMatrix; //0x02C0 
		D3DXMATRIX projMatrix; //0x0300 
		char pad_0x0340[0x50]; //0x0340
		void* N0000021D; //0x0390 
		char pad_0x0394[0x8AC]; //0x0394
	}; //Size=0x0C40

	struct UD3DRenderDevice
	{
		char pad_0x0000[0x150]; //0x0000
		float TesselationFactor; //0x0150 
		__int32 DesiredRefreshRate; //0x0154 
		char pad_0x0158[0x4]; //0x0158
		_D3DCAPS9 DeviceCaps; //0x015C 
		_D3DADAPTER_IDENTIFIER9 DeviceIdentifier; //0x028C 
		char pad_0x06D8[0xA8]; //0x06D8
		IDirect3D9* d3d9; //0x0780 
		IDirect3DDevice9* device; //0x0784 
		char pad_0x0788[0x17D8]; //0x0788
		render_glob RenderInterface; //0x1F60 
	}; //Size=0x07D4

	struct FPlane
	{
		float xyz[3];
		float dist;
	};

	struct FConvexVolume
	{
		FPlane BoundingPlanes[32];
	}; //Size=0x0200
	
}
