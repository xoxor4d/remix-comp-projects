#include "std_include.hpp"
#include "c_surface_mgr.h"

namespace sdk
{
	void surface::set_cursor_always_visible(bool v) {
		shared::utils::mem::virtual_function<void(__thiscall*)(void*, bool)>(this, 56)(this, v);
	}

	bool surface::is_cursor_visible() {
		return shared::utils::mem::virtual_function<bool(__thiscall*)(void*)>(this, 57)(this);
	}

	void surface::unlock_cursor() {
		shared::utils::mem::virtual_function<void(__thiscall*)(void*)>(this, 65)(this);
	}

	void surface::lock_cursor() {
		shared::utils::mem::virtual_function<void(__thiscall*)(void*)>(this, 66)(this);
	}
}
