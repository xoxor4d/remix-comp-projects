#include "std_include.hpp"
#include "map_settings.hpp"

#include "game_settings.hpp"
#include "imgui.hpp"
#include "interfaces.hpp"
#include "remix_lights.hpp"
#include "shared/common/flags.hpp"
#include "shared/common/remix_api.hpp"
#include "shared/common/toml.hpp"

namespace mods::blackmesa
{
#define CATCH_ERR	catch (toml::type_error& err) { shared::common::console(); printf("%s\n", err.what()); return; }

	// format 2 decimals
	inline std::string format_float(float value)
	{
		return std::format("{:.2f}", value);
	}

	/// Builds a string containing all map markers for the current map
	/// @param areas	map_settings -> area overrides
	/// @return			the final string in toml format
	std::string map_settings::build_map_marker_string_for_current_map(const std::vector<map_settings::marker_settings_s>& markers)
	{
		std::string toml_str = map_settings::get_map_settings().mapname + " = [\n"s;
		for (auto& m : markers)
		{
			if (!m.comment.empty()) {
				toml_str += "\n        # " + m.comment + "\n";
			}

			toml_str += "        { " + ("nocull = "s) + std::to_string(m.index);

			//if (m.no_cull)
			{
				toml_str += ", areas = [";
				for (auto it = m.areas.begin(); it != m.areas.end(); ++it)
				{
					if (it != m.areas.begin()) {
						toml_str += ", ";
					}
					toml_str += std::to_string(*it);
				}
				toml_str += "]";
			}

			toml_str += ", position = [" + format_float(m.origin.x) + ", " + format_float(m.origin.y) + ", " + format_float(m.origin.z) + "]";
			toml_str += ", rotation = [" + format_float(RAD2DEG(m.rotation.x)) + ", " + format_float(RAD2DEG(m.rotation.y)) + ", " + format_float(RAD2DEG(m.rotation.z)) + "]";

			//if (m.no_cull) {
				toml_str += ", scale = [" + format_float(m.scale.x) + ", " + format_float(m.scale.y) + ", " + format_float(m.scale.z) + "]";
			//}

			toml_str += " },\n";
		}
		toml_str += "    ]";

		return toml_str;
	}

	/// Builds a string containing all culling overrides for the current map
	/// @param areas	map_settings -> area overrides
	/// @return			the final string in toml format
	std::string map_settings::build_culling_overrides_string_for_current_map(const std::unordered_map<std::uint32_t, map_settings::area_overrides_s>& areas)
	{
		// temp map to sort areas by area number
		std::map<int, map_settings::area_overrides_s> sorted_area_settings(areas.begin(), areas.end());

		std::string toml_str = map_settings::get_map_settings().mapname + " = [\n"s;
		for (auto& [ar_num, a] : sorted_area_settings)
		{
			// {
			{
				const auto num_spaces = ar_num < 10 ? 2u : ar_num < 100 ? 1u : 0u;
				toml_str += "        { in_area = ";

				for (auto i = 0u; i < num_spaces; i++) {
					toml_str += " ";
				}

				toml_str += std::to_string(ar_num);
			}

			//if (a.cull_mode >= map_settings::AREA_CULL_INFO_NOCULLDIST_START
			//	&& a.cull_mode <= map_settings::AREA_CULL_INFO_NOCULLDIST_END)
			{
				toml_str += ", nocull_dist = " + format_float(a.nocull_distance);
			}

			// }
			toml_str += " },\n";

		} // end loop
		toml_str += "    ]";

		return toml_str;
	}

