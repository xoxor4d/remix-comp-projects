#include "std_include.hpp"
#include <psapi.h>

namespace shared::utils
{
	namespace mem
	{
		void** virtual_table(mem::addr_t inst) {
			return inst.read<void**>();
		}

		DWORD find_pattern_in_module(const HMODULE module, const std::string_view& signature, DWORD offset)
		{
			if (!module) {
				throw std::runtime_error("No or invalid module specified");
			}

			DWORD size = 0u;
			uint8_t* base = nullptr;

			{
				const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
				if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
					throw std::runtime_error("Invalid IMAGE_DOS_SIGNATURE");
				}

				const auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<BYTE*>(module) + dos_header->e_lfanew);

				if (nt_headers->Signature != IMAGE_NT_SIGNATURE) {
					throw std::runtime_error("Invalid IMAGE_NT_SIGNATURE");
				}

				size = nt_headers->OptionalHeader.SizeOfImage;
				base = reinterpret_cast<uint8_t*>(module);
			}

			// parse signature into bytes and mask
			std::vector<uint8_t> pattern_bytes;
			std::vector<bool> mask; // true = wildcard, false = match byte

			for (size_t i = 0; i < signature.length();)
			{
				if (signature[i] == ' ')
				{
					++i;
					continue;
				}

				if (signature[i] == '?')
				{
					pattern_bytes.push_back(0); // dummy value for wildcard
					mask.push_back(true);
					++i;

					if (i < signature.length() && signature[i] == '?') { // handle ??
						++i;
					}
				}
				else
				{
					// two hex digits as a byte
					const char hex[3] = { signature[i], i + 1 < signature.length() ? signature[i + 1] : '0', 0 };
					pattern_bytes.push_back(static_cast<uint8_t>(std::strtol(hex, nullptr, 16)));
					mask.push_back(false);
					i += 2;
				}
			}

			const size_t pattern_length = pattern_bytes.size();
			if (pattern_length == 0 || pattern_length > size) {
				return 0;
			}

			// scan memory
			for (size_t i = 0; i <= size - pattern_length; ++i)
			{
				bool found = true;
				for (size_t j = 0; j < pattern_length; ++j)
				{
					if (!mask[j] && base[i + j] != pattern_bytes[j])
					{
						found = false;
						break;
					}
				}

				if (found) {
					return reinterpret_cast<DWORD>(base + i + offset);
				}
			}

