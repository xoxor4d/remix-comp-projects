#include "std_include.hpp"
#include "remix_api.hpp"
#include "bridge_remix_api.h"

namespace shared::common
{
	// gets the singleton instance
	remix_api& remix_api::get()
	{
		static remix_api instance;
		return instance;
	}

	void remix_api::initialize(
		PFN_remixapi_BridgeCallback begin_scene_callback,
		PFN_remixapi_BridgeCallback end_scene_callback,
		PFN_remixapi_BridgeCallback on_present_callback, 
		bool is_asi)
	{
		auto& instance = get();
		if (!instance.m_initialized)
		{
			if (const auto status = remixapi::bridge_initRemixApi(&instance.m_bridge, is_asi);
				status == REMIXAPI_ERROR_CODE_SUCCESS)
			{
				instance.m_initialized = true;

				if (begin_scene_callback || end_scene_callback || on_present_callback) {
					remixapi::bridge_setRemixApiCallbacks(begin_scene_callback, end_scene_callback, on_present_callback);
				}

				printf("[RemixApi] Initialized remixApi!\n");
			}
			else { console(); printf("[!][RemixApi] Failed to initialize the remixApi - Code: %d\n", status); }
		}
	}
}
