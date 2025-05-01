#pragma once
#include "shared/common/remix_api.hpp"

namespace mods::ue2fixes
{
	namespace tex_addons
	{
		extern LPDIRECT3DTEXTURE9 sky_gray_up;
	}

	class patches final : public shared::common::loader::component_module
	{
	public:
		patches();

		static inline patches* p_this = nullptr;
		static patches* get() { return p_this; }

		void init_texture_addons(bool release = false);

	private:
		bool m_textures_initialized = false;
	};
}