	/// Builds a toml string for the provided light definition
	/// @param def	ref to the light def
	/// @return		the final string in toml format
	std::string map_settings::build_light_string_for_single_light(const map_settings::remix_light_settings_s& def)
	{
		std::string toml_str = "# " + def.comment + "\n";

		toml_str += "        {";

		// choreo trigger
		if (!def.trigger_choreo_name.empty())
		{
			toml_str += " trigger = { choreo = \"" + def.trigger_choreo_name + "\"";

			if (!def.trigger_choreo_actor.empty()) {
				toml_str += ", actor = \"" + def.trigger_choreo_actor + "\"";
			}

			if (!def.trigger_choreo_event.empty()) {
				toml_str += ", event = \"" + def.trigger_choreo_event + "\"";
			}

			if (!def.trigger_choreo_param1.empty()) {
				toml_str += ", param1 = \"" + def.trigger_choreo_param1 + "\"";
			}

			if (!shared::utils::float_equal(def.trigger_delay, 0.0f)) {
				toml_str += ", delay = " + format_float(def.trigger_delay);
			}

			if (def.trigger_always) {
				toml_str += ", always = " + (def.trigger_always ? "true"s : "false"s);
			}

			toml_str += " },"; // end table
		}
		// sound trigger
		else if (def.trigger_sound_hash)
		{
			toml_str += " trigger = { sound = " + std::format("0x{:X}", def.trigger_sound_hash);

			if (!shared::utils::float_equal(def.trigger_delay, 0.0f)) {
				toml_str += ", delay = " + format_float(def.trigger_delay);
			}

			if (def.trigger_always) {
				toml_str += ", always = " + (def.trigger_always ? "true"s : "false"s);
			}

			toml_str += " },"; // end table
		}

		// kill
		if (const auto has_kill_choreo = !def.kill_choreo_name.empty();
			has_kill_choreo || def.kill_sound_hash != 0u)
		{
			if (has_kill_choreo) {
				toml_str += " kill = { choreo = \"" + def.kill_choreo_name + "\"";
			}
			else { // sound
				toml_str += " kill = { sound = " + std::format("0x{:X}", def.kill_sound_hash);
			}

			if (!shared::utils::float_equal(def.kill_delay, 0.0f)) {
				toml_str += ", delay = " + format_float(def.kill_delay);
			}

			toml_str += " },"; // end table
		}

		// attach
		if (const auto has_attach_radius = def.attach_prop_radius != 0.0f;
			has_attach_radius || !def.attach_prop_name.empty())
		{
			if (has_attach_radius) {
				toml_str += " attach = { radius = " + std::to_string(def.attach_prop_radius); //format_float(def.attach_prop_radius);
			}
			else { // name str
				toml_str += " attach = { name = \"" + def.attach_prop_name + "\"";
			}

			if (!(def.attach_prop_mins.IsZero() && def.attach_prop_maxs.IsZero()))
			{
				toml_str += ", bounds = ["
					+ format_float(def.attach_prop_mins.x) + ", " + format_float(def.attach_prop_mins.y) + ", " + format_float(def.attach_prop_mins.z) + ", "
					+ format_float(def.attach_prop_maxs.x) + ", " + format_float(def.attach_prop_maxs.y) + ", " + format_float(def.attach_prop_maxs.z) + "]";
			}

			toml_str += " },"; // end table
		}

		// points
		toml_str += " points = [\n";

		for (const auto& p : def.points)
		{
			toml_str += "            { ";

			toml_str += "position = [" + format_float(p.position.x) + ", " + format_float(p.position.y) + ", " + format_float(p.position.z) + "]";
			toml_str += ", radiance = [" + format_float(p.radiance.x) + ", " + format_float(p.radiance.y) + ", " + format_float(p.radiance.z) + "]";
			toml_str += ", scalar = " + format_float(p.radiance_scalar);
			toml_str += ", radius = " + format_float(p.radius);
			toml_str += ", smoothness = " + format_float(p.smoothness);

			// only write shaping if deg != 180
			if (!shared::utils::float_equal(p.degrees, 180.0f))
			{
				toml_str += ", direction = [" + format_float(p.direction.x) + ", " + format_float(p.direction.y) + ", " + format_float(p.direction.z) + "]";
				toml_str += ", degrees = " + format_float(p.degrees);
				toml_str += ", softness = " + format_float(p.softness);
				toml_str += ", exponent = " + format_float(p.exponent);
			}

			toml_str += ", volumetric_scale = " + format_float(p.volumetric_scale);

			// ignore t0
			if (!shared::utils::float_equal(p.timepoint, 0.0f)) {
				toml_str += ", timepoint = " + format_float(p.timepoint);
			}

			toml_str += " },\n";
		}

		// end points
		toml_str += "        ]";

		if (def.run_once) {
			toml_str += ", run_once = " + (def.run_once ? "true"s : "false"s);
		}

		if (def.loop) {
			toml_str += ", loop = " + (def.loop ? "true"s : "false"s);
		}

		if (def.loop_smoothing) {
			toml_str += ", loop_smoothing = " + (def.loop_smoothing ? "true"s : "false"s);
		}

		toml_str += " },";
		return toml_str;
	}


