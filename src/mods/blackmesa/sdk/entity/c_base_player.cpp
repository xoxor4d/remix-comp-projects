#include <std_include.hpp>
#include "c_base_entity.hpp"
#include "mods/blackmesa/modules/interfaces.hpp"

namespace sdk
{
	Vector c_base_player::get_eye_pos()
	{
		return get_vec_origin() + get_view_offset();
	}

	/*c_base_weapon* c_base_player::get_active_weapon()
	{
		auto active_weapon = read<uintptr_t>(g_netvars.get_netvar(shared::utils::fnv::hash("DT_CSPlayer"), shared::utils::fnv::hash("m_hActiveWeapon"))) & 0xFFF;
		return reinterpret_cast<c_base_weapon*>(mods::blackmesa::interfaces::get()->m_entity_list->get_client_entity(active_weapon));
	}*/
}

