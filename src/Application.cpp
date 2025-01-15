#include "FileNavigation.h"
#include "pch.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "Timer.h"
#include "Log.h"
#include "Application.h"
#include "CoreSystem.h"
#include "MultiThreading.h"
#include <csignal>
#include <filesystem>
#include <shellapi.h>
#include <stdio.h>
#include <winuser.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include <dwmapi.h>
#include <windef.h>
#include <wingdi.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_img.h"

// For files being dragged and dropped
void drop_callback(GLFWwindow* window, int count, const char** paths)
{
	for (int i = 0; i < count; i++) {
		if (std::filesystem::is_directory(paths[i])) {
			GL_INFO("Folder: {}", paths[i]);
			FileNavigation::AddFolder(paths[i]);
		} else {
			GL_INFO("File: {}", paths[i]);
			CoreSystem::GetTextEditor()->LoadFile(paths[i]);
		}
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	CoreSystem::GetTextEditor()->RecalculateBounds();
	glViewport(0, 0, width, height);
	Application::Get().Draw();
}

bool Application::Init()
{
	OpenGL::ScopedTimer timer("Application::Init");

#ifdef GL_DEBUG
	OpenGL::Log::Init();
#endif

	if (!glfwInit())
		return false;

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, 1);

	Get().mWindow = glfwCreateWindow(Get().width, Get().height, APP_NAME, NULL, NULL);
	glfwSetWindowSizeLimits(Get().mWindow, 380, 500, GLFW_DONT_CARE, GLFW_DONT_CARE);
	if (!Get().mWindow) {
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(Get().mWindow);
	GL_INFO("OPENGL - {}", (const char*)glGetString(GL_VERSION));
	HWND WinHwnd = glfwGetWin32Window(Application::GetGLFWwindow());
	BOOL USE_DARK_MODE = true;
	BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS =
	    SUCCEEDED(DwmSetWindowAttribute(WinHwnd, DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE, &USE_DARK_MODE, sizeof(USE_DARK_MODE)));


	glfwSetDropCallback(Get().mWindow, drop_callback);
	glfwSetFramebufferSizeCallback(Get().mWindow, framebuffer_size_callback);
	return true;
}


void Application::SetApplicationIcon(unsigned char* logo_img, int length)
{
	OpenGL::ScopedTimer timer("AppIconLoadTime");
	GLFWimage images[1];
	images[0].pixels = stbi_load_from_memory(logo_img, length, &images[0].width, &images[0].height, 0, 4); // rgba channels
	glfwSetWindowIcon(Get().mWindow, 1, images);
	stbi_image_free(images[0].pixels);
}


bool Application::InitImGui()
{
	GL_INFO("ImGui Init");
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// ImGui::GetStyle().ScaleAllSizes(0.5f);


#ifndef GL_DEBUG
	const std::string app_dir = GetUserDirectory("txedit");
	io.IniFilename = app_dir.c_str();
	io.LogFilename = nullptr;
#endif

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	if (!ImGui_ImplGlfw_InitForOpenGL(Get().mWindow, true)) {
		GL_CRITICAL("FAILED INIT IMGUI");
		return false;
	}
#ifdef GL_BUILD_OPENGL2
	if (!ImGui_ImplOpenGL2_Init()) {
		GL_INFO("INIT OPENGL2");
#else
	if (!ImGui_ImplOpenGL3_Init()) {
		GL_INFO("INIT OPENGL3");
#endif
		GL_ERROR("Failed to initit OpenGL");
		return false;
	}
	GL_INFO("OPENGL - {}", (const char*)glGetString(GL_VERSION));

	glfwSwapInterval(0); // Gives maximum FPS
	SetStyleColorDarkness();
	return true;
}

void Application::CenterWindowOnScreen()
{
	int horizontal = 0;
	int vertical = 0;
	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);
	horizontal = desktop.right;
	vertical = desktop.bottom;
	// Get().GetCoreState()->position=ImVec2((horizontal - Get().GetCoreState()->width) * 0.5f, (vertical - Get().GetCoreState()->height) *
	// 0.5f); glfwSetWindowPos(Get().mWindow,(int)Get().GetCoreState()->position.x,(int)Get().GetCoreState()->position.y);
}

void Application::BackupDataBeforeCrash()
{
	// std::ofstream file(mCoreSystem.m_CacheDir+"\\settings.json");
	// if (!file.is_open()) {
	//     GL_ERROR("Failed to Create New File");
	//     return;
	// }
	// nlohmann::json jsonData;
	// jsonData["version"]="v0.7.0";
	// jsonData["hasCrashed"]=true;
	// for(const auto&[id,dir]:this->mCoreSystem.GetSettings()->mDirs){
	//     jsonData["dirs"][id]=dir;
	// }
}



void Application::Draw()
{
	glfwPollEvents();
	if(!Application::IsWindowFocused()) return;
#ifdef GL_BUILD_OPENGL2
	ImGui_ImplOpenGL2_NewFrame();
#else
	ImGui_ImplOpenGL3_NewFrame();
#endif
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();



	Application::GetCoreSystem()->Render();
	ImGui::Render();
	int display_w, display_h;
	glfwGetFramebufferSize(Get().mWindow, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef GL_BUILD_OPENGL2
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
#else
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

	// if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
	// 	GLFWwindow* backup_current_context = glfwGetCurrentContext();
	// 	ImGui::UpdatePlatformWindows();
	// 	ImGui::RenderPlatformWindowsDefault();
	// 	glfwMakeContextCurrent(backup_current_context);
	// }

	// glfwMakeContextCurrent(Get().mWindow);
	glfwSwapBuffers(Get().mWindow);
}


void Application::Destroy()
{
#ifdef GL_BUILD_OPENGL2
	ImGui_ImplOpenGL2_Shutdown();
#else
	ImGui_ImplOpenGL3_Shutdown();
#endif
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(GetGLFWwindow());
	glfwTerminate();
}

void Application::HandleArguments(std::wstring commands)
{
	// TODO: Display Notification to user if something goes wrong
	GL_INFO("Parsing Args");
	namespace fs = std::filesystem;
	int nArgs;

	LPWSTR* wArgList = CommandLineToArgvW(commands.c_str(), &nArgs);
	if (!wArgList) {
		GL_ERROR("Failed to parse args");
		return;
	}

	CoreSystem* core = Application::GetCoreSystem();

	for (int i = 1; i < nArgs; ++i) {
		fs::path path(wArgList[i]);
		if (fs::exists(path)) {
			if (fs::is_regular_file(path)) {
				GL_INFO("FILE:{}", path.generic_string());
				core->GetTextEditor()->LoadFile(path.generic_string().c_str());
			} else if (fs::is_directory(path)) {
				GL_INFO("FOLDER:{}", path.generic_string());
				FileNavigation::AddFolder(path.generic_string());
			} else {
				ShowErrorMessage("Invalid File/Folder Selected");
			}
		} else {
			ShowErrorMessage("Path Doesn't Exists");
		}
	}

	LocalFree(wArgList);
}


void Application::HandleCrash(int signal)
{
	const char* errorMessage = nullptr;
	switch (signal) {
		case SIGSEGV:
			errorMessage = "Segmentation fault";
			break;
		case SIGABRT:
			errorMessage = "Abort signal received";
			break;
		case SIGFPE:
			errorMessage = "Floating point exception";
			break;
		case SIGILL:
			errorMessage = "Illegal instruction";
			break;
		case SIGTERM:
			errorMessage = "Termination request";
			break;
		case SIGINT:
			errorMessage = "User Interrupt(Ctrl+C)";
		default:
			errorMessage = "Unknown error";
			break;
	}

	GL_CRITICAL("Error:{} (Signal: {})", errorMessage, signal);
}


void Application::SetupSystemSignalHandling()
{
	std::signal(SIGSEGV, Application::HandleCrash); // Segmentation fault
	std::signal(SIGABRT, Application::HandleCrash); // Abort
	std::signal(SIGFPE, Application::HandleCrash);  // Floating point exception
	std::signal(SIGILL, Application::HandleCrash);  // Illegal instruction
	std::signal(SIGTERM, Application::HandleCrash); // Termination request
	std::signal(SIGINT, Application::HandleCrash);  // Interrupt (Ctrl+C)
}
