#include "FontAwesome6.h"
#include "pch.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include <filesystem>
#include <shellapi.h>
#include <sstream>
#include <string>
#include <unordered_map>
#include <winuser.h>
#define STB_IMAGE_IMPLEMENTATION
#include "TextEditor.h"
#include "stb_img.h"
#include <codecvt>

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

int width{0};
int height{0};

Editor editor;
struct Entity{
	std::string filename;
	std::string path;
	bool is_directory=false;
	bool is_explored=false;
};

std::unordered_map<std::string,std::vector<Entity>> mDirectoryData;



void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void draw(GLFWwindow* window, ImGuiIO& io);

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	editor.reCalculateBounds = true;
	glViewport(0, 0, width, height);
	draw(window, ImGui::GetIO());
}

std::string wstringToUTF8(const std::wstring& input) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(input);
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
	for (int i = 0; i < count; i++) {
		if (std::filesystem::is_directory(paths[i])) {
			GL_INFO("Folder: {}", paths[i]);
		} else {
			GL_INFO("File: {}", paths[i]);
			std::ifstream t(paths[i]);
			t.seekg(0, std::ios::end);
			// size = t.tellg();
			// file_data.resize(size, ' ');
			// t.seekg(0);
			// t.read(&file_data[0], size);
			// editor.SetBuffer(file_data);
		}
	}
}

void renderFolderItems(std::string path,bool isRoot=false){
	if(isRoot){
    	static std::string folderName=std::filesystem::path(path).filename().generic_string();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(6.0f,2.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(4.0f,2.0f));
		if(ImGui::TreeNodeEx(folderName.c_str(),ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen)){

			if(ImGui::IsItemClicked(ImGuiMouseButton_Right)) ImGui::OpenPopup("folder_opt");

	        const char* options[] = {ICON_FA_CARET_RIGHT"  New File",ICON_FA_CARET_RIGHT"  Rename",ICON_FA_CARET_RIGHT"  Open Folder",ICON_FA_CARET_RIGHT"  Open Terminal",ICON_FA_CARET_RIGHT"  New Folder",ICON_FA_CARET_RIGHT"  Delete Folder"};
	        static int selected=-1;

	        if (ImGui::BeginPopup("folder_opt"))
	        {
	            for (int i = 0; i < IM_ARRAYSIZE(options); i++){
	            	if(i==4) ImGui::Separator();
	                if (ImGui::Selectable(options[i])) selected = i;
	            }
	            ImGui::EndPopup();
	        }

    		renderFolderItems(path);
    		ImGui::TreePop();
    	}
    	ImGui::PopStyleVar(2);
    	return;
	}
	if(mDirectoryData.empty()  || mDirectoryData.find(path)==mDirectoryData.end()){
		std::cout << path << std::endl;
		auto& entities=mDirectoryData[path];
		for(const auto& entity:std::filesystem::directory_iterator(path)) entities.push_back({
			entity.path().filename().generic_string(),
			entity.path().generic_string(),
			entity.is_directory(),
			false
		});
		std::stable_partition(entities.begin(), entities.end(), [](const auto& entity) { return entity.is_directory; });
	}
	auto& entities=mDirectoryData[path];
	if(entities.empty()) return;
	std::stringstream oss;
	for(Entity& item:entities){
		const char* icon = item.is_directory ? item.is_explored ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER : ICON_FA_FILE;
		oss << icon << " " << item.filename.c_str();
		if(item.is_directory) {
			if(ImGui::TreeNodeEx(oss.str().c_str(),ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Selected)){
				std::stringstream wss;
				wss << path << "\\" << item.filename.c_str();
				renderFolderItems(wss.str(),false);
				ImGui::TreePop();
			}
		}
		else{
			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(6.0f,8.0f));
			ImGui::PushStyleColor(ImGuiCol_Header,ImGui::GetStyle().Colors[ImGuiCol_SliderGrabActive]);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImGui::GetStyle().Colors[ImGuiCol_SliderGrab]);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX()+28.0f);
			if(ImGui::Selectable(oss.str().c_str(),item.is_explored,ImGuiSelectableFlags_SpanAvailWidth|ImGuiSelectableFlags_SelectOnClick)){
				for(Entity& en:entities) en.is_explored=false;
				if(!item.is_explored) item.is_explored=true;
				editor.LoadFile(item.path.c_str());
				editor.reCalculateBounds=true;
			}
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(2);
			ImGui::PopFont();
		}
		oss.str("");
	}
}

