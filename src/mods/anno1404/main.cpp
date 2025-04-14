#include "std_include.hpp"
#include <psapi.h>

BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM lParam)
{
	DWORD window_pid, target_pid = static_cast<DWORD>(lParam);
	GetWindowThreadProcessId(hwnd, &window_pid);

	if (window_pid == target_pid && IsWindowVisible(hwnd))
	{
		shared::globals::main_window = hwnd;
		return FALSE;
	}

	return TRUE;
}

DWORD WINAPI find_game_window_by_sha1([[maybe_unused]] LPVOID lpParam)
{
	std::uint32_t T = 0;

	char exe_path[MAX_PATH];
	GetModuleFileNameA(nullptr, exe_path, MAX_PATH); // Current process executable
	const std::string sha1 = shared::utils::hash_file_sha1(exe_path);

	if (sha1 == "e10684e2680c03c434e54da952f000aa419126cd")
	{
		while (!shared::globals::main_window)
		{
			EnumWindows(enum_windows_proc, static_cast<LPARAM>(GetCurrentProcessId()));
			if (!shared::globals::main_window) {
				Sleep(100); T += 100;
			}

			if (T >= 30000) 
			{
				Beep(300, 100); Sleep(100); Beep(200, 100);
				shared::common::console(); std::cout << "[!][INIT FAILED] Not loading Anno1404-RTX Compatibility Mod\n";
				return TRUE;
			}
		}
	}
	else
	{
		Beep(300, 100); Sleep(100); Beep(200, 100);
		shared::common::console(); std::cout << "[!][INIT FAILED] Unsupported Game or Version of the Game!\n";
		return TRUE;
	}

#ifdef DEBUG
	Beep(523, 100);
#endif

	SetWindowTextA(shared::globals::main_window, "Anno 1404 - RTX");
	mods::anno1404::main();
	return 0;
}

namespace
{
	using drawindexedprimitive_fn = long(__stdcall*)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT); drawindexedprimitive_fn drawindexedprimitive_original = {};
	long __stdcall drawindexedprimitive_hk(IDirect3DDevice9* device, D3DPRIMITIVETYPE type, INT base_vertex_index, UINT min_vertex_index, UINT num_vertices, UINT start_index, UINT prim_count)
	{

		IDirect3DVertexDeclaration9* vertex_decl = nullptr;
		device->GetVertexDeclaration(&vertex_decl);

		enum d3ddecltype : BYTE
		{
			D3DDECLTYPE_FLOAT1 = 0,		// 1D float expanded to (value, 0., 0., 1.)
			D3DDECLTYPE_FLOAT2 = 1,		// 2D float expanded to (value, value, 0., 1.)
			D3DDECLTYPE_FLOAT3 = 2,		// 3D float expanded to (value, value, value, 1.)
			D3DDECLTYPE_FLOAT4 = 3,		// 4D float
			D3DDECLTYPE_D3DCOLOR = 4,	// 4D packed unsigned bytes mapped to 0. to 1. range

			// Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
			D3DDECLTYPE_UBYTE4 = 5,		// 4D unsigned byte
			D3DDECLTYPE_SHORT2 = 6,		// 2D signed short expanded to (value, value, 0., 1.)
			D3DDECLTYPE_SHORT4 = 7,		// 4D signed short

			// The following types are valid only with vertex shaders >= 2.0
			D3DDECLTYPE_UBYTE4N = 8,	// Each of 4 bytes is normalized by dividing to 255.0
			D3DDECLTYPE_SHORT2N = 9,	// 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
			D3DDECLTYPE_SHORT4N = 10,	// 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
			D3DDECLTYPE_USHORT2N = 11,  // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
			D3DDECLTYPE_USHORT4N = 12,  // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
			D3DDECLTYPE_UDEC3 = 13,		// 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
			D3DDECLTYPE_DEC3N = 14,		// 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
			D3DDECLTYPE_FLOAT16_2 = 15,	// Two 16-bit floating point values, expanded to (value, value, 0, 1)
			D3DDECLTYPE_FLOAT16_4 = 16,	// Four 16-bit floating point values
			D3DDECLTYPE_UNUSED = 17,	// When the type field in a decl is unused.
		};

		enum d3ddecluse : BYTE
		{
			D3DDECLUSAGE_POSITION = 0,
			D3DDECLUSAGE_BLENDWEIGHT,   // 1
			D3DDECLUSAGE_BLENDINDICES,  // 2
			D3DDECLUSAGE_NORMAL,        // 3
			D3DDECLUSAGE_PSIZE,         // 4
			D3DDECLUSAGE_TEXCOORD,      // 5
			D3DDECLUSAGE_TANGENT,       // 6
			D3DDECLUSAGE_BINORMAL,      // 7
			D3DDECLUSAGE_TESSFACTOR,    // 8
			D3DDECLUSAGE_POSITIONT,     // 9
			D3DDECLUSAGE_COLOR,         // 10
			D3DDECLUSAGE_FOG,           // 11
			D3DDECLUSAGE_DEPTH,         // 12
			D3DDECLUSAGE_SAMPLE,        // 13
		};

		struct d3dvertelem
		{
			WORD Stream;		// Stream index
			WORD Offset;		// Offset in the stream in bytes
			d3ddecltype Type;	// Data type
			BYTE Method;		// Processing method
			d3ddecluse Usage;	// Semantics
			BYTE UsageIndex;	// Semantic index
		};

		d3dvertelem decl[MAX_FVF_DECL_SIZE]; UINT numElements = 0;
		vertex_decl->GetDeclaration((D3DVERTEXELEMENT9*)decl, &numElements);

		bool use_shader = false;
		DWORD fvf = D3DFVF_XYZ;

		if (decl[1].Stream == 0)
		{
			switch (decl[1].Usage)
			{
			case D3DDECLUSAGE_NORMAL:
				fvf |= D3DFVF_NORMAL; break;
			case D3DDECLUSAGE_COLOR:
				fvf |= D3DFVF_DIFFUSE; break;
			case D3DDECLUSAGE_TEXCOORD:
				fvf |= D3DFVF_TEX1; break;
			}
		}

		if (decl[2].Stream == 0)
		{
			switch (decl[2].Usage)
			{
			case D3DDECLUSAGE_POSITION:
				use_shader = true;  break;
			case D3DDECLUSAGE_NORMAL:
				fvf |= D3DFVF_NORMAL; break;
			case D3DDECLUSAGE_COLOR:
				fvf |= D3DFVF_DIFFUSE; break;
			case D3DDECLUSAGE_TEXCOORD:
				fvf |= D3DFVF_TEX1; break;
			}
		}

		if (decl[3].Stream == 0)
		{
			switch (decl[3].Usage)
			{
			case D3DDECLUSAGE_NORMAL:
				fvf |= D3DFVF_NORMAL; break;
			case D3DDECLUSAGE_COLOR:
				fvf |= D3DFVF_DIFFUSE; break;
			case D3DDECLUSAGE_TEXCOORD:
				fvf |= D3DFVF_TEX1; break;
			}
		}

		DWORD og_fvf = 0u;
		bool is_ui = false;

		IDirect3DVertexShader9* og_shader = nullptr;
		if (!use_shader)
		{
			device->GetFVF(&og_fvf);

			if (og_fvf != 0u) {
				is_ui = true;
			}
			else
			{
				device->GetVertexShader(&og_shader);
				device->SetFVF(fvf);
				//device->SetVertexShader(nullptr);
				device->SetTransform(D3DTS_WORLD, &shared::globals::IDENTITY);
			}


		}

		auto hr = drawindexedprimitive_original(device, type, base_vertex_index, min_vertex_index, num_vertices, start_index, prim_count);

		if (!use_shader && !is_ui)
		{
			device->SetFVF(og_fvf);
			device->SetVertexShader(og_shader);
		}

		return hr;
	}

	using present_fn = long(__stdcall*)(IDirect3DDevice9*, RECT*, RECT*, HWND, RGNDATA*); present_fn present_original = {};
	long __stdcall present_hk(IDirect3DDevice9* device, RECT* source_rect, RECT* dest_rect, HWND dest_window_override, RGNDATA* dirty_region)
	{
		return present_original(device, source_rect, dest_rect, dest_window_override, dirty_region);
	}

	using reset_fn = long(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*); reset_fn reset_original = {};
	long __stdcall reset_hk(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* present_parameters)
	{
		const auto result = reset_original(device, present_parameters);
		return result;
	}
}

