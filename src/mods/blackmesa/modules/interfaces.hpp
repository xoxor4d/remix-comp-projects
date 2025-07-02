#pragma once
#include "mods/blackmesa/sdk/engine/c_engine_client.hpp"
#include "mods/blackmesa/sdk/entity/c_entity_list.hpp"
#include "mods/blackmesa/sdk/vgui/surface/c_surface_mgr.h"

namespace mods::blackmesa
{
	class interfaces final : public shared::common::loader::component_module
	{
	public:
		interfaces();

		static inline interfaces* p_this = nullptr;
		static interfaces* get() { return p_this; }
		sdk::base_client* m_client = nullptr;
		sdk::engine_client* m_engine = nullptr;
		sdk::entity_list* m_entity_list = nullptr;
		sdk::surface* m_surface = nullptr;

		static bool is_initialized()
		{
			if (const auto im = get(); im && im->m_initialized) {
				return true;
			}
			return false;
		}

	private:
		bool m_initialized = false;
		template <typename m_interface>
		static m_interface* get_interface(const std::string& module_name, const std::string& interface_name);
	};
}
