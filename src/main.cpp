#include "pch.h"
#include "Log.h"
#include "Timer.h"
#include "resources/AppIcon.embed"
#include "CoreSystem.h"
#include <minwindef.h>
#include <processenv.h>
#include <wingdi.h>
#include "Application.h"
#include "TabsManager.h"



int main(int argc, char* argv[])
{
	OpenGL::Timer timer;
	if (!Application::Init())
		return -1;
	Application::SetupSystemSignalHandling();
	Application::SetApplicationIcon(AppIcon, IM_ARRAYSIZE(AppIcon));


	// Initialize ImGUI
	if (!Application::InitImGui())
		return -1;
	Application::GetCoreSystem()->InitFonts();

	if (argc > 1)
		Application::HandleArguments(GetCommandLineW());

	CoreSystem::GetFileNavigation()->AddFolder("D:/Projects/c++/txedit");
	TabsManager::Get().OpenFile("D:/Projects/c++/txedit/src/TextEditor.cpp");
	// TabsManager::Get().OpenFile("D:/Projects/c++/txedit/src/Application.cpp");
	// TabsManager::Get().OpenFile("D:/Projects/c++/txedit/src/TabsManager.cpp");

	GL_WARN("BootUp Time: {}ms", timer.ElapsedMillis());
	// const double TARGET_FPS = 60.0;
	// const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;
	while (!glfwWindowShouldClose(Application::GetGLFWwindow())) {
		// auto start = std::chrono::high_resolution_clock::now();
		Application::Draw();

		// auto end = std::chrono::high_resolution_clock::now();
		// std::chrono::duration<double> frameTime = end - start;

		// if (frameTime.count() < TARGET_FRAME_TIME) {
		// 	std::this_thread::sleep_for(std::chrono::duration<double>(TARGET_FRAME_TIME - frameTime.count()));
		// }
	}

	Application::Destroy();
	return 0;
}


#ifdef _WIN32
	#ifdef GL_DIST
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	std::stringstream test(lpCmdLine);
	std::string segment;
	std::vector<std::string> seglist;

	while (std::getline(test, segment, ' ')) seglist.push_back(segment);

	return main((int)seglist.size(), &lpCmdLine);
}
	#endif
#endif
