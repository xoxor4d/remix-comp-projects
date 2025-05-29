#pragma once
#include "shared/common/remix_vars.hpp"

namespace mods::blackmesa
{
	class map_settings final : public shared::common::loader::component_module
	{
	public:
		map_settings();
		~map_settings();

		static inline map_settings* p_this = nullptr;
		static map_settings* get() { return p_this; }

		struct marker_settings_s
		{
			std::uint32_t index = 0;
			Vector origin = {};
			Vector rotation = { 0.0f, 0.0f, 0.0f };
			Vector scale = { 1.0f, 1.0f, 1.0f }; // no_cull only
			std::unordered_set<std::uint32_t> areas; // no_cull only
			std::string comment;

			void* handle = nullptr; // internal use
			bool is_hidden = false; // internal use
		};

		struct api_config_var
		{
			std::string variable;
			std::string value;
		};

		// ---

		static constexpr float DEFAULT_NOCULL_DIST = 1000.0f;

		struct area_overrides_s
		{
			float nocull_distance = DEFAULT_NOCULL_DIST;
			std::uint32_t area_index;
		};

		struct map_settings_s
		{
			std::string	mapname;
			float water_uv_scale = 1.0f;
			float water_uv_top_scale = 0.0f;
			float water_offset_top = 0.5f; // top layer
			float water_offset_bottom = 0.0f; // bottom layer
			std::unordered_map<std::uint32_t, area_overrides_s> area_settings;
			float default_nocull_dist = DEFAULT_NOCULL_DIST;
			std::vector<marker_settings_s> map_markers;
			std::vector<std::string> api_var_configs;
		};

		static map_settings_s& get_map_settings() { return m_map_settings; }
		static const std::string& get_map_name() { return m_map_settings.mapname; }

		static std::string build_map_marker_string_for_current_map(const std::vector<map_settings::marker_settings_s>& markers);
		static std::string build_culling_overrides_string_for_current_map(const std::unordered_map<std::uint32_t, map_settings::area_overrides_s>& areas);

		void set_settings_for_map(const std::string& map_name);
		static void on_map_load(const std::string& map_name);
		static void on_map_unload();
		static void clear_map_settings();
		static void reload();

	private:
		static inline map_settings_s m_map_settings = {};
		static inline std::vector<std::string> m_args;
		static inline bool m_loaded = false;

		bool parse_toml();
		bool matches_map_name();
		void open_and_set_var_config(const std::string& config, bool no_error = false, bool ignore_hashes = false, const char* custom_path = nullptr);
	};
}
