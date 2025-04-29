#pragma once

namespace mods::swat4::game
{
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
