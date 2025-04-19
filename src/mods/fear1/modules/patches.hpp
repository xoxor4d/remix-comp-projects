#pragma once
#include "shared/common/remix_api.hpp"

namespace mods::fear1
{
	class patches final : public shared::common::loader::component_module
	{
	public:
		patches();

		static inline patches* p_this = nullptr;
		static patches* get() { return p_this; }

		static bool pre_drawindexedprim_call(D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);
		static void post_drawindexedprim_call();

		bool m_ff_use_shader = false;
	};
}
