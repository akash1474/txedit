#include "Log.h"
#include "Timer.h"
#include "images.h"
#include "pch.h"
#include "CoreSystem.h"
#include <minwindef.h>
#include <processenv.h>
#include <wingdi.h>
#include "Application.h"
#include "TabsManager.h"

// Callback function for EnumFontFamiliesEx
int CALLBACK EnumFontFamExProc(const LOGFONT* lpelfe, const TEXTMETRIC* lpntme, DWORD FontType, LPARAM lParam) {
    GL_INFO(std::filesystem::path(lpelfe->lfFaceName).generic_u8string().c_str());
    return 1; // Continue enumeration
}

// Function to get installed fonts
std::vector<std::string> GetInstalledFonts() {
    std::vector<std::string> fonts;
    LOGFONT lf = { 0 };
    lf.lfCharSet = DEFAULT_CHARSET;
    HDC hdc = GetDC(NULL);
    EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)&fonts, 0);
    ReleaseDC(NULL, hdc);
    return fonts;
}

int main(int argc,char* argv[])
{
	OpenGL::Timer timer;
	if(!Application::Init()) return -1;
	Application::SetupSystemSignalHandling();
	Application::SetApplicationIcon(logo_img,IM_ARRAYSIZE(logo_img));

	// Initialize ImGUI
	if(!Application::InitImGui()) return -1;
	Application::GetCoreSystem()->InitFonts();

	if(argc > 1) Application::HandleArguments(GetCommandLineW());

	CoreSystem::GetFileNavigation()->AddFolder("D:/Projects/c++/txedit");
	// TabsManager::Get().OpenFile("D:/Projects/c++/txedit/src/TextEditor.cpp");
	// TabsManager::Get().OpenFile("D:/Projects/c++/txedit/src/Application.cpp");
	// TabsManager::Get().OpenFile("D:/Projects/c++/txedit/src/TabsManager.cpp");

    // std::string path = "C:\\Windows\\Fonts";
    // std::ofstream file("./win32_fontfiles.txt");
    // for (const auto& entry : std::filesystem::directory_iterator(path)) file << entry.path().generic_u8string().c_str() << std::endl;

	// std::vector<std::string> fonts = GetInstalledFonts();


	GL_WARN("BootUp Time: {}ms",timer.ElapsedMillis());
	while (!glfwWindowShouldClose(Application::GetGLFWwindow())) {
		Application::Draw();
	}

	Application::Destroy();
	return 0;
}


#ifdef _WIN32
#ifdef GL_DIST
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	std::stringstream test(lpCmdLine);
	std::string segment;
	std::vector<std::string> seglist;

	while(std::getline(test, segment, ' ')) seglist.push_back(segment);
	
	return main((int)seglist.size(),&lpCmdLine);
}
#endif
#endif
