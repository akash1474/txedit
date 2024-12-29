project "TreeSitter"
    kind "StaticLib"
    language "C"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "lib/src/lib.c",
    }

    includedirs {"lib/src","lib/src/wasm","lib/include" }

    filter "system:windows"
        systemversion "latest"

    filter "system:linux"
        pic "On"
        systemversion "latest"

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