void draw(GLFWwindow* window, ImGuiIO& io)
{
	glfwPollEvents();
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	// ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
	glfwSetDropCallback(window, drop_callback);

	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode;
	static ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	static bool is_open = true;
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImVec2 size(viewport->WorkSize.x,viewport->WorkSize.y-22.0f);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	window_flags |= ImGuiWindowFlags_NoBackground;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Container", &is_open, window_flags);
	ImGui::PopStyleVar(3);
	static ImGuiID dockspace_id = ImGui::GetID("DDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f),dockspace_flags);

	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			ImGui::MenuItem("New File");
			ImGui::MenuItem("Open File");
			ImGui::MenuItem("Open Folder");
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			ImGui::MenuItem("Cut");
			ImGui::MenuItem("Copy");
			ImGui::MenuItem("Paste");
            ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::End();
	#ifdef GL_DEBUG
	static bool show_demo=true;
	ImGui::ShowDemoWindow(&show_demo);


	ImGui::Begin("Project");


	static int LineSpacing = 10.0f;
	if (ImGui::SliderInt("LineSpacing", &LineSpacing, 0, 20)) {
		editor.setLineSpacing(LineSpacing);
	}

    ImGui::Spacing();
	ImGui::Text("PositionY:%.2f", ImGui::GetMousePos().y);
    ImGui::Spacing();
    ImGui::Text("mCursorPosition: X:%d  Y:%d",editor.GetEditorState()->mCursorPosition.mColumn,editor.GetEditorState()->mCursorPosition.mLine);
    ImGui::Text("mSelectionStart: X:%d  Y:%d",editor.GetEditorState()->mSelectionStart.mColumn,editor.GetEditorState()->mSelectionStart.mLine);
    ImGui::Text("mSelectionEnd:   X:%d  Y:%d",editor.GetEditorState()->mSelectionEnd.mColumn,editor.GetEditorState()->mSelectionEnd.mLine);


    ImGui::Spacing();
    ImGui::Spacing();
    static std::string mode;
    ImColor color;
    switch(editor.GetSelectionMode()){
	    case 0: 
	    	mode="Normal";
	    	color=ImColor(50,206,187,255);
	    	break;
	    case 1: 
	    	mode="Word";
	    	color=ImColor(233,196,106,255);
	    	break;
	    case 2: 
	    	mode="Line";
	    	color=ImColor(231,111,81,255);
	    	break;
    }
    ImGui::Text("SelectionMode: ");
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text,color.Value);
    ImGui::Text("%s", mode.c_str());
    ImGui::PopStyleColor();



    ImGui::Spacing();
    ImGui::Spacing();
    static int v{1};
    ImGui::Text("Goto Line: "); ImGui::SameLine(); 
    if(ImGui::InputInt("##ScrollToLine", &v,1,100,ImGuiInputTextFlags_EnterReturnsTrue))
        editor.ScrollToLineNumber(v);

    if(ImGui::Button("Select File")) SelectFile();

    if(ImGui::Button("Select Files")){
    	auto files=SelectFiles();
    	for(auto& file:files)
    		std::wcout << file << std::endl;
    }

	ImGui::End();
	#endif

	static float s_width=250.0f;
	static bool is_opening=true;
	static Animation side_bar;

	// static float curr=300.0f;
	// if(side_bar.hasStarted){
	// 	float width=(is_opening ? s_width*side_bar.update() : s_width*(1.0f-side_bar.update()));
	// 	GL_INFO(width);
	// 	curr=width;
	// 	if(width > 1.0f) ImGui::SetNextWindowSize(ImVec2{width,-1.0f});

	// }
	if(is_opening){
		ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0.067f,0.075f,0.078f,1.000f));
		ImGui::SetNextWindowSize(ImVec2{s_width,-1.0f},ImGuiCond_Once);
		ImGui::Begin("Project Directory");
	    // static bool isLoaded=false;
	   	// static std::string folderPath; 
	    // if(ImGui::Button("Select Folder")) {
	    // 	folderPath=wstringToUTF8(SelectFolder());
	    // 	isLoaded=true;
	    // }
	    // if(isLoaded){
	    	// renderFolderItems(folderPath,true);
	    // }
	    	// ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(0.067,0.074,0.078,0.000));
	    	renderFolderItems("D:/Projects/c++/txedit",true);
	    	// ImGui::PopStyleColor();
		ImGui::End();
		ImGui::PopStyleColor();
	}


	editor.Render();
	// #ifdef GL_DEBUG
	// ImGui::Begin("Console");
	// ImGui::End();
	// #endif




	ImGui::SetNextWindowPos(ImVec2(0,size.y));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x,22.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(4.0f,0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg,ImGui::GetStyle().Colors[ImGuiCol_TitleBg]);
	ImGui::PushFont(io.Fonts->Fonts[1]);
	ImGui::Begin("Status Bar",0,ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize );
		ImGui::PushFont(io.Fonts->Fonts[0]);
		if(ImGui::Button(ICON_FA_BARS)){
			//Animate Sidebar
			// side_bar.start();
			is_opening=!is_opening;
			// int success = !std::system("git rev-parse --abbrev-ref HEAD");
			// GL_INFO(success);
		}
		ImGui::PopFont();
		ImGui::SameLine();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY()+2.0f);
		ImGui::Text("Line:%d",editor.GetEditorState()->mCursorPosition.mLine+1);
		ImGui::SameLine();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY()+2.0f);
		ImGui::Text("Column:%d",editor.GetEditorState()->mCursorPosition.mColumn+1);

		static Animation aFileSave(2.0f);

		if(editor.isFileSaving){
			editor.isFileSaving=false;
			aFileSave.start();
		}
		if(aFileSave.hasStarted){
			ImGui::SameLine();
			ImGui::SetCursorPosY(ImGui::GetCursorPosY()+2.0f);
			ImGui::TextColored(ImVec4(0.165,0.616,0.561,1.000),"Saved:%s",editor.GetCurrentFilePath().c_str());
			aFileSave.update();
		}


		static bool branchLoaded =false;
		static std::string branch;
		if(!branchLoaded){
			// branch=exec("git rev-parse --abbrev-ref HEAD > git.txt");
			branchLoaded=true;
		}
		if(!branch.empty()){
			ImGui::PushFont(io.Fonts->Fonts[0]);
			ImGui::SameLine(ImGui::GetWindowWidth()-60.0f);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY());
			ImGui::Text("%s %s",ICON_FA_CODE_BRANCH,branch.c_str());
			ImGui::PopFont();
		}
	ImGui::End();
	ImGui::PopFont();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);

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


