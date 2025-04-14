#pragma once

#define TOML_ERROR(TITLE, ENTRY, MSG, ...) \
	game::console(); std::cout << toml::format_error(toml::make_error_info(#TITLE, (ENTRY), utils::va(#MSG, __VA_ARGS__))) << std::endl; \

namespace shared::common::toml
{
	
}