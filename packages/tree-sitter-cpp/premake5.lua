project "TreeSitterCpp"
    kind "StaticLib"
    language "C"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/**.c",
    }

    includedirs { "src", "bindings" }

    filter "system:windows"
        systemversion "latest"
        cdialect "C99"

    filter "system:linux"
        pic "On"
        systemversion "latest"
        cdialect "C99"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        optimize "off"
        staticruntime "On"

    filter "configurations:Release"
        runtime "Release"
        optimize "On"
        staticruntime "On"
