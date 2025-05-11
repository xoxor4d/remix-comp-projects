dependencies = {
	basePath = "./deps"
}

function dependencies.load()
	dir = path.join(dependencies.basePath, "premake/*.lua")
	deps = os.matchfiles(dir)

	for i, dep in pairs(deps) do
		dep = dep:gsub(".lua", "")
		require(dep)
	end
end

function dependencies.imports()
	for i, proj in pairs(dependencies) do
		if type(i) == 'number' then
			proj.import()
		end
	end
end

function dependencies.projects()
	for i, proj in pairs(dependencies) do
		if type(i) == 'number' then
			proj.project()
		end
	end
end

dependencies.load()

workspace "remix-comp-proj"

	startproject "bioshock1-rtx"
	location "./build"
	objdir "%{wks.location}/obj"
	targetdir "%{wks.location}/bin/%{cfg.buildcfg}"
	
    configurations { 
        "Debug", 
        "Release"
    }

	platforms "Win32"
	architecture "x86"

	cppdialect "C++20"
	systemversion "latest"
    symbols "On"
    staticruntime "On"

    disablewarnings {
		"4239",
		"4369",
		"4505",
		"4996",
		"6001",
		"6385",
		"6386",
		"26812"
	}

    defines { 
        "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS" 
    }

    filter "platforms:Win*"
		defines {
			"_WINDOWS", 
			"WIN32"
		}
	filter {}

	-- Release

	filter "configurations:Release"
		optimize "Full"

		buildoptions {
			"/GL"
		}

		defines {
			"NDEBUG"
		}
		
		flags { 
            "MultiProcessorCompile", 
            "LinkTimeOptimization", 
            "No64BitChecks",
			"FatalCompileWarnings"
        }
	filter {}

	-- Debug

	filter "configurations:Debug"
		optimize "Debug"

		defines { 
            "DEBUG", 
            "_DEBUG" 
        }

		flags { 
            "MultiProcessorCompile", 
            "No64BitChecks" 
        }
	filter {}

	-- Projects

	project "_shared"
		kind "StaticLib"
		language "C++"

		targetdir "bin/%{cfg.buildcfg}"
		objdir "obj/%{cfg.buildcfg}"
		
		pchheader "std_include.hpp"
		pchsource "src/shared/std_include.cpp"

		files {
			"./src/shared/**.hpp",
			"./src/shared/**.cpp",
		}

		includedirs {
			"%{prj.location}/src",
			"./src",
		}

		resincludedirs {
			"$(ProjectDir)src"
		}

        buildoptions { 
            "/Zm100 -Zm100" 
        }

        -- Specific configurations
		flags { 
			"UndefinedIdentifiers" 
		}

		warnings "Extra"
		dependencies.imports()

        group "Dependencies"
            dependencies.projects()
		group ""

	---------------------------

	project "anno1404-rtx"
		kind "SharedLib"
		language "C++"

		linkoptions {
			"/PDBCompress"
		}

		pchheader "std_include.hpp"
		pchsource "src/mods/anno1404/std_include.cpp"

		files {
			"./src/mods/anno1404/**.hpp",
			"./src/mods/anno1404/**.cpp",
		}

		includedirs {
			"%{prj.location}/src",
			"./src",
		}

		links {
			"_shared"
		}

		resincludedirs {
			"$(ProjectDir)src"
		}

        buildoptions { 
            "/Zm100 -Zm100" 
        }

		filter "configurations:Debug or configurations:Release"
			if(os.getenv("ANNO1404_ROOT")) then
				print ("Setup paths using environment variable 'ANNO1404_ROOT' :: '" .. os.getenv("ANNO1404_ROOT") .. "'")
				targetdir(os.getenv("ANNO1404_ROOT"))
				debugdir (os.getenv("ANNO1404_ROOT"))
				debugcommand (os.getenv("ANNO1404_ROOT") .. "/" .. "Anno4.exe")
			end
		filter {}

        -- Specific configurations
		flags { 
			"UndefinedIdentifiers" 
		}

		warnings "Extra"

		dependencies.imports()

        group "Dependencies"
            dependencies.projects()
		group ""
	
	---------------------------

	project "bioshock1-rtx"
	kind "SharedLib"
	language "C++"

	linkoptions {
		"/PDBCompress"
	}

	pchheader "std_include.hpp"
	pchsource "src/mods/bioshock1/std_include.cpp"

	files {
		"./src/mods/bioshock1/**.hpp",
		"./src/mods/bioshock1/**.cpp",
	}

	includedirs {
		"%{prj.location}/src",
		"./src",
	}

	links {
		"_shared"
	}

	resincludedirs {
		"$(ProjectDir)src"
	}

	buildoptions { 
		"/Zm100 -Zm100" 
	}

	filter "configurations:Debug or configurations:Release"
		if(os.getenv("BIOSHOCK1_ROOT")) then
			print ("Setup paths using environment variable 'BIOSHOCK1_ROOT' :: '" .. os.getenv("BIOSHOCK1_ROOT") .. "'")
			targetdir(os.getenv("BIOSHOCK1_ROOT"))
			debugdir (os.getenv("BIOSHOCK1_ROOT"))
			debugcommand (os.getenv("BIOSHOCK1_ROOT") .. "/" .. "Bioshock.exe")
			debugargs { "-dx9 -NOINTRO -windowed" }
		end
	filter {}

	-- Specific configurations
	flags { 
		"UndefinedIdentifiers" 
	}

	warnings "Extra"

	dependencies.imports()

	group "Dependencies"
		dependencies.projects()
	group ""


	---------------------------

	project "mirrorsedge-rtx"
	kind "SharedLib"
	language "C++"

	linkoptions {
		"/PDBCompress"
	}

	pchheader "std_include.hpp"
	pchsource "src/mods/mirrorsedge/std_include.cpp"

	files {
		"./src/mods/mirrorsedge/**.hpp",
		"./src/mods/mirrorsedge/**.cpp",
	}

	includedirs {
		"%{prj.location}/src",
		"./src",
	}

	links {
		"_shared"
	}

	resincludedirs {
		"$(ProjectDir)src"
	}

	buildoptions { 
		"/Zm100 -Zm100" 
	}

	filter "configurations:Debug or configurations:Release"
		if(os.getenv("MIRRORSEDGE_ROOT")) then
			print ("Setup paths using environment variable 'MIRRORSEDGE_ROOT' :: '" .. os.getenv("MIRRORSEDGE_ROOT") .. "'")
			targetdir(os.getenv("MIRRORSEDGE_ROOT"))
			debugdir (os.getenv("MIRRORSEDGE_ROOT"))
			debugcommand (os.getenv("MIRRORSEDGE_ROOT") .. "/" .. "MirrorsEdge.exe")
		end
	filter {}

	-- Specific configurations
	flags { 
		"UndefinedIdentifiers" 
	}

	warnings "Extra"

	dependencies.imports()

	group "Dependencies"
		dependencies.projects()
	group ""


	---------------------------

	project "fear1-rtx"
	kind "SharedLib"
	language "C++"

	linkoptions {
		"/PDBCompress"
	}

	pchheader "std_include.hpp"
	pchsource "src/mods/fear1/std_include.cpp"

	files {
		"./src/mods/fear1/**.hpp",
		"./src/mods/fear1/**.cpp",
	}

	includedirs {
		"%{prj.location}/src",
		"./src",
	}

	links {
		"_shared"
	}

	resincludedirs {
		"$(ProjectDir)src"
	}

	buildoptions { 
		"/Zm100 -Zm100" 
	}

	filter "configurations:Debug or configurations:Release"
		if(os.getenv("FEAR1_ROOT")) then
			print ("Setup paths using environment variable 'FEAR1_ROOT' :: '" .. os.getenv("FEAR1_ROOT") .. "'")
			targetdir(os.getenv("FEAR1_ROOT"))
			debugdir (os.getenv("FEAR1_ROOT"))
			debugcommand (os.getenv("FEAR1_ROOT") .. "/" .. "FEAR.exe")
		end
	filter {}

	-- Specific configurations
	flags { 
		"UndefinedIdentifiers" 
	}

	warnings "Extra"

	dependencies.imports()

	group "Dependencies"
		dependencies.projects()
	group ""


	---------------------------

	project "swat4-rtx"
	kind "SharedLib"
	language "C++"

	linkoptions {
		"/PDBCompress"
	}

	pchheader "std_include.hpp"
	pchsource "src/mods/swat4/std_include.cpp"

	files {
		"./src/mods/swat4/**.hpp",
		"./src/mods/swat4/**.cpp",
	}

	includedirs {
		"%{prj.location}/src",
		"./src",
	}

	links {
		"_shared"
	}

	resincludedirs {
		"$(ProjectDir)src"
	}

	buildoptions { 
		"/Zm100 -Zm100" 
	}

	filter "configurations:Debug or configurations:Release"
		if(os.getenv("SWAT4_ROOT")) then
			print ("Setup paths using environment variable 'SWAT4_ROOT' :: '" .. os.getenv("SWAT4_ROOT") .. "'")
			targetdir(os.getenv("SWAT4_ROOT"))
			debugdir (os.getenv("SWAT4_ROOT"))
			debugcommand (os.getenv("SWAT4_ROOT") .. "/" .. "Swat4.exe")
		end
	filter {}

	-- Specific configurations
	flags { 
		"UndefinedIdentifiers" 
	}

	warnings "Extra"

	-- Post-build
	postbuildcommands {
		"MOVE /Y \"$(TargetDir)swat4-rtx.dll\" \"$(TargetDir)swat4-rtx.asi\"",
	}

	dependencies.imports()

	group "Dependencies"
		dependencies.projects()
	group ""

	
	---------------------------

	project "killingfloor1-rtx"
	kind "SharedLib"
	language "C++"

	linkoptions {
		"/PDBCompress"
	}

	pchheader "std_include.hpp"
	pchsource "src/mods/killingfloor1/std_include.cpp"

	files {
		"./src/mods/killingfloor1/**.hpp",
		"./src/mods/killingfloor1/**.cpp",
	}

	includedirs {
		"%{prj.location}/src",
		"./src",
	}

	links {
		"_shared"
	}

	resincludedirs {
		"$(ProjectDir)src"
	}

	buildoptions { 
		"/Zm100 -Zm100" 
	}

	filter "configurations:Debug or configurations:Release"
		if(os.getenv("KILLINGFLOOR1_ROOT")) then
			print ("Setup paths using environment variable 'KILLINGFLOOR1_ROOT' :: '" .. os.getenv("KILLINGFLOOR1_ROOT") .. "'")
			targetdir(os.getenv("KILLINGFLOOR1_ROOT"))
			debugdir (os.getenv("KILLINGFLOOR1_ROOT"))
			debugcommand (os.getenv("KILLINGFLOOR1_ROOT") .. "/" .. "KillingFloor.exe")
		end
	filter {}

	-- Specific configurations
	flags { 
		"UndefinedIdentifiers" 
	}

	warnings "Extra"

	-- Post-build
	postbuildcommands {
		"MOVE /Y \"$(TargetDir)killingfloor1-rtx.dll\" \"$(TargetDir)killingfloor1-rtx.asi\"",
	}

	dependencies.imports()

	group "Dependencies"
		dependencies.projects()
	group ""

	---------------------------

	project "manhunt-rtx"
	kind "SharedLib"
	language "C++"

	linkoptions {
		"/PDBCompress"
	}

	pchheader "std_include.hpp"
	pchsource "src/mods/manhunt/std_include.cpp"

	files {
		"./src/mods/manhunt/**.hpp",
		"./src/mods/manhunt/**.cpp",
	}

	includedirs {
		"%{prj.location}/src",
		"./src",
	}

	links {
		"_shared"
	}

	resincludedirs {
		"$(ProjectDir)src"
	}

	buildoptions { 
		"/Zm100 -Zm100" 
	}

	filter "configurations:Debug or configurations:Release"
		if(os.getenv("MANHUNT1_ROOT")) then
			print ("Setup paths using environment variable 'MANHUNT1_ROOT' :: '" .. os.getenv("MANHUNT1_ROOT") .. "'")
			targetdir(os.getenv("MANHUNT1_ROOT"))
			debugdir (os.getenv("MANHUNT1_ROOT"))
			debugcommand (os.getenv("MANHUNT1_ROOT") .. "/" .. "manhunt.exe")
		end
	filter {}

	-- Specific configurations
	flags { 
		"UndefinedIdentifiers" 
	}

	warnings "Extra"

	-- Post-build
	postbuildcommands {
		"MOVE /Y \"$(TargetDir)manhunt-rtx.dll\" \"$(TargetDir)manhunt-rtx.asi\"",
	}

	dependencies.imports()

	group "Dependencies"
		dependencies.projects()
	group ""


	---------------------------

	project "blackhawkdown-rtx"
	kind "SharedLib"
	language "C++"

	linkoptions {
		"/PDBCompress"
	}

	pchheader "std_include.hpp"
	pchsource "src/mods/blackhawkdown/std_include.cpp"

	files {
		"./src/mods/blackhawkdown/**.hpp",
		"./src/mods/blackhawkdown/**.cpp",
	}

	includedirs {
		"%{prj.location}/src",
		"./src",
	}

	links {
		"_shared"
	}

	resincludedirs {
		"$(ProjectDir)src" 
	}

	buildoptions { 
		"/Zm100 -Zm100" 
	}

	-- Specific configurations
	flags { 
		"UndefinedIdentifiers" 
	}

	filter "configurations:Debug or configurations:Release"
		if(os.getenv("BLACKHAWKDOWN_ROOT")) then
			print ("Setup paths using environment variable 'BLACKHAWKDOWN_ROOT' :: '" .. os.getenv("BLACKHAWKDOWN_ROOT") .. "'")
			targetdir(os.getenv("BLACKHAWKDOWN_ROOT"))
			debugdir (os.getenv("BLACKHAWKDOWN_ROOT"))
			debugcommand (os.getenv("BLACKHAWKDOWN_ROOT") .. "/" .. "dfbhd.exe")
		end
	filter {}

	warnings "Extra"

	-- Post-build
	postbuildcommands {
		"MOVE /Y \"$(TargetDir)blackhawkdown-rtx.dll\" \"$(TargetDir)a_blackhawkdown-rtx.asi\"",
	}

	dependencies.imports()

	group "Dependencies"
		dependencies.projects()
	group ""

	---------------------------

	project "ue2fixes-rtx"
	kind "SharedLib"
	language "C++"

	linkoptions {
		"/PDBCompress"
	}

	pchheader "std_include.hpp"
	pchsource "src/mods/ue2fixes/std_include.cpp"

	files {
		"./src/mods/ue2fixes/**.hpp",
		"./src/mods/ue2fixes/**.cpp",
	}

	includedirs {
		"%{prj.location}/src",
		"./src",
	}

	links {
		"_shared"
	}

	resincludedirs {
		"$(ProjectDir)src"
	}

	buildoptions { 
		"/Zm100 -Zm100" 
	}

	-- Specific configurations
	flags { 
		"UndefinedIdentifiers" 
	}

	warnings "Extra"

	-- Post-build
	postbuildcommands {
		"MOVE /Y \"$(TargetDir)ue2fixes-rtx.dll\" \"$(TargetDir)a_ue2fixes-rtx.asi\"",
	}

	dependencies.imports()

	group "Dependencies"
		dependencies.projects()
	group ""