project "LunaSVG"
	kind "StaticLib"
	language "C++"

	targetdir ("bin/%{prj.name}")
	objdir ("bin-int/%{prj.name}")

	files
	{
		"source/**.cpp",
		"source/**.h",
		"3rdparty/plutovg/**.c",
		"3rdparty/plutovg/**.h",
		"include/**.h"
	}

	includedirs{"src","include","3rdparty/plutovg"}

	filter "system:windows"
		systemversion "latest"
		cppdialect "C++17"

	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++17"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		optimize "off"
		staticruntime "On"
      	buildoptions { "/MP" }

	filter "configurations:Release"
		runtime "Release"
		optimize "On"
		staticruntime "On"
      	buildoptions { "/MP" }

    filter "configurations:Dist"
		runtime "Release"
		optimize "on"
        symbols "off"
		staticruntime "On"
        buildoptions { "/MP" }