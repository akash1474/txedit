#include "Log.h"
#include "Timer.h"
#include "images.h"
#include "pch.h"
#include "CoreSystem.h"
#include <processenv.h>
#include "Application.h"



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

	CoreSystem::GetTextEditor()->LoadFile("D:/Projects/c++/txedit/src/TextEditor.cpp");
	CoreSystem::GetFileNavigation()->AddFolder("D:/Projects/c++/txedit");


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
