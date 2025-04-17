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

		static bool pre_drawindexedprim_call(IDirect3DDevice9* unhooked_device, D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);
		static void post_drawindexedprim_call();
		bool m_ff_use_shader = false;
		bool m_ff_was_modified = false;

		IDirect3DVertexShader9* m_ff_og_shader = nullptr;
		IDirect3DPixelShader9* m_ff_og_psshader = nullptr;

		struct vec4
		{
			float x, y, z, w;
		};

		struct mesh_inst_data
		{
			vec4 pos;
			vec4 rot;
			vec4 color;
		};

		std::vector<vec4> m_cInstanceBuffer;
	};
}
