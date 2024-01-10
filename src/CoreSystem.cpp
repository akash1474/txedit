#include "pch.h"
#include "CoreSystem.h"
#include <chrono>
#include <future>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_img.h"
#include "ImageTexture.h"

#ifdef GL_DEBUG

void CoreSystem::RenderDebugInfo(){
	static bool show_demo=true;
	ImGui::ShowDemoWindow(&show_demo);


	ImGui::Begin("Project");

	static int LineSpacing = 15.0f;
	if (ImGui::SliderInt("LineSpacing", &LineSpacing, 0, 20)) {
		Get().mTextEditor.setLineSpacing(LineSpacing);
	}

    ImGui::Spacing();
	ImGui::Text("PositionY:%.2f", ImGui::GetMousePos().y);
    ImGui::Spacing();
    ImGui::Text("mCursorPosition: X:%d  Y:%d",Get().mTextEditor.GetEditorState()->mCursorPosition.mColumn,Get().mTextEditor.GetEditorState()->mCursorPosition.mLine);
    ImGui::Text("mSelectionStart: X:%d  Y:%d",Get().mTextEditor.GetEditorState()->mSelectionStart.mColumn,Get().mTextEditor.GetEditorState()->mSelectionStart.mLine);
    ImGui::Text("mSelectionEnd:   X:%d  Y:%d",Get().mTextEditor.GetEditorState()->mSelectionEnd.mColumn,Get().mTextEditor.GetEditorState()->mSelectionEnd.mLine);


    ImGui::Spacing();
    ImGui::Spacing();
    static std::string mode;
    ImColor color;
    switch(Get().mTextEditor.GetSelectionMode()){
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
        Get().mTextEditor.ScrollToLineNumber(v);

    if(ImGui::Button("Select File")) SelectFile();

    if(ImGui::Button("Select Files")){
    	auto files=SelectFiles();
    	for(auto& file:files)
    		std::wcout << file << std::endl;
    }

    static ImageTexture img1("./assets/screenshots/editor.png");
    static ImageTexture img2("./assets/screenshots/multi_cursor.png");
    static ImageTexture img3("./assets/screenshots/selection.png");
    ImageTexture::AsyncImGuiImage(img1,ImVec2(362, 256));
    ImageTexture::AsyncImGuiImage(img2,ImVec2(362, 256));
    ImageTexture::AsyncImGuiImage(img3,ImVec2(362, 256));


	ImGui::End();
}

#endif


void CoreSystem::Render(){

	static const ImGuiIO& io=ImGui::GetIO();
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
	static ImGuiWindowFlags window_flags = ImGuiWindowFlags_None | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;



	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImVec2 size(viewport->WorkSize.x,StatusBarManager::IsInputPanelOpen() ? viewport->WorkSize.y - StatusBarManager::StatusBarSize - StatusBarManager::PanelSize : viewport->WorkSize.y- StatusBarManager::StatusBarSize);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    	window_flags |= ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Container",nullptr, window_flags);
	ImGui::PopStyleVar(3);


	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable){
        ImGuiID dockspace_id = ImGui::GetID("DDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

		static bool isFirst=true;
		if(isFirst){
			isFirst=false;
			ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
			ImGui::DockBuilderAddNode(dockspace_id,dockspace_flags | ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspace_id, size);

			auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.3f, nullptr, &dockspace_id);
			// auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.7f, nullptr, &dockspace_id);
			ImGui::DockBuilderDockWindow("Project Directory", dock_id_left);
			ImGui::DockBuilderDockWindow("Editor", dockspace_id);
			ImGui::DockBuilderFinish(dockspace_id);
		}
	}





	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			ImGui::MenuItem("New File");

			if(ImGui::MenuItem("Open File"))
				Get().mTextEditor.LoadFile(SelectFile().c_str());

			if(ImGui::MenuItem("Open Folder")){
				std::string path=SelectFolder();
				if(!path.empty()) Get().mFileNavigation.AddFolder(path.c_str());
			}
			
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
	RenderDebugInfo();
	#endif

	if(Get().mFileNavigation.IsOpen()) Get().mFileNavigation.Render();

	Get().mTextEditor.Render();

	StatusBarManager::Render(size,viewport);

}



