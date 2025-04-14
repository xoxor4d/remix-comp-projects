#pragma once
#include "shared/common/remix_api.hpp"

namespace mods::mirrorsedge
{
	class patches final : public component_module
	{
	public:
		patches();

		static inline patches* p_this = nullptr;
		static patches* get() { return p_this; }

		static void pre_drawindexedprim_call();
		static void post_drawindexedprim_call();
	};
}
