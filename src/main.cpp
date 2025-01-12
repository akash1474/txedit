#include "GLFW/glfw3.h"
#define _CRTDBG_MAP_ALLOC
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

//Make sure Visual Leak Detector (VLD) is installed
#ifdef DETECT_MEMORY_LEAKS_VLD
	#include <vld.h>
#endif

void WindowFocusCallback(GLFWwindow* window, int focused) {
    if (focused) {
    	Application::SetFrameRate(120.0f);
    	Application::SetWindowIsFocused(true);
    }
    else
    {
    	Application::SetFrameRate(10.0f);
    	Application::SetWindowIsFocused(false);
    }

}

void WindowIconifyCallback(GLFWwindow* window, int iconified) {
    if (iconified){
    	Application::SetFrameRate(10.0f);
    	Application::SetWindowIsFocused(false);
    }
}

void WindowMaximizeCallback(GLFWwindow* window, int maximized) {
    if (maximized){
    	Application::SetFrameRate(120.0f);
    	Application::SetWindowIsFocused(true);
    }
}


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

	glfwSetWindowFocusCallback(Application::GetGLFWwindow(), WindowFocusCallback);
	glfwSetWindowIconifyCallback(Application::GetGLFWwindow(), WindowIconifyCallback);
	glfwSetWindowMaximizeCallback(Application::GetGLFWwindow(), WindowMaximizeCallback);

	GL_WARN("BootUp Time: {}ms", timer.ElapsedMillis());
	Application::SetWindowIsFocused(true);
	Application::SetFrameRate(120.0f);
#ifdef GL_DEBUG
	double prevTime = 0.0;
	double crntTime = 0.0;
	double timeDiff;
	unsigned int counter = 0;
#endif
	
	while (!glfwWindowShouldClose(Application::GetGLFWwindow())) {
	    auto start = std::chrono::high_resolution_clock::now();

	#ifdef GL_DEBUG
		crntTime = glfwGetTime();
		timeDiff = crntTime - prevTime;
		counter++;
		if (timeDiff >= 1.0 / 30.0)
		{
			std::string FPS = std::to_string((1.0 / timeDiff) * counter);
			std::string ms = std::to_string((timeDiff / counter) * 1000);
			std::string newTitle = "TxEdit - " + FPS + "FPS / " + ms + "ms";
			glfwSetWindowTitle(Application::GetGLFWwindow(), newTitle.c_str());
			// GL_INFO(newTitle);

			prevTime = crntTime;
			counter = 0;
		}
	#endif

	    // Log the current ImGui-reported frame rate

	    // Only draw if the window is focused
	    Application::Draw();

	    auto end = std::chrono::high_resolution_clock::now();
	    std::chrono::duration<double> frameTime = end - start;

	    // Ensure frame time meets the target
	    while (frameTime.count() < Application::GetFrameTime()) {
	        auto sleepStart = std::chrono::high_resolution_clock::now();
	        std::this_thread::sleep_for(std::chrono::duration<double>(Application::GetFrameTime() - frameTime.count()));
	        auto sleepEnd = std::chrono::high_resolution_clock::now();

	        frameTime += sleepEnd - sleepStart;
	    }
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
