#pragma once

namespace sdk
{
#define VGUI_MAT_SURFACE_INTERFACE_VERSION "VGUI_Surface030"

	class surface {
	public:
		void set_cursor_always_visible(bool v);
		bool is_cursor_visible();
		void unlock_cursor();
		void lock_cursor();
	};
}