void CoreSystem::HandleArguments(int argc,char* argv[]){
	namespace fs=std::filesystem;
	for (int i = 1; i < argc; ++i) {
        fs::path path(argv[i]);
        if (fs::exists(path)) {
            if (fs::is_regular_file(path)) {
            	GL_INFO("FILE:{}",path.generic_string());
                CoreSystem::GetTextEditor()->LoadFile(path.generic_string().c_str());
            } else if (fs::is_directory(path)) {
            	GL_INFO("FOLDER:{}",path.generic_string());
                CoreSystem::GetFileNavigation()->AddFolder(path.generic_string());
            } else {
            	ShowErrorMessage("Invalid File/Folder Selected");
            }
        } else {
            ShowErrorMessage("Path Doesn't Exists");
        }
    }
}

bool CoreSystem::Init(){

	#ifdef GL_DEBUG
		OpenGL::Log::Init();
	#endif

	if (!glfwInit()) return false;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

	Get().mWindow = glfwCreateWindow(WIDTH, HEIGHT, "TxEdit", NULL, NULL);
	glfwSetWindowSizeLimits(Get().mWindow, 330, 500, GLFW_DONT_CARE, GLFW_DONT_CARE);
	if (!Get().mWindow) {
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(Get().mWindow);
	return true;
}

void CoreSystem::SetApplicationIcon(unsigned char* logo_img,int length){
	GLFWimage images[1];
	images[0].pixels = stbi_load_from_memory(logo_img, length, &images[0].width, &images[0].height, 0, 4); // rgba channels
	glfwSetWindowIcon(Get().mWindow, 1, images);
	stbi_image_free(images[0].pixels);
}

bool CoreSystem::InitImGui(){
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();


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


	ImGui_ImplGlfw_InitForOpenGL(Get().mWindow, true);
	if (!ImGui_ImplOpenGL2_Init()){
		GL_ERROR("Failed to initit OpenGL 2");
		return false;
	}
	return true;
}

float GetFontSize(){
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);	
    float baseSize = 12.0f;
    float scaleFactor = 0.007f;
    float fontSize = baseSize + std::min(screenWidth, screenHeight) * scaleFactor;
    return fontSize;
}


void CoreSystem::InitFonts(){
	GL_INFO("Initializing Fonts");
	const ImGuiIO& io=ImGui::GetIO();
	io.Fonts->Clear();
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
	const float font_size=GetFontSize();
	io.Fonts->AddFontFromMemoryTTF((void*)data_font, font_data_size, font_size, &font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)data_icon, icon_data_size, (font_size+4.0f) * 2.0f / 3.0f, &icon_config, icons_ranges);


	io.Fonts->AddFontFromMemoryTTF((void*)monolisa_medium, IM_ARRAYSIZE(monolisa_medium), font_size-4.0f, &font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)data_icon_regular, icon_regular_data_size, (font_size+4.0f) * 2.0f / 3.0f, &icon_config, icons_ranges);
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	CoreSystem::GetTextEditor()->RecalculateBounds();
	glViewport(0, 0, width, height);
	CoreSystem::Get().Draw();
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
	for (int i = 0; i < count; i++) {
		if (std::filesystem::is_directory(paths[i])) {
			GL_INFO("Folder: {}", paths[i]);
			CoreSystem::GetFileNavigation()->AddFolder(paths[i]);
		} else {
			GL_INFO("File: {}", paths[i]);
			CoreSystem::GetTextEditor()->LoadFile(paths[i]);
		}
	}
}

void CoreSystem::Draw()
{
	glfwPollEvents();
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	glfwSetDropCallback(Get().mWindow, drop_callback);

	CoreSystem::Render();

	ImGui::Render();
	int display_w, display_h;
	glfwGetFramebufferSize(Get().mWindow, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glfwSetFramebufferSizeCallback(Get().mWindow, framebuffer_size_callback);

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}

	glfwMakeContextCurrent(Get().mWindow);
	glfwSwapBuffers(Get().mWindow);
}

void CoreSystem::Destroy(){
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(GetGLFWwindow());
	glfwTerminate();
}