			return 0;
		}
	}

	hook::~hook()
	{
		if (this->initialized) {
			this->uninstall();
		}
	}

	hook* hook::initialize(DWORD _place, void(*_stub)(), bool _useJump)
	{
		return this->initialize(_place, reinterpret_cast<void*>(_stub), _useJump);
	}

	hook* hook::initialize(DWORD _place, void* _stub, bool _useJump)
	{
		return this->initialize(reinterpret_cast<void*>(_place), _stub, _useJump);
	}

	hook* hook::initialize(void* _place, void* _stub, bool _useJump)
	{
		if (this->initialized) return this;
		this->initialized = true;

		this->useJump = _useJump;
		this->place = _place;
		this->stub = _stub;

		this->original = static_cast<char*>(this->place) + 5 + *reinterpret_cast<DWORD*>((static_cast<char*>(this->place) + 1));

		return this;
	}

	hook* hook::install(bool unprotect, bool keepUnportected)
	{
		std::lock_guard<std::mutex> _(this->stateMutex);

		if (!this->initialized || this->installed) {
			return this;
		}

		this->installed = true;

		if (unprotect) VirtualProtect(this->place, sizeof(this->buffer), PAGE_EXECUTE_READWRITE, &this->protection);
		std::memcpy(this->buffer, this->place, sizeof(this->buffer));

		char* code = static_cast<char*>(this->place);

		*code = static_cast<char>(this->useJump ? 0xE9 : 0xE8);

		*reinterpret_cast<size_t*>(code + 1) = reinterpret_cast<size_t>(this->stub) - (reinterpret_cast<size_t>(this->place) + 5);

		if (unprotect && !keepUnportected) VirtualProtect(this->place, sizeof(this->buffer), this->protection, &this->protection);

		FlushInstructionCache(GetCurrentProcess(), this->place, sizeof(this->buffer));

		return this;
	}

	hook* hook::quick()
	{
		if (hook::installed) {
			hook::installed = false;
		}

		return this;
	}

	DWORD hook::create_trampoline()
	{
		if (this->trampoline) {
			throw std::runtime_error("Already created a trampoline for this hook!");
		}

		// alloc memory for trampoline (original 5 bytes + jmp)
		this->trampoline = VirtualAlloc(nullptr, sizeof(this->buffer) + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!this->trampoline) {
			return 0;
		}

		// copy original 5 bytes
		std::memcpy(this->trampoline, this->buffer, sizeof(this->buffer));

		// append jmp to the address after the hook (place + 5)
		char* trampoline_code = static_cast<char*>(this->trampoline) + sizeof(this->buffer);
		*trampoline_code = static_cast<char>(0xE9); // JMP
		*reinterpret_cast<size_t*>(trampoline_code + 1) = reinterpret_cast<size_t>(this->place) + 5 - (reinterpret_cast<size_t>(trampoline_code) + 5);

		FlushInstructionCache(GetCurrentProcess(), this->trampoline, sizeof(this->buffer) + 5);
		return reinterpret_cast<DWORD>(this->trampoline);
	}

	hook* hook::uninstall(bool unprotect)
	{
		std::lock_guard<std::mutex> _(this->stateMutex);

		if (!this->initialized || !this->installed) {
			return this;
		}

		this->installed = false;

		if (unprotect) {
			VirtualProtect(this->place, sizeof(this->buffer), PAGE_EXECUTE_READWRITE, &this->protection);
		}

		std::memcpy(this->place, this->buffer, sizeof(this->buffer));

		if (unprotect) {
			VirtualProtect(this->place, sizeof(this->buffer), this->protection, &this->protection);
		}

		{
			if (this->trampoline)
			{
				VirtualFree(this->trampoline, 0, MEM_RELEASE);
				this->trampoline = nullptr;
			}
		}

		FlushInstructionCache(GetCurrentProcess(), this->place, sizeof(this->buffer));
		return this;
	}

	void* hook::get_address()
	{
		return this->place;
	}

	void hook::nop(void* place, size_t length)
	{
		DWORD oldProtect;
		VirtualProtect(place, length, PAGE_EXECUTE_READWRITE, &oldProtect);

		memset(place, 0x90, length);

		VirtualProtect(place, length, oldProtect, &oldProtect);
		FlushInstructionCache(GetCurrentProcess(), place, length);
	}

	void hook::nop(DWORD place, size_t length)
	{
		nop(reinterpret_cast<void*>(place), length);
	}

	void hook::set_wstring(void* place, const wchar_t* string, size_t length)
	{
		DWORD oldProtect;
		VirtualProtect(place, (length + 1) * sizeof(wchar_t), PAGE_EXECUTE_READWRITE, &oldProtect);

		wcsncpy_s(static_cast<wchar_t*>(place), length + 1, string, length);

		VirtualProtect(place, (length + 1) * sizeof(wchar_t), oldProtect, &oldProtect);
	}

	void hook::set_wstring(DWORD place, const wchar_t* string, size_t length)
	{
		hook::set_wstring(reinterpret_cast<void*>(place), string, length);
	}

	void hook::set_wstring(void* place, const wchar_t* string)
	{
		hook::set_wstring(place, string, wcslen(string));
	}

	void hook::set_wstring(DWORD place, const wchar_t* string)
	{
		hook::set_wstring(reinterpret_cast<void*>(place), string);
	}

	void hook::set_string(void* place, const char* string, size_t length)
	{
		DWORD oldProtect;
		VirtualProtect(place, length + 1, PAGE_EXECUTE_READWRITE, &oldProtect);

		strncpy_s(static_cast<char*>(place), length, string, length);

		VirtualProtect(place, length + 1, oldProtect, &oldProtect);
	}

	void hook::set_string(DWORD place, const char* string, size_t length)
	{
		hook::set_string(reinterpret_cast<void*>(place), string, length);
	}

	void hook::set_string(void* place, const char* string)
	{
		hook::set_string(place, string, strlen(static_cast<char*>(place)));
	}

	void hook::set_string(DWORD place, const char* string)
	{
		hook::set_string(reinterpret_cast<void*>(place), string);
	}

	void hook::write_string(void* place, const std::string& string)
	{
		DWORD old_protect;
		VirtualProtect(place, string.size() + 1, PAGE_EXECUTE_READWRITE, &old_protect);

		memcpy(place, &string[0], string.size() + 1);

		VirtualProtect(place, string.size() + 1, old_protect, &old_protect);
		FlushInstructionCache(GetCurrentProcess(), place, string.size());
	}

	void hook::write_string(const DWORD place, const std::string& string)
	{
		write_string(reinterpret_cast<void*>(place), string);
	}

	void hook::redirect_jump(void* place, void* stub)
	{
		char* operandPtr = static_cast<char*>(place) + 2;
		int newOperand = reinterpret_cast<int>(stub) - (reinterpret_cast<int>(place) + 6);
		utils::hook::set<int>(operandPtr, newOperand);
	}

	void hook::redirect_jump(DWORD place, void* stub)
	{
		hook::redirect_jump(reinterpret_cast<void*>(place), stub);
	}


	// #
	// #

	void* cinterface::get_interface(const HMODULE hmodule, const char* const sz_object)
	{
		if (const auto addr = GetProcAddress(hmodule, "CreateInterface"); addr)
		{
			if (const auto pfCreateInterface = reinterpret_cast<void* (*)(const char*, int*)>(addr); pfCreateInterface) {
				return pfCreateInterface(sz_object, nullptr);
			}
		}
		return nullptr;
	}
}
