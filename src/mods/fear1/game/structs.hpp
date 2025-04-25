#pragma once

namespace mods::fear1::game
{
	struct mesh_info
	{
		char pad[23];
		std::uint8_t bone_count;
	};

	enum EAABBCorner
	{
		eAABB_NearTopLeft = 0,
		eAABB_NearTopRight = 1,
		eAABB_NearBottomLeft = 2,
		eAABB_NearBottomRight = 3,
		eAABB_FarTopLeft = 4,
		eAABB_FarTopRight = 5,
		eAABB_FarBottomLeft = 6,
		eAABB_FarBottomRight = 7,
		eAABB_None = 8,
	};

	struct LTPlane
	{
		Vector m_normal; //0x0000 
		float m_dist; //0x000C 
	}; //Size=0x0010

	struct ViewBoxDef
	{
		Vector2D m_COP;
		Vector2D m_WindowSize;
		float m_NearZ;
		float m_FarZ;
		float m_unk;
	};

	struct LTRectFloat
	{
		int left;
		int top;
		float right;
		float bottom;
	};

	struct LTRect
	{
		int left;
		int top;
		int right;
		int bottom;
	};

	struct viewParms
	{
		ViewBoxDef m_ViewBox;
		LTRectFloat m_RectFloat;
		LTRect m_Rect2;
		float m_fScreenWidth;
		float m_fScreenHeight;
		shared::float3x4 viewMatrix;
		D3DXMATRIX projMatrix;
		D3DXMATRIX m_DeviceTimesProjection;
		D3DXMATRIX m_FullTransform;
		D3DXMATRIX identityMatrix;
		Vector m_ViewPoints[8];
		Vector m_ViewAABBMin;
		Vector m_ViewAABBMax;
		LTPlane m_ClipPlanes[6];
		EAABBCorner m_AABBPlaneCorner0[6];
		Vector m_up;
		Vector m_rt;
		Vector m_fwd;
		Vector m_pos;
		Vector m_SkyViewPos;
		char pad_0x02A0[4];
		Vector unkpos;
		int pad_0x2B0;
		int pad_0x2B4;
		int pad_0x2B8;
		int pad_0x2BC;
		void** m_SkyObjects;
		int m_nSkyObjects;
		LTRect m_Rect;
		float m_xFov;
		float m_yFov;
		__int8 N00000737; //0x02E0 
		__int8 m_DrawMode_maybe; //0x02E1 
		__int8 N000007A5; //0x02E2 
		__int8 m_DrawMode_maybe2; //0x02E3 
		Vector m_Pos;
		Vector m_Rot;
		float some_random_one;
		Vector m_PosDupe;
		int pad_0x30C;
		int pad_0x310;
		int pad_0x314;
		int pad_0x318;
		int pad_0x31C;
		int pad_0x320;
		int pad_0x324;
		int pad_0x328;
		int pad_0x32C;
		int pad_0x330;
		int pad_0x334;
		int pad_0x338;
		int pad_0x33C;
		int pad_0x340;
		int pad_0x344;
		int pad_0x348;
		int pad_0x34C;
		int pad_0x350;
		int pad_0x354;
		int pad_0x358;
		int pad_0x35C;
		int pad_0x360;
		int pad_0x364;
		int pad_0x368;
		int pad_0x36C;
		int pad_0x370;
		int pad_0x374;
		int pad_0x378;
		int pad_0x37C;
		int pad_0x380;
		int pad_0x384;
		int pad_0x388;
		int pad_0x38C;
		int pad_0x390;
		int pad_0x394;
		int pad_0x398;
		int pad_0x39C;
		int pad_0x3A0;
		int pad_0x3A4;
		int pad_0x3A8;
		int pad_0x3AC;
		int pad_0x3B0;
		int pad_0x3B4;
		int pad_0x3B8;
		int pad_0x3BC;
		int pad_0x3C0;
		int pad_0x3C4;
		int pad_0x3C8;
		int pad_0x3CC;
		int pad_0x3D0;
		int pad_0x3D4;
		int pad_0x3D8;
		int pad_0x3DC;
		int pad_0x3E0;
		int pad_0x3E4;
		int pad_0x3E8;
		int pad_0x3EC;
		int pad_0x3F0;
		int pad_0x3F4;
		int pad_0x3F8;
		int pad_0x3FC;
		int pad_0x400;
		int pad_0x404;
		int pad_0x408;
		int pad_0x40C;
		int pad_0x410;
		int pad_0x414;
		int pad_0x418;
		int pad_0x41C;
		int pad_0x420;
		int pad_0x424;
		int pad_0x428;
		int pad_0x42C;
		int pad_0x430;
		int pad_0x434;
		int pad_0x438;
		int pad_0x43C;
	};

	STATIC_ASSERT_OFFSET(viewParms, unkpos, 0x2A4);
}
