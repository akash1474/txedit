#include "FontAwesome6.h"
#include "pch.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include <filesystem>
#include <shellapi.h>
#include <sstream>
#include <string>
#include <winuser.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_img.h"
#include <codecvt>
#include "CoreSystem.h"
#include "TextEditor.h"

std::string exec(const char* cmd) {
	// bool status = !std::system(cmd);
	// if (!status) return "None";
    char fcmd[128];
    sprintf_s(fcmd, sizeof(fcmd), "/c %s", cmd);
	SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_FLAG_NO_UI;
    sei.lpFile = "cmd.exe";
    sei.lpParameters = fcmd;
    sei.nShow = SW_SHOWDEFAULT; // Hide the console window
    if(ShellExecuteExA(&sei)){
		std::string result;
		std::ifstream file("git.txt");
		std::getline(file,result);
		file.close();
		std::filesystem::remove("git.txt");
		return result;
    }
    return "None";
}


#define WIDTH 900
#define HEIGHT 600

Editor editor;
CoreSystem coreSystem;



void draw(GLFWwindow* window, ImGuiIO& io);

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	coreSystem.GetTextEditor()->RecalculateBounds();
	glViewport(0, 0, width, height);
	draw(window, ImGui::GetIO());
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
	for (int i = 0; i < count; i++) {
		if (std::filesystem::is_directory(paths[i])) {
			GL_INFO("Folder: {}", paths[i]);
			coreSystem.GetFileNavigation()->AddFolder(paths[i]);
		} else {
			GL_INFO("File: {}", paths[i]);
			coreSystem.GetTextEditor()->LoadFile(paths[i]);
		}
	}
}


void draw(GLFWwindow* window, ImGuiIO& io)
{
	glfwPollEvents();
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	glfwSetDropCallback(window, drop_callback);

	coreSystem.Render();

	ImGui::Render();
	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}

	glfwMakeContextCurrent(window);
	glfwSwapBuffers(window);
}


void HandleArguments(int argc,char* argv[]){
	namespace fs=std::filesystem;
	for (int i = 1; i < argc; ++i) {
        fs::path path(argv[i]);
        if (fs::exists(path)) {
            if (fs::is_regular_file(path)) {
            	GL_INFO("FILE:{}",path.generic_string());
                coreSystem.GetTextEditor()->LoadFile(path.generic_string().c_str());
            } else if (fs::is_directory(path)) {
            	GL_INFO("FOLDER:{}",path.generic_string());
                coreSystem.GetFileNavigation()->AddFolder(path.generic_string());
            } else {
            	ShowErrorMessage("Invalid File/Folder Selected");
            }
        } else {
            ShowErrorMessage("Path Doesn't Exists");
        }
    }
}


int main(int argc,char* argv[])
{

	if(argc > 1) HandleArguments(argc,argv);

	GLFWwindow* window;
	
	#ifdef GL_DEBUG
		OpenGL::Log::Init();
	#endif

	if (!glfwInit()) return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "TxEdit", NULL, NULL);
	glfwSetWindowSizeLimits(window, 330, 500, GLFW_DONT_CARE, GLFW_DONT_CARE);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	GLFWimage images[1];
	images[0].pixels = stbi_load_from_memory(logo_img, IM_ARRAYSIZE(logo_img), &images[0].width, &images[0].height, 0, 4); // rgba channels
	glfwSetWindowIcon(window, 1, images);
	stbi_image_free(images[0].pixels);

	// Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();


	GL_INFO("Initializing Fonts");
	io.Fonts->Clear();
	const std::string app_dir=GetUserDirectory("txedit");
	io.IniFilename=app_dir.c_str();
	io.LogFilename = nullptr;

	glfwSwapInterval(1);
	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameRounding = 2.0f;
	style.ItemSpacing.y = 6.0f;
	style.ScrollbarRounding = 2.0f;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}


	ImGui_ImplGlfw_InitForOpenGL(window, true);
	if (!ImGui_ImplOpenGL2_Init()) GL_ERROR("Failed to initit OpenGL 2");

	static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
	ImFontConfig icon_config;
	icon_config.MergeMode = true;
	icon_config.PixelSnapH = true;
	icon_config.FontDataOwnedByAtlas = false;

	const int font_data_size = IM_ARRAYSIZE(data_font);
	const int icon_data_size = IM_ARRAYSIZE(data_icon);
	const int icon_regular_data_size = IM_ARRAYSIZE(data_icon_regular);

	ImFontConfig font_config;
	font_config.FontDataOwnedByAtlas = false;
	io.Fonts->AddFontFromMemoryTTF((void*)data_font, font_data_size, 16, &font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)data_icon, icon_data_size, 20 * 2.0f / 3.0f, &icon_config, icons_ranges);


	io.Fonts->AddFontFromMemoryTTF((void*)monolisa_medium, IM_ARRAYSIZE(monolisa_medium), 12, &font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)data_icon_regular, icon_regular_data_size, 20 * 2.0f / 3.0f, &icon_config, icons_ranges);


    StyleColorDarkness();

	coreSystem.GetTextEditor()->LoadFile("D:/Projects/c++/txedit/src/TextEditor.cpp");
	coreSystem.GetFileNavigation()->AddFolder("D:/Projects/c++/txedit");

	while (!glfwWindowShouldClose(window)) {
		draw(window, io);
	}
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}