int main(void)
{
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
	// io.IniFilename=nullptr;
	io.LogFilename = nullptr;

	glfwSwapInterval(1);
	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameRounding = 2.0f;
	style.ItemSpacing.y = 6.0f;
	style.ScrollbarRounding = 2.0f;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}


	ImGui_ImplGlfw_InitForOpenGL(window, true);
	if (!ImGui_ImplOpenGL2_Init()) GL_ERROR("Failed to initit OpenGL 2");

	// glfwSetKeyCallback(window, keyboardCallback);

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


	// StyleColorsDracula();
	// auto& colors=ImGui::GetStyle().Colors;
    // colors[ImGuiCol_TitleBg]=ImVec4{0.067,0.075,0.078,1.000};
    // colors[ImGuiCol_TitleBgActive]=ImVec4{0.067,0.075,0.078,1.000};
    // colors[ImGuiCol_Tab]=ImVec4{0.067,0.075,0.078,1.000};
    // colors[ImGuiCol_TabActive] = ImVec4{0.114,0.125,0.129,1.000};
    // colors[ImGuiCol_TabHovered] = ImVec4{0.134,0.135,0.139,1.000};
    StyleColorDarkness();

	editor.LoadFile("D:/Projects/c++/txedit/src/TextEditor.cpp");

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