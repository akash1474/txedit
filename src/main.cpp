#include "GLFW/glfw3.h"
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
#include "FileNavigation.h"

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

void SleepForFPS(int target_fps) {
    static auto last_time = std::chrono::high_resolution_clock::now();
    auto frame_time = std::chrono::milliseconds(1000 / target_fps);

    auto current_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time);

    if (elapsed < frame_time) {
        std::this_thread::sleep_for(frame_time - elapsed);
    }

    last_time = std::chrono::high_resolution_clock::now();
}

int GetTargetFPS(GLFWwindow* window) {
	if(Application::RunAtMaxRefreshRate()) 
		return 300;

	if(!glfwGetWindowAttrib(Application::GetGLFWwindow(), GLFW_FOCUSED) && !glfwGetWindowAttrib(Application::GetGLFWwindow(), GLFW_HOVERED))
		Application::SetFrameRate(10.0f);
	else 
    	Application::SetFrameRate(60.0f);

    
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
        return 1;  // Minimized window: very low FPS
    }
    if (glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
        return 60; // Focused window: High FPS
    }
    return 30;  // Unfocused but visible window: Moderate FPS
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

	FileNavigation::AddFolder("D:/Projects/c++/txedit");
	TabsManager::OpenFile("D:/Projects/c++/txedit/src/TextEditor.cpp");
	// TabsManager::OpenFile("C:/Program Files/lite-xl/data/core/dirwatch.lua");
	// TabsManager::OpenFile("D:/Projects/c++/txedit/test/highlighting/test_python.py");
	// TabsManager::OpenFile("D:/Projects/c++/txedit/test/highlighting/test_java.java");

	// glfwSetWindowFocusCallback(Application::GetGLFWwindow(), WindowFocusCallback);
	// glfwSetWindowIconifyCallback(Application::GetGLFWwindow(), WindowIconifyCallback);
	// glfwSetWindowMaximizeCallback(Application::GetGLFWwindow(), WindowMaximizeCallback);

	GL_WARN("BootUp Time: {}ms", timer.ElapsedMillis());
	Application::SetWindowIsFocused(true);
	Application::SetFrameRate(120.0f);
#ifdef GL_DEBUG
	double prevTime = 0.0;
	double crntTime = 0.0;
	double timeDiff;
	unsigned int counter = 0;
#endif

	// std::string jsonPath="./assets/icons.json";
	// std::unordered_map<std::string, IconData> iconData= loadIconData(jsonPath);
	// std::string extension="cpp";
	// auto[key,icondata]=getIconForFile(extension, iconData);
	// GL_INFO("File:{}.png,Name:{}",key,icondata.name);
	
	while (!glfwWindowShouldClose(Application::GetGLFWwindow())) {

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
		Application::HandleFPSCooldown();
		int target_fps = GetTargetFPS(Application::GetGLFWwindow());
	    // Log the current ImGui-reported frame rate

		if(target_fps>1){
		    // Only draw if the window is focused
		    Application::Draw();
		}

		if(!Application::RunAtMaxRefreshRate())
	    	SleepForFPS(target_fps);
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
