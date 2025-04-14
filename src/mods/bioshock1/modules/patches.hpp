#pragma once
#include "shared/common/remix_api.hpp"

namespace mods::bioshock1
{
	class patches final : public component_module
	{
	public:
		patches();

		static inline patches* p_this = nullptr;
		static patches* get() { return p_this; }

		static bool pre_drawindexedprim_call();
		static void post_drawindexedprim_call();

		bool m_ff_use_shader = false;
	};
}
