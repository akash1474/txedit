-- Building
-- MsBuild XPlayer.sln /p:configuration=Release

workspace "TxEdit"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }


outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

includeDirs={}
includeDirs["glfw"]="packages/glfw/include"
includeDirs["SpdLog"]="packages/spdlog/include"
includeDirs["ImGui"]="packages/imgui"
includeDirs["Mini"]="packages/mINI/src/mini"
includeDirs["LunaSVG"]="packages/lunasvg/include"
includeDirs["UUID"]="packages/uuid_v4"
includeDirs["TreeSitter"]="packages/tree-sitter/lib/include"
includeDirs["TreeSitterCpp"]="packages/tree-sitter-cpp/src"

-- /MP -- Multithreaded build 
-- /MT -- Static Linking. Defines _MT 
-- /MD -- Dynamic Linking. Defines _MT and _DLL 
include "packages/glfw"
include "packages/imgui"
include "packages/lunasvg"
include "packages/tree-sitter"
include "packages/tree-sitter-cpp"

project "text_editor"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++17"
   targetdir "bin"
   objdir "bin/obj"
   pchheader "pch.h"
   pchsource "src/pch.cpp"

   links {
      "glfw","ImGui","opengl32","LunaSVG","userenv","Shell32","dwmapi","TreeSitter","TreeSitterCpp"
   }

   includedirs{
      "src",
      "%{includeDirs.glfw}",
      "%{includeDirs.ImGui}",
      "%{includeDirs.Mini}",
      "%{includeDirs.LunaSVG}",
      "%{includeDirs.SpdLog}",
      "%{includeDirs.UUID}",
      "%{includeDirs.TreeSitter}",
      "%{includeDirs.TreeSitterCpp}"
   }

   files { 
      "src/**.cpp"
   }


   filter "system:windows"
      systemversion "latest"

   filter "configurations:Debug"
      runtime "Debug"
      symbols "On"
      staticruntime "On"
      optimize "Off"
      characterset ("Unicode") -- Default
      buildoptions { "/MP","/DEBUG:FULL","/utf-8" } --"/utf-8" - tells compiler to interprete string literals as utf8
      defines {"GL_DEBUG"} 

   filter {"configurations:Release"}
      runtime "Release"
      optimize "On"
      symbols "Off"
      staticruntime "On"
      buildoptions { "/MP","/utf-8" }
      defines {"GL_DEBUG","_CRT_SECURE_NO_WARNINGS"}

   filter "configurations:Dist"
      kind "WindowedApp"
      runtime "Release"
      optimize "On"
      symbols "Off"
      staticruntime "On"
      buildoptions { "/MP","/utf-8"}
      linkoptions {"/ENTRY:mainCRTStartup"}
      files { 'setup.rc', '**.ico' }
      vpaths { ['./*'] = { '*.rc', '**.ico' }}
