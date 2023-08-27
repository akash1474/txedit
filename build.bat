@echo off
set solution_name=""

for %%i in ("*.sln") do set solution_name=%%i

if /i [%1] == [run] goto :run

echo [94m[ Premake ][0m - [90mGenerating vs2022 files[0m
premake5 vs2022


where /q cl
if [%errorlevel%]==[1] (
	echo [ Environment Setup ]
	call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
	set errorlevel=0
)

set build_type=Debug
set pcolor=93

if /i [%1]==[] (
	goto :build	
) 


if /i [%1] == [release] (
	set pcolor=92
	set build_type=Release
) else if /i %1 == dist (
	set build_type=Dist
	set pcolor=92
) else if /i %1 == debug (
	set build_type=Debug
	set pcolor=93
) else if /i %1 == clean (
	set pcolor=92
	set build_type=%2
	goto :clean
) else (
	echo [91mInvalid Build Argument[0m
	goto :eof
)
goto :build


:build
	echo [%pcolor%m[ %build_type% Build ][0m
	MsBuild %solution_name% /p:configuration=%build_type%
	goto :run

:clean
	if [%build_type%] == [] set build_type=Debug
	echo [%pcolor%m[ Clean %build_type% Build ][0m
	MsBuild %solution_name% /p:Configuration=%build_type% /t:Rebuild
	goto :run

:run
if %errorlevel% == 1 (
	echo [91m[ Build Failed ][0m
	set /a errorlevel=0;
	goto :eof
) 
set exe_path=
for %%i in ("bin\*.exe") do set exe_path=%%i
if [%exe_path%]==[] (
	echo [91m[ Error ][0m --- [90mNo Executable Found![0m
    exit /b 1
)
echo [96m[ Running Executable ][0m
echo ----------------------------------------------------
%exe_path%
goto :eof
