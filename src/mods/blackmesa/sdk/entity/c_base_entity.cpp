#include <std_include.hpp>
#include "c_base_entity.hpp"

#include "mods/blackmesa/sdk/client/c_base_client.hpp"
#include "mods/blackmesa/sdk/client/c_collideable.hpp"

namespace sdk
{
	void* c_base_entity::networkable()
	{
		const auto ret = reinterpret_cast<void*>(uintptr_t(this) + 8); // third func in IClientNetworkable
		return ret;
	}

	/*
	collideable_t* c_base_entity::get_collideable()
	{
		using original_fn = collideable_t * (__thiscall*)(void*);
		return (*(original_fn * *)this)[3](this);
	}
	
	Vector c_base_entity::get_absolute_origin()
	{
		if (!this)
			return Vector(0, 0, 0);
	
		using original_fn = Vector & (__thiscall*)(c_base_entity*);
		return (*(original_fn * *)this)[10](this);
	}*/
	
	c_client_class* c_base_entity::client_class()
	{
		using original_fn = c_client_class* (__thiscall*)(void*);

		auto x = (*(original_fn**)networkable());
		auto z = x[2];
		auto y = z(networkable());
		return y;
		//return (*(original_fn**)networkable())[1](networkable());
	}
	
	/*bool c_base_entity::is_dormant()
	{
		using original_fn = bool(__thiscall*)(void*);
		return (*static_cast<original_fn**>(networkable()))[7](networkable());
	}*/
	
	Vector c_base_entity::get_eye_pos()
	{
		return origin() + view_offset();
	}
}
