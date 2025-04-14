#pragma once

namespace shared::common::remix
{
	void set_texture_category(IDirect3DDevice9* dev, const std::uint32_t& cat);
    void reset_texture_category(IDirect3DDevice9* dev);
}
