#pragma once
#include "bridge_remix_api.h"
#include "remix/remix_c.h"

namespace shared::common
{
	class remix_api
	{
	public:
		// enforce singleton pattern
		remix_api(const remix_api&) = delete;
		remix_api& operator=(const remix_api&) = delete;

		static remix_api& get();

		static void initialize(
			PFN_remixapi_BridgeCallback begin_scene_callback,
			PFN_remixapi_BridgeCallback end_scene_callback,
			PFN_remixapi_BridgeCallback on_present_callback,
			bool is_asi = false);

		static bool is_initialized() { return get().m_initialized; }

		remixapi_Interface m_bridge;

	private:
		remix_api() : m_initialized(false) {}
		bool m_initialized;
	};
}