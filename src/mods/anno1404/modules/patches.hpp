#pragma once
#include "shared/common/remix_api.hpp"

namespace mods::anno1404
{
	class patches final : public component_module
	{
	public:
		patches();

		static inline patches* p_this = nullptr;
		static patches* get() { return p_this; }
	};
}
