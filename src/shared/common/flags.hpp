#pragma once

namespace shared::common
{
	class flags
	{
	public:
		static bool has_flag(const std::string& flag);
		static void parse_flags();

	private:
		static std::vector<std::string> enabled_flags;
	};
}