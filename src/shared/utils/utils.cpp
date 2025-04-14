#include "std_include.hpp"

#define VA_BUFFER_COUNT		64
#define VA_BUFFER_SIZE		65536

namespace shared::utils
{
	float rad_to_deg(const float radians) {
		return radians * (180.0f / M_PI);
	}

	float deg_to_rad(const float degrees) {
		return degrees * M_PI / 180.0f;
	}

	int try_stoi(const std::string& str, const int& default_return_val)
	{
		int ret = default_return_val;

		try {
			ret = std::stoi(str);
		}
		catch (const std::invalid_argument) { }

		return ret;
	}

	float try_stof(const std::string& str, const float& default_return_val)
	{
		float ret = default_return_val;

		try {
			ret = std::stof(str);
		}
		catch (const std::invalid_argument) { }

		return ret;
	}

	std::string split_string_between_delims(const std::string& str, const char delim_start, const char delim_end)
	{
		const auto first = str.find_last_of(delim_start);
		if (first == std::string::npos) return "";

		const auto last = str.find_first_of(delim_end, first);
		if (last == std::string::npos) return "";

		return str.substr(first + 1, last - first - 1);
	}

	bool starts_with(std::string_view haystack, std::string_view needle)
	{
		return (haystack.size() >= needle.size() && !strncmp(needle.data(), haystack.data(), needle.size()));
	}

	bool string_contains(const std::string_view& s1, const std::string_view s2)
	{
		const auto it = s1.find(s2);

		if (it != std::string::npos) {
			return true;
		}

		return false;
	}

	void replace_all(std::string& source, const std::string_view& from, const std::string_view& to)
	{
		std::string new_string;
		new_string.reserve(source.length());  // avoids a few memory allocations

		std::string::size_type last_pos = 0;
		std::string::size_type find_pos;

		while (std::string::npos != (find_pos = source.find(from, last_pos)))
		{
			new_string.append(source, last_pos, find_pos - last_pos);
			new_string += to;
			last_pos = find_pos + from.length();
		}

		// Care for the rest after last occurrence
		new_string += source.substr(last_pos);

		source.swap(new_string);
	}

	bool erase_substring(std::string& base, const std::string& replace)
	{
		if (const auto it = base.find(replace);
			it != std::string::npos)
		{
			base.erase(it, replace.size());
			return true;
		}

		return false;
	}

	std::string str_to_lower(std::string input)
	{
		std::ranges::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return input;
	}

	std::string convert_wstring(const std::wstring& wstr)
	{
		std::string result;
		result.reserve(wstr.size());

		for (const auto& chr : wstr) {
			result.push_back(static_cast<char>(chr));
		}

		return result;
	}

	int is_space(int c)
	{
		if (c < -1) {
			return 0;
		}

		return _isspace_l(c, nullptr);
	}

