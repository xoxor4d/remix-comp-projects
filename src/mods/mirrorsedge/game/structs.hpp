#pragma once

namespace mods::mirrorsedge::game
{
	struct render_glob
	{
		char pad_0x0000[0x280]; //0x0000
		D3DXMATRIX worldMatrix; //0x0280 
		D3DXMATRIX viewMatrix; //0x02C0 
		D3DXMATRIX projMatrix; //0x0300 
		char pad_0x0340[0x500]; //0x0340

	};
}
