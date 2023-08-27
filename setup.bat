@echo off
set glfw=https://github.com/glfw/glfw.git
set lunasvg=https://github.com/sammycage/lunasvg.git
set mini=https://github.com/pulzed/mINI.git
set spdlog=-o spdlog.zip https://codeload.github.com/gabime/spdlog/zip/refs/tags/v1.12.0
set imgui=-o imgui.zip https://codeload.github.com/ocornut/imgui/zip/refs/tags/v1.89.7


echo -- Cloning glfw github repo
git clone --depth 1 %glfw%
echo.
echo -- Downloading spdlog
curl %spdlog%
echo.
echo -- Downloading imgui
curl %imgui%
echo.
echo -- Downloading lunasvg
git clone --depth 1 %lunasvg%
echo.
echo -- Downloading mini
git clone --depth 1 %mini% ./packages/mINI
echo.



if exist glfw (
	echo.
	echo Setting Up GLFW
	mv --force ./glfw/* ./packages/glfw
) else (
	echo [ GLFW ] No directory found
	exit /b
)

if exist lunasvg (
	echo.
	echo Setting Up LunaSVG
	mv --force ./lunasvg/* ./packages/lunasvg
) else (
	echo [ LunaSVG ] No directory found
	exit /b
)


echo.
echo -- Extracting Files
winrar x -idv spdlog.zip *
winrar x -idv imgui.zip *

if not exist .\packages\spdlog mkdir .\packages\spdlog
if not exist .\packages\imgui mkdir .\packages\imgui

echo -- Setting up spdlog
mv -f ./spdlog-1.12.0/* ./packages/spdlog

echo -- Setting up imgui
mv -f ./imgui-1.89.7/*.cpp ./packages/imgui
mv -f ./imgui-1.89.7/*.h ./packages/imgui
mv -f ./imgui-1.89.7/backends/imgui_impl_opengl2.cpp ./packages/imgui
mv -f ./imgui-1.89.7/backends/imgui_impl_opengl2.h ./packages/imgui
mv -f ./imgui-1.89.7/backends/imgui_impl_glfw.cpp ./packages/imgui
mv -f ./imgui-1.89.7/backends/imgui_impl_glfw.h ./packages/imgui

echo -- Cleaning
rmdir /s /q imgui-1.89.7
del /f imgui.zip
del /f spdlog.zip
rmdir /s /q spdlog-1.12.0
rmdir /s /q glfw