	void map_settings::set_settings_for_map(const std::string& map_name)
	{
		m_map_settings.mapname = !map_name.empty() ? map_name : interfaces::get()->m_engine->get_level_name();
		shared::utils::replace_all(m_map_settings.mapname, std::string("maps/"), "");		// if sp map
		shared::utils::replace_all(m_map_settings.mapname, std::string(".bsp"), "");

		parse_toml();

		static bool disable_map_configs = shared::common::flags::has_flag("xo_disable_map_conf");
		if (shared::common::remix_api::is_initialized())
		{
			if (!disable_map_configs)
			{
				// resets all modified variables back to rtx.conf level
				shared::common::remix_vars::reset_all_modified();

				// auto apply {map_name}.conf (if it exists)
				open_and_set_var_config(m_map_settings.mapname + ".conf", true);

				// apply other manually defined configs
				for (const auto& f : m_map_settings.api_var_configs) {
					open_and_set_var_config(f);
				}
			}

			cross_handle_map_and_game_settings();

			// lights are spawned manually in edit mode
			if (!imgui::get()->m_light_edit_mode) {
				remix_lights::get()->add_all_map_setting_lights_without_creation_trigger();
			}
		}

		m_map_settings.default_nocull_dist = game_settings::get()->default_nocull_distance.get_as<float>();

		// are we using any sound hashes to trigger light spawning?
		/*{
			if (!m_map_settings.remix_lights.empty())
			{
				for (const auto& l : m_map_settings.remix_lights)
				{
					if (l.trigger_sound_hash || l.kill_sound_hash)
					{
						m_map_settings.using_any_light_sound_hash = true;
						break;
					}
				}
			}
		}*/

		m_loaded = true;
	}

#define TOML_ERROR(TITLE, ENTRY, MSG, ...) \
	shared::common::console(); std::cout << toml::format_error(toml::make_error_info(#TITLE, (ENTRY), shared::utils::va(#MSG, __VA_ARGS__))) << std::endl; \

#define TOML_CATCH \
	catch (toml::type_error& err) { \
		shared::common::console(); std::cout << err.what() << std::endl; return; \
	}

	bool map_settings::parse_toml()
	{
		try
		{
			auto config = toml::parse("rtx_comp\\map_settings.toml", toml::spec::v(1, 1, 0));

			// #
			auto to_float = [](const toml::value& entry, const float default_val = 0.0f)
				{
					if (entry.is_floating()) {
						return static_cast<float>(entry.as_floating());
					}

					if (entry.is_integer()) {
						return static_cast<float>(entry.as_integer());
					}

					try { // this will fail and let the user know whats wrong
						return static_cast<float>(entry.as_floating());
					}
					catch (toml::type_error& err) {
						shared::common::console(); printf("%s\n", err.what());
					}

					return default_val;
				};

			// #
			auto to_int = [](const toml::value& entry, const int default_val = 0)
				{
					if (entry.is_floating()) {
						return static_cast<int>(entry.as_floating());
					}

					if (entry.is_integer()) {
						return static_cast<int>(entry.as_integer());
					}

					try { // this will fail and let the user know whats wrong
						return static_cast<int>(entry.as_integer());
					}
					catch (toml::type_error& err) {
						shared::common::console(); printf("%s\n", err.what());
					}

					return default_val;
				};

			auto to_uint = [](const toml::value& entry, const std::uint32_t default_val = 0u)
				{
					if (entry.is_floating()) {
						return static_cast<std::uint32_t>(entry.as_floating());
					}

					if (entry.is_integer()) {
						return static_cast<std::uint32_t>(entry.as_integer());
					}

					try { // this will fail and let the user know whats wrong
						return static_cast<std::uint32_t>(entry.as_integer());
					}
					catch (toml::type_error& err) {
						shared::common::console(); printf("%s\n", err.what());
					}

					return default_val;
				};

			// #
			auto to_bool = [](const toml::value& entry, const bool default_setting = false)
				{
					if (entry.is_boolean()) {
						return static_cast<bool>(entry.as_boolean());
					}

					if (entry.is_integer()) {
						return static_cast<bool>(entry.as_integer());
					}

					try { // this will fail and let the user know whats wrong
						return static_cast<bool>(entry.as_boolean());
					}
					catch (toml::type_error& err) {
						shared::common::console(); printf("%s\n", err.what());
					}

					return default_setting;
				};

			// ####################
			// parse 'WATER' table
			if (config.contains("WATER"))
			{
				auto& water_table = config["WATER"];

				// try to find the loaded map
				if (water_table.contains(m_map_settings.mapname))
				{
					if (const auto map = water_table[m_map_settings.mapname];
						!map.is_empty())
					{
						if (map.contains("scale")) {
							m_map_settings.water_uv_scale = to_float(map.at("scale"), 1.0f);
						}

						if (map.contains("scale_top")) {
							m_map_settings.water_uv_bottom_scale = to_float(map.at("scale_top"), 0.0f);
						}

						if (map.contains("top_layer_offset")) {
							m_map_settings.water_offset_top = to_float(map.at("top_layer_offset"), 0.5f);
						}

						if (map.contains("bottom_layer_offset")) {
							m_map_settings.water_offset_base = to_float(map.at("bottom_layer_offset"), 0.0f);
						}
					}
				}
			} // end 'WATER'


			// ####################
			// parse 'CULL' table
			if (config.contains("CULL"))
			{
				auto& cull_table = config["CULL"];

				// #
				auto process_cull_entry = [to_uint, to_float](const toml::value& entry)
					{
						if (entry.contains("in_area"))
						{
							const auto area = to_uint(entry.at("in_area"));

							// nocull dist for certain cull modes
							float temp_nocull_dist = game_settings::get()->default_nocull_distance.get_as<float>();
							if (entry.contains("nocull_dist")) {
								temp_nocull_dist = to_float(entry.at("nocull_dist"));
							}

							m_map_settings.area_settings.emplace(area,
								area_overrides_s
								{
									temp_nocull_dist,
									area
								});
						}
					};

				// try to find the loaded map
				if (cull_table.contains(m_map_settings.mapname))
				{
					if (const auto map = cull_table[m_map_settings.mapname];
						!map.is_empty() && !map.as_array().empty())
					{
						for (const auto& entry : map.as_array()) {
							process_cull_entry(entry);
						}
					}
				}
			} // end 'CULL'


			// ####################
			// parse 'HIDEMODEL' table
			if (config.contains("HIDEMODEL"))
			{
				// try to find the loaded map
				if (auto& hidemdl_table = config["HIDEMODEL"];
					hidemdl_table.contains(m_map_settings.mapname))
				{
					if (const auto map = hidemdl_table[m_map_settings.mapname];
						!map.is_empty())
					{
						if (map.contains("name"))
						{
							if (auto& names = map.at("name");
								!names.is_empty())
							{
								if (const auto& narray = map.at("name").as_array();
									!narray.empty())
								{
									for (auto& str : narray) {
										m_map_settings.hide_models.substrings.insert(str.as_string());
									}
								}
							}
						}

						if (map.contains("radius"))
						{
							if (auto& radii = map.at("radius");
								!radii.is_empty())
							{
								if (const auto& rarray = map.at("radius").as_array();
									!rarray.empty())
								{
									for (auto& r : rarray) {
										m_map_settings.hide_models.radii.insert(to_float(r, -1.0f));
									}
								}
							}
						}
					}
				}
			} // end 'HIDEMODEL'


			// ####################
			// parse 'UNBAKE' table
			if (config.contains("UNBAKE"))
			{
				auto& unbake_table = config["UNBAKE"];

				// try to find the loaded map
				if (unbake_table.contains(m_map_settings.mapname))
				{
					if (const auto map = unbake_table[m_map_settings.mapname];
						!map.is_empty())
					{
						if (map.contains("name"))
						{
							if (auto& names = map.at("name");
								!names.is_empty())
							{
								if (const auto& arr = names.as_array();
									!arr.empty())
								{
									for (auto& str : arr) {
										m_map_settings.unbake_models.strings.insert(str.as_string());
									}
								}
							}
						}

						if (map.contains("checksum"))
						{
							if (auto& checksum = map.at("checksum");
								!checksum.is_empty())
							{
								if (const auto& arr = checksum.as_array();
									!arr.empty())
								{
									for (auto& sum : arr) {
										m_map_settings.unbake_models.checksums.insert(to_int(sum, 0u));
									}
								}
							}
						}
					}
				}


				if (unbake_table.contains("ALL"))
				{
					if (auto& all = unbake_table.at("ALL");
						!all.is_empty())
					{
						if (all.contains("name"))
						{
							if (auto& names = all.at("name");
								!names.is_empty())
							{
								if (const auto& arr = names.as_array();
									!arr.empty())
								{
									for (auto& str : arr) {
										m_map_settings.unbake_models.strings.insert(str.as_string());
									}
								}
							}
						}

						if (all.contains("checksum"))
						{
							if (auto& checksum = all.at("checksum");
								!checksum.is_empty())
							{
								if (const auto& arr = checksum.as_array();
									!arr.empty())
								{
									for (auto& sum : arr) {
										m_map_settings.unbake_models.checksums.insert(to_uint(sum, 0u));
									}
								}
							}
						}
					}
				}
			} // end 'UNBAKE'

			// ####################
			// parse 'MARKER' table
			if (config.contains("MARKER"))
			{
				auto& marker_table = config["MARKER"];

				// #
				auto process_marker_entry = [to_int, to_float](const toml::value& entry)
					{
						bool temp_is_nocull_marker = false;
						std::uint32_t temp_marker_index = 0u;

						if (entry.contains("nocull"))
						{
							temp_marker_index = static_cast<std::uint32_t>(to_int(entry.at("nocull"), 0u));
							temp_is_nocull_marker = true;
						}
						else
						{
							TOML_ERROR("[MARKER] #index", entry, "Marker did not define an index via 'marker' or 'nocull' -> skipping");
							return;
						}

						std::string temp_comment;
						if (!entry.comments().empty())
						{
							temp_comment = entry.comments().at(0);
							temp_comment.erase(0, 2); // rem '# '
						}

						if (entry.contains("position"))
						{
							if (const auto& pos = entry.at("position").as_array();
								pos.size() == 3)
							{
								Vector temp_rotation;
								Vector temp_scale = { 1.0, 1.0f, 1.0f };

								// optional
								if (entry.contains("rotation"))
								{
									if (const auto& rot = entry.at("rotation").as_array(); rot.size() == 3) {
										temp_rotation = { DEG2RAD(to_float(rot[0])), DEG2RAD(to_float(rot[1])), DEG2RAD(to_float(rot[2])) };
									}
									else { TOML_ERROR("[MARKER] #rotation", entry.at("rotation"), "expected a 3D vector but got => %d ", entry.at("rotation").as_array().size()); }
								}

								// optional
								if (entry.contains("scale"))
								{
									if (const auto& scale = entry.at("scale").as_array(); scale.size() == 3) {
										temp_scale = { to_float(scale[0]), to_float(scale[1]), to_float(scale[2]) };
									}
									else { TOML_ERROR("[MARKER] #scale", entry.at("scale"), "expected a 3D vector but got => %d ", entry.at("scale").as_array().size()); }
								}

								// optional
								std::unordered_set<std::uint32_t> temp_area_set;
								if (entry.contains("areas"))
								{
									if (const auto& areas = entry.at("areas").as_array(); !areas.empty())
									{
										for (const auto& a : areas) {
											temp_area_set.insert(to_int(a));
										}
									}
								}

								m_map_settings.map_markers.emplace_back(
									marker_settings_s
									{
										.index = temp_marker_index,
										.origin = { to_float(pos[0]), to_float(pos[1]), to_float(pos[2]) },
										.rotation = temp_rotation,
										.scale = temp_scale,
										.areas = std::move(temp_area_set),
										.comment = std::move(temp_comment)
									});
							}
							else { TOML_ERROR("[MARKER] #position", entry.at("position"), "expected a 3D vector but got => %d ", entry.at("position").as_array().size()); }
						}
					};

				// try to find the loaded map
				if (marker_table.contains(m_map_settings.mapname))
				{
					if (const auto map = marker_table[m_map_settings.mapname];
						!map.is_empty() && !map.as_array().empty())
					{
						for (const auto& entry : map.as_array()) {
							process_marker_entry(entry);
						}
					}
				}
			} // end 'MARKER'


			// ####################
			// parse 'CONFIGVARS' table
			{
				auto& configvar_table = config["CONFIGVARS"];

				// try to find the loaded map
				if (configvar_table.contains(m_map_settings.mapname))
				{
					if (const auto map = configvar_table[m_map_settings.mapname];
						!map.is_empty())
					{
						if (map.contains("startup"))
						{
							if (auto& startup = map.at("startup").as_array();
								!startup.empty())
							{
								for (const auto& conf : startup)
								{
									try {
										m_map_settings.api_var_configs.emplace_back(conf.as_string());
									}
									catch (toml::type_error& err) {
										shared::common::console(); printf("%s\n", err.what());
									}
								}
							}
						}
					}
				}
			} // end 'CONFIGVARS'

			// ####################
			// parse 'LIGHTS' table
			if (config.contains("LIGHTS"))
			{
				auto& light_table = config["LIGHTS"];

				// #
				auto process_light_entry = [to_bool, to_int, to_uint, to_float](const toml::value& entry)
					{
						if (entry.contains("points") && !entry.at("points").as_array().empty())
						{
							// - parse trigger

							std::string temp_trigger_choreo_name;
							std::string temp_trigger_choreo_actor;
							std::string temp_trigger_choreo_event;
							std::string temp_trigger_choreo_param1;

							std::uint32_t temp_trigger_sound = 0u;
							float temp_trigger_delay = 0.0f;
							bool temp_trigger_always = false;

							std::string temp_comment;
							if (!entry.comments().empty())
							{
								temp_comment = entry.comments().at(0);
								temp_comment.erase(0, 2); // rem '# '
							}

							if (entry.contains("trigger"))
							{
								bool has_valid_trigger = false;
								const auto& trigger = entry.at("trigger");

								// choreo trigger
								if (trigger.contains("choreo"))
								{
									try { temp_trigger_choreo_name = trigger.at("choreo").as_string(); }
									CATCH_ERR;

									if (trigger.contains("actor"))
									{
										try { temp_trigger_choreo_actor = trigger.at("actor").as_string(); }
										CATCH_ERR;
									}

									if (trigger.contains("event"))
									{
										try { temp_trigger_choreo_event = trigger.at("event").as_string(); }
										CATCH_ERR;
									}

									if (trigger.contains("param1"))
									{
										try { temp_trigger_choreo_param1 = trigger.at("param1").as_string(); }
										CATCH_ERR;
									}

									has_valid_trigger = true;
								}
								// sound trigger
								else if (trigger.contains("sound"))
								{
									temp_trigger_sound = to_uint(trigger.at("sound"), 0u);
									has_valid_trigger = true;
								}

								if (has_valid_trigger)
								{
									if (trigger.contains("delay")) {
										temp_trigger_delay = to_float(trigger.at("delay"), 0.0f);
									}

									if (trigger.contains("always")) {
										temp_trigger_always = to_bool(trigger.at("always"), false);
									}
								}
								else { TOML_ERROR("[LIGHTS] #trigger", trigger, "defined trigger with no choreo / sound hash"); }
							}

							// - parse kill

							std::string temp_kill_choreo_name;
							std::uint32_t temp_kill_sound = 0u;
							float temp_kill_delay = 0.0f;

							if (entry.contains("kill"))
							{
								bool has_valid_kill_trigger = false;
								const auto& kill = entry.at("kill");

								// choreo
								if (kill.contains("choreo"))
								{
									try { temp_kill_choreo_name = kill.at("choreo").as_string(); }
									CATCH_ERR;

									has_valid_kill_trigger = true;
								}
								// sound
								else if (kill.contains("sound"))
								{
									temp_kill_sound = to_uint(kill.at("sound"), 0u);
									has_valid_kill_trigger = true;
								}

								if (has_valid_kill_trigger)
								{
									if (kill.contains("delay")) {
										temp_kill_delay = to_float(kill.at("delay"), 0.0f);
									}
								}
								else { TOML_ERROR("[LIGHTS] #trigger", kill, "defined kill trigger with no choreo / sound hash"); }
							}

							// - parse points

							const auto& parray = entry.at("points").as_array();
							std::vector<remix_light_settings_s::point_s> temp_points;

							// for each point
							for (auto i = 0u; i < parray.size(); i++)
							{
								bool point_has_valid_position = false;

								const auto& p = parray[i];
								if (p.contains("position"))
								{
									if (const auto& positions = p.at("position").as_array(); positions.size() == 3) {
										point_has_valid_position = true;
									}
									else { TOML_ERROR("[LIGHTS] #position", p.at("position"), "expected a 3D vector but got => %d ", p.at("position").as_array().size()); }
								}

								if (!i && !point_has_valid_position) // first point needs to define a position
								{
									TOML_ERROR("[LIGHTS] #position", p, "first point needs to define a position! Ignoring light");
									break;
								}

								Vector temp_radiance = { 10.0f, 10.0f, 10.0f };
								if (p.contains("radiance"))
								{
									if (const auto& radiance = p.at("radiance").as_array(); radiance.size() == 3)
									{
										temp_radiance = Vector(to_float(radiance[0], 10.0f), to_float(radiance[1], 10.0f), to_float(radiance[2], 10.0f));
									}
									else { TOML_ERROR("[LIGHTS] #radiance", p.at("radiance"), "expected a 3D vector but got => %d ", p.at("radiance").as_array().size()); }
								}

								float temp_radiance_scalar = 1.0f;
								if (p.contains("scalar")) {
									temp_radiance_scalar = to_float(p.at("scalar"), 1.0f);
								}

								float temp_radius = 1.0f;
								if (p.contains("radius")) {
									temp_radius = to_float(p.at("radius"), 1.0f);
								}

								float temp_timepoint = 0.0f;
								if (i && p.contains("timepoint")) { // do not set timepoint for first point
									temp_timepoint = to_float(p.at("timepoint"), 0.0f);
								}

								float temp_smoothness = 0.5f;
								if (p.contains("smoothness"))
								{
									temp_smoothness = to_float(p.at("smoothness"), 0.5f);
									temp_smoothness = std::clamp<float>(temp_smoothness, 0.0f, 10.0f);
								}


								// shaping

								Vector temp_direction = { 0.0f, 0.0f, 1.0f };
								if (p.contains("direction"))
								{
									if (const auto& direction = p.at("direction").as_array(); direction.size() == 3)
									{
										temp_direction = Vector(to_float(direction[0], 0.0f), to_float(direction[1], 0.0f), to_float(direction[2], 1.0f));
										temp_direction.Normalize();
									}
									else { TOML_ERROR("[LIGHTS] #direction", p.at("direction"), "expected a 3D vector but got => %d ", p.at("direction").as_array().size()); }
								}

								bool temp_shaping_enabled = false;
								float temp_degrees = 180.0f;
								if (p.contains("degrees"))
								{
									temp_degrees = to_float(p.at("degrees"), 180.0f);
									temp_degrees = std::clamp<float>(temp_degrees, 0.0f, 180.0f);
									temp_shaping_enabled = temp_degrees != 180.0f;
								}

								float temp_softness = 0.0f;
								if (p.contains("softness"))
								{
									temp_softness = to_float(p.at("softness"), 0.0f);
									temp_softness = std::clamp<float>(temp_softness, 0.0f, M_PI);
								}

								float temp_exponent = 0.0f;
								if (p.contains("exponent")) {
									temp_exponent = to_float(p.at("exponent"), 0.0f);
								}

								// volumetrics
								float temp_volumetric = 0.0f;
								if (p.contains("volumetric_scale")) { // volumetricRadianceScale
									temp_volumetric = to_float(p.at("volumetric_scale"), 1.0f);
								}

								// to avoid code duplication
								Vector pt;

								// using either position defined in current point or previous position
								if (point_has_valid_position)
								{
									const auto& positions = p.at("position").as_array();
									pt = Vector(to_float(positions[0]), to_float(positions[1]), to_float(positions[2]));
								}
								else {
									pt = temp_points.back().position; // pos of previous point
								}

								temp_points.emplace_back(
									remix_light_settings_s::point_s(
										pt,
										temp_radiance,
										temp_radiance_scalar,
										temp_radius,
										temp_timepoint,
										temp_smoothness,
										temp_shaping_enabled,
										temp_direction,
										temp_degrees,
										temp_softness,
										temp_exponent,
										temp_volumetric)
								);
							}

							// attach settings

							float temp_attach_prop_radius = 0.0f;
							std::string temp_attach_prop_str;
							Vector temp_attach_prop_bounds_min;
							Vector temp_attach_prop_bounds_max;

							if (entry.contains("attach"))
							{
								bool has_valid_attach = false;
								const auto& attach = entry.at("attach");

								if (attach.contains("radius"))
								{
									temp_attach_prop_radius = to_float(attach.at("radius"), 0.0f);
									has_valid_attach = true;
								}
								else if (attach.contains("name"))
								{
									try { temp_attach_prop_str = attach.at("name").as_string(); }
									CATCH_ERR;

									has_valid_attach = true;
								}

								if (has_valid_attach)
								{
									m_map_settings.using_any_light_attached_to_prop = true;

									if (attach.contains("bounds"))
									{
										if (const auto& bounds = attach.at("bounds").as_array();
											bounds.size() == 6u)
										{
											temp_attach_prop_bounds_min = Vector(to_float(bounds[0]), to_float(bounds[1]), to_float(bounds[2]));
											temp_attach_prop_bounds_max = Vector(to_float(bounds[3]), to_float(bounds[4]), to_float(bounds[5]));
										}
									}
								}
							}

							// - parse general settings

							if (!temp_points.empty())
							{
								bool temp_run_once = false;
								if (entry.contains("run_once")) {
									temp_run_once = to_bool(entry.at("run_once"), false);
								}

								bool temp_loop = false;
								if (entry.contains("loop")) {
									temp_loop = to_bool(entry.at("loop"), false);
								}

								bool temp_loop_smoothing = false;
								if (entry.contains("loop_smoothing")) {
									temp_loop_smoothing = to_bool(entry.at("loop_smoothing"), false);
								}

								m_map_settings.remix_lights.push_back(
									remix_light_settings_s(
										std::move(temp_points),
										temp_run_once,
										temp_loop,
										temp_loop_smoothing,
										temp_trigger_always,
										std::move(temp_trigger_choreo_name),
										std::move(temp_trigger_choreo_actor),
										std::move(temp_trigger_choreo_event),
										std::move(temp_trigger_choreo_param1),
										temp_trigger_sound,
										temp_trigger_delay,
										std::move(temp_kill_choreo_name),
										temp_kill_sound,
										temp_kill_delay,
										temp_attach_prop_radius,
										std::move(temp_attach_prop_str),
										temp_attach_prop_bounds_min,
										temp_attach_prop_bounds_max,
										std::move(temp_comment))
								);
							}
						}
						else { TOML_ERROR("[LIGHTS] #points", entry, "needs at least one point to define a light"); }
					};

				// try to find the loaded map
				if (light_table.contains(m_map_settings.mapname))
				{
					if (const auto& map = light_table[m_map_settings.mapname];
						!map.is_empty() && !map.as_array().empty())
					{
						for (const auto& entry : map.as_array()) {
							process_light_entry(entry);
						}
					}
				}
			} // end 'LIGHTS'

			// ####################
			// parse 'CVARS' table
			if (config.contains("CVARS"))
			{
				auto& cvar_table = config["CVARS"];

				// try to find the loaded map
				if (cvar_table.contains(m_map_settings.mapname))
				{
					if (const auto map = cvar_table[m_map_settings.mapname];
						!map.is_empty() && map.is_array())
					{
						const auto& vars = map.as_array();
						for (auto& str : vars) {
							interfaces::get()->m_engine->execute_client_cmd_unrestricted(str.as_string().c_str());
						}
					}
				}
			} // end 'CVARS'
		}

		catch (const toml::syntax_error& err)
		{
			shared::common::console();
			printf("%s\n", err.what());
			return false;
		}

		return true;
	}

	bool map_settings::matches_map_name()
	{
		return shared::utils::str_to_lower(m_args[0]) == m_map_settings.mapname;
	}

	void map_settings::open_and_set_var_config(const std::string& config, const bool no_error, const bool ignore_hashes, const char* custom_path)
	{
		std::string path = "rtx_comp\\map_configs";
		if (custom_path)
		{
			path = custom_path;
		}

		std::ifstream file;
		if (shared::utils::open_file_homepath(path, config, file))
		{
			std::string input;
			while (std::getline(file, input))
			{
				if (shared::utils::starts_with(input, "#")) {
					continue;
				}

				if (auto pair = shared::utils::split(input, '=');
					pair.size() == 2u)
				{
					shared::utils::trim(pair[0]);
					shared::utils::trim(pair[1]);

					if (ignore_hashes && pair[1].starts_with("0x")) {
						continue;
					}

					if (pair[1].empty()) {
						continue;
					}

					if (const auto o = shared::common::remix_vars::get_option(pair[0].c_str()); o)
					{
						const auto& v = shared::common::remix_vars::string_to_option_value(o->second.type, pair[1]);
						shared::common::remix_vars::set_option(o, v, true);
					}
				}
			}

			file.close();
		}
		else if (!no_error)
		{
			shared::common::console();
			printf("[MapSettings] Failed to find config: \"%s\" in %s \n", config.c_str(), custom_path ? custom_path : "\"" "rtx_comp\\map_configs\"");
		}
	}

	void map_settings::on_map_load(const std::string& map_name)
	{
		is_level.reset();
		is_level.update(map_name);

		if (m_loaded) {
			get()->clear_map_settings();
		}

		get()->set_settings_for_map(map_name);
	}

	void map_settings::on_map_unload()
	{
		{
			std::filesystem::create_directories(shared::globals::root_path + "\\rtx_comp\\logs\\");

			std::ofstream file;
			file.open((shared::globals::root_path + "\\rtx_comp\\logs\\autosave_mapsettings.toml").c_str());

			file << "# This file is autogenerated. It contains the the latest imgui map-setting changes.\n\n";

			file << "[CULL]\n";
			auto& areas = map_settings::get_map_settings().area_settings;
			file << "    " << build_culling_overrides_string_for_current_map(areas) << "\n\n";

			file << "[MARKER]\n";
			auto& markers = map_settings::get_map_settings().map_markers;
			file << "    " << build_map_marker_string_for_current_map(markers) << "\n\n";

			file << "[LIGHTS]\n";
			const auto lights = remix_lights::get();

			if (const auto edit_light = lights->get_first_active_light(); edit_light)
			{
				auto temp_def = edit_light->def;
				if (edit_light->mover.is_initialized()) {
					temp_def.points = edit_light->mover.get_points_vec();
				}

				file << "    " << build_light_string_for_single_light(temp_def) << "\n\n";
			}

			file.close();
		}

		get()->clear_map_settings();
	}

	void map_settings::clear_map_settings()
	{
		m_map_settings.area_settings.clear();
		m_map_settings.hide_models.substrings.clear();
		m_map_settings.hide_models.radii.clear();
		m_map_settings.unbake_models.strings.clear();
		m_map_settings.unbake_models.checksums.clear();
		m_map_settings.api_var_configs.clear();

		remix_lights::get()->destroy_and_clear_all_active_lights();
		m_map_settings.remix_lights.clear();
		//m_map_settings.using_any_light_sound_hash = false;

		m_map_settings = {};
		m_loaded = false;

		trigger_vis_logic();
	}

	game::ConCommand xo_mapsettings_update {};
	void map_settings::reload()
	{
		clear_map_settings();
		map_settings::get()->set_settings_for_map("");
		imgui::get()->m_light_edit_mode = false;
	}

	map_settings::map_settings()
	{
		p_this = this;
		game::con_add_command(&xo_mapsettings_update, "xo_mapsettings_update", map_settings::reload, "Reloads the map_settings.toml file + map.conf");
	}

	map_settings::~map_settings()
	{ }

#undef CATCH_ERR
}