	// trim from start
	std::string& ltrim(std::string& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int val)
			{
				return !is_space(val);
			}));

		return s;
	}

	// trim from end
	std::string& rtrim(std::string& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int val)
			{
				return !is_space(val);

			}).base(), s.end());

		return s;
	}

	// trim from both ends
	std::string& trim(std::string& s)
	{
		return ltrim(rtrim(s));
	}

	bool has_matching_symbols(const std::string& str, char opening_symbol, char closing_symbol, bool single_only)
	{
		int count = 0;

		for (char c : str)
		{
			if (c == opening_symbol) {
				count++;
			}
			else if (c == closing_symbol)
			{
				count--;

				if (count < 0) {
					return false;  // malformed
				}
			}

			if (single_only && count > 1) {
				return false;
			}
		}

		return count == 0;
	}

	const char* va(const char* fmt, ...)
	{
		static char g_vaBuffer[VA_BUFFER_COUNT][VA_BUFFER_SIZE];
		static int g_vaNextBufferIndex = 0;

		va_list ap;
		va_start(ap, fmt);
		char* dest = g_vaBuffer[g_vaNextBufferIndex];
		vsnprintf(g_vaBuffer[g_vaNextBufferIndex], VA_BUFFER_SIZE, fmt, ap);
		g_vaNextBufferIndex = (g_vaNextBufferIndex + 1) % VA_BUFFER_COUNT;
		va_end(ap);
		return dest;
	}

	void extract_integer_words(const std::string_view& str, std::vector<int>& integers, bool check_for_duplicates)
	{
		std::stringstream ss;

		//Storing the whole string into string stream
		ss << str;

		// Running loop till the end of the stream
		std::string temp;
		int found;

		while (!ss.eof())
		{
			// extracting word by word from stream 
			ss >> temp;

			// Checking the given word is integer or not
			if (std::stringstream(temp) >> found)
			{
				if (check_for_duplicates)
				{
					// check if we added the integer already
					if (std::find(integers.begin(), integers.end(), found) == integers.end())
					{
						// new integer
						integers.push_back(found);
					}
				}

				else
				{
					//cout << found << " ";
					integers.push_back(found);
				}
			}

			// To save from space at the end of string
			temp = "";
		}
	}

	void transpose_matrix3x4_to_d3dxmatrix(const shared::matrix3x4_t& src, D3DXMATRIX& dest)
	{
		dest.m[0][0] = src.m_flMatVal[0][0];
		dest.m[0][1] = src.m_flMatVal[1][0];
		dest.m[0][2] = src.m_flMatVal[2][0];
		dest.m[0][3] = 0.0f;

		dest.m[1][0] = src.m_flMatVal[0][1];
		dest.m[1][1] = src.m_flMatVal[1][1];
		dest.m[1][2] = src.m_flMatVal[2][1];
		dest.m[1][3] = 0.0f;

		dest.m[2][0] = src.m_flMatVal[0][2];
		dest.m[2][1] = src.m_flMatVal[1][2];
		dest.m[2][2] = src.m_flMatVal[2][2];
		dest.m[2][3] = 0.0f;

		dest.m[3][0] = src.m_flMatVal[0][3];
		dest.m[3][1] = src.m_flMatVal[1][3];
		dest.m[3][2] = src.m_flMatVal[2][3];
		dest.m[3][3] = 1.0f;
	}

	void transpose_float4x4(const float* row_major, float* column_major)
	{
		// transpose the matrix by swapping the rows and columns
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j) {
				column_major[j * 4 + i] = row_major[i * 4 + j];
			}
		}
	}

	bool float_equal(const float a, const float b, const float eps)
	{
		return std::fabs(a - b) < eps;
	}

	// https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Source/Runtime/Core/Private/Math/UnrealMath.cpp
	float finterp_to(const float current, const float target, const float delta_time, const float interpolation_speed)
	{
		// If no interp speed, jump to target value
		if (interpolation_speed <= 0.0f) {
			return target;
		}

		// distance to reach
		const float distance = target - current;

		// If distance is too small, just set the desired location
		if (distance * distance < 1.e-8f) {
			return target;
		}

		// Delta Move, Clamp so we do not over shoot.
		const float delta_move = distance * std::clamp(delta_time * interpolation_speed, 0.0f, 1.0f);

		return current + delta_move;
	}

	/**
	* @brief			open handle to a file within the home-path (root)
	* @param sub_dir	sub directory within home-path (root)
	* @param file_name	the file name
	* @param file		in-out file handle
	* @return			file handle state (valid or not)
	*/
	bool open_file_homepath(const std::string& root_path, const std::string& sub_dir, const std::string& file_name, std::ifstream& file)
	{
		file.open(root_path + sub_dir + "\\" + file_name);
		if (!file.is_open()) {
			return false;
		}

		return true;
	}

	//fnv1a
	std::uint64_t string_hash64(const std::string_view& str)
	{
		const uint64_t FNV_prime = 1099511628211u;
		const uint64_t offset_basis = 14695981039346656037u;
		uint64_t hash = offset_basis;

		for (const char c : str)
		{
			hash ^= static_cast<uint64_t>(c);
			hash *= FNV_prime;
		}

		return hash;
	}

	//fnv1a
	std::uint32_t string_hash32(const std::string_view& str)
	{
		const uint32_t FNV_prime = 16777619u;
		const uint32_t offset_basis = 2166136261u;
		uint32_t hash = offset_basis;

		for (const char c : str)
		{
			hash ^= static_cast<uint64_t>(c);
			hash *= FNV_prime;
		}

		return hash;
	}

	uint32_t hash32_combine(uint32_t seed, const char* str)
	{
		while (*str != '\0') 
		{
			seed ^= std::hash<char>{}(*str) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			++str;
		}
		return seed;
	}

	uint32_t hash32_combine(const uint32_t seed, const int val)
	{
		return seed ^ (std::hash<int>{}(val)+0x9e3779b9 + (seed << 6) + (seed >> 2));
	}

	uint32_t hash32_combine(const uint32_t seed, float val)
	{
		const uint32_t* ptr = reinterpret_cast<uint32_t*>(&val);
		return seed ^ (*ptr + 0x9e3779b9 + (seed << 6) + (seed >> 2));
	}

	std::string hash_file_sha1(const char* file_path)
	{
		const auto file = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) {
			return {};
		}

		HCRYPTPROV prov_handle = 0;
		HCRYPTHASH hash_handle = 0;

		BYTE buffer[4096];
		DWORD bytes_read = 0;

		BYTE hash[20]; // SHA-1 produces a 20-byte hash
		DWORD hash_len = sizeof(hash);

		if (!CryptAcquireContext(&prov_handle, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) ||
			!CryptCreateHash(prov_handle, CALG_SHA1, 0, 0, &hash_handle))
		{
			CloseHandle(file);
			return {};
		}

		while (ReadFile(file, buffer, sizeof(buffer), &bytes_read, nullptr) && bytes_read > 0)
		{
			if (!CryptHashData(hash_handle, buffer, bytes_read, 0))
			{
				CryptDestroyHash(hash_handle);
				CryptReleaseContext(prov_handle, 0);
				CloseHandle(file);
				return {};
			}
		}

		std::string hash_string;
		if (CryptGetHashParam(hash_handle, HP_HASHVAL, hash, &hash_len, 0))
		{
			std::ostringstream oss;
			for (DWORD i = 0; i < hash_len; ++i) {
				oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
			}

			hash_string = oss.str();
		}

		CryptDestroyHash(hash_handle);
		CryptReleaseContext(prov_handle, 0);
		CloseHandle(file);
		return hash_string;
	}
}
