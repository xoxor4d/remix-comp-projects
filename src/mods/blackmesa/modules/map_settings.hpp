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

		struct remix_light_settings_s
		{
			struct point_s
			{
				Vector position;
				Vector radiance;
				float radiance_scalar = 1.0f;
				float radius = 1.0f;
				float timepoint = 0.0f;
				float smoothness = 0.5f;

				// shaping
				bool use_shaping = false;
				Vector direction = { 0.0f, 0.0f, 1.0f };
				float degrees = 180.0; // cone angle
				float softness = 0.0f; // cone
				float exponent = 0.0f; // focus

				// volumetric
				float volumetric_scale = 1.0f;
			};

			std::vector<point_s> points;
			bool run_once = false;
			bool loop = false;
			bool loop_smoothing = false;
			bool trigger_always = false;

			std::string trigger_choreo_name;
			std::string trigger_choreo_actor;
			std::string trigger_choreo_event;
			std::string trigger_choreo_param1;
			std::uint32_t trigger_sound_hash;
			float trigger_delay = 0.0f;

			std::string kill_choreo_name;
			std::uint32_t kill_sound_hash;
			float kill_delay = 0.0f;

			float attach_prop_radius = 0.0f;
			std::string attach_prop_name;
			Vector attach_prop_mins; // min bounds
			Vector attach_prop_maxs; // max bounds

			std::string comment;
		};

		// ---

		static constexpr float DEFAULT_NOCULL_DIST = 1000.0f;

		struct area_overrides_s
		{
			float nocull_distance = DEFAULT_NOCULL_DIST;
			std::uint32_t area_index;
		};

		struct hide_models_s
		{
			std::unordered_set<std::string> substrings;
			std::unordered_set<float> radii;
		};

		struct unbake_models_s
		{
			std::unordered_set<int> checksums;
			std::unordered_set<std::string> strings;
		};

		struct map_settings_s
		{
			std::string	mapname;
			float water_uv_scale = 1.0f;
			float water_uv_bottom_scale = 0.0f;
			float water_offset_top = 0.5f; // top layer
			float water_scale_top = 1.0f; // top layer
			float water_offset_base = 0.0f; // base layer
			std::unordered_map<std::uint32_t, area_overrides_s> area_settings;
			float default_nocull_dist = DEFAULT_NOCULL_DIST;
			hide_models_s hide_models;
			//std::unordered_set<std::string> unbake_models;
			unbake_models_s unbake_models;
			//std::vector<remix_light_settings_s> remix_lights;
			std::vector<marker_settings_s> map_markers;
			std::vector<std::string> api_var_configs;
			bool using_any_light_attached_to_prop = false;
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