void grab_device_hk(IDirect3DDevice9* dev)
{
	shared::globals::d3d_device = dev;

	auto get_virtual = [](void* _class, unsigned int index) {
		return static_cast<unsigned int>((*static_cast<int**>(_class))[index]);
		};

	MH_CreateHook(reinterpret_cast<void*>(get_virtual(dev, 17)), present_hk, reinterpret_cast<void**>(&present_original));
	MH_CreateHook(reinterpret_cast<void*>(get_virtual(dev, 16)), reset_hk, reinterpret_cast<void**>(&reset_original));
	MH_CreateHook(reinterpret_cast<void*>(get_virtual(dev, 82)), drawindexedprimitive_hk, reinterpret_cast<void**>(&drawindexedprimitive_original));
	MH_EnableHook(MH_ALL_HOOKS);
}

__declspec(naked) void grab_device_stub()
{
	static uint32_t retn_addr = 0x8EF76E;
	__asm
	{
		// mov eax, [edi] // eax = &device

		pushad;
		push	eax;
		call	grab_device_hk;
		add		esp, 4;
		popad;

		mov     edx, [eax];
		mov     edx, [edx + 0x1C];
		jmp		retn_addr;
	}
}

BOOL APIENTRY DllMain(HMODULE, const DWORD ul_reason_for_call, LPVOID)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) 
	{
		if (const auto MH_INIT_STATUS = MH_Initialize(); MH_INIT_STATUS != MH_STATUS::MH_OK)
		{
			shared::common::console(); std::cout << "[!][INIT FAILED] MinHook failed to initialize with code: " << MH_INIT_STATUS << std::endl;
			return TRUE;
		}

		if (const auto t = CreateThread(nullptr, 0, find_game_window_by_sha1, nullptr, 0, nullptr); t) {
			CloseHandle(t);
		}

		// very early hooks
		shared::utils::hook(0x8EF769, grab_device_stub, HOOK_JUMP).install()->quick();
	}

	return TRUE;
}
