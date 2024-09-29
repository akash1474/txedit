#include "imgui.h"
#include "pch.h"
#include "CoreSystem.h"
#include "ImageTexture.h"
#include "MultiThreading.h"
#include "resources/FontAwesomeRegular.embed"
#include "resources/FontAwesomeSolid.embed"
#include "resources/MonoLisaRegular.embed"
#include "resources/JetBrainsMonoNLRegular.embed"
#include "resources/JetBrainsMonoNLItalic.embed"
#include <chrono>
#include <cstring>
#include <thread>
#include "Terminal.h"

#ifdef GL_DEBUG



void CoreSystem::RenderDebugInfo(){
	static bool show_demo=true;
	ImGui::ShowDemoWindow(&show_demo);


	ImGui::Begin("Project");

	static int LineSpacing = 15.0f;
	if (ImGui::SliderInt("LineSpacing", &LineSpacing, 0, 20)) {
		Get().mTextEditor.SetLineSpacing(LineSpacing);
	}

    ImGui::Spacing();
	ImGui::Text("PositionY:%.2f", ImGui::GetMousePos().y);
    ImGui::Spacing();
    ImGui::Text("mCursorPosition: X:%d  Y:%d",Get().mTextEditor.GetEditorState()->mCursorPosition.mColumn,Get().mTextEditor.GetEditorState()->mCursorPosition.mLine);
    ImGui::Text("mSelectionStart: X:%d  Y:%d",Get().mTextEditor.GetEditorState()->mSelectionStart.mColumn,Get().mTextEditor.GetEditorState()->mSelectionStart.mLine);
    ImGui::Text("mSelectionEnd:   X:%d  Y:%d",Get().mTextEditor.GetEditorState()->mSelectionEnd.mColumn,Get().mTextEditor.GetEditorState()->mSelectionEnd.mLine);
    ImGui::Text("mUndoManagerTop:   X:%d  Y:%d",Get().mTextEditor.GetEditorState()->mSelectionEnd.mColumn,Get().mTextEditor.GetEditorState()->mSelectionEnd.mLine);

    ImGui::Spacing();
    CoreSystem::GetTextEditor()->GetUndoMananger()->DisplayUndoStack();

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

    #ifdef GL_DEBUG

    static ImageTexture img1("./assets/screenshots/editor.png");
    static ImageTexture img2("./assets/screenshots/multi_cursor.png");
    static ImageTexture img3("./assets/screenshots/selection.png");
    static bool pushed=false;
    if(!pushed){
    	MultiThreading::ImageLoader::PushImageToQueue(&img1);
    	MultiThreading::ImageLoader::PushImageToQueue(&img2);
    	MultiThreading::ImageLoader::PushImageToQueue(&img3);
    	pushed=true;
    }
    MultiThreading::ImageLoader::LoadImages();
    ImageTexture::AsyncImage(&img1,ImVec2(362, 256));
    ImageTexture::AsyncImage(&img2,ImVec2(362, 256));
    ImageTexture::AsyncImage(&img3,ImVec2(362, 256));
    // ImageTexture::AsyncImage(imgs[1],ImVec2(362, 256));
    // ImageTexture::AsyncImage(imgs[2],ImVec2(362, 256));
    #endif


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
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) window_flags |= ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Container",nullptr, window_flags);
	ImGui::PopStyleVar(3);

	static Terminal terminal;


	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable){
        ImGuiID dockspace_id = ImGui::GetID("DDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

		static bool isFirst=true;
		if(isFirst){
			isFirst=false;
			ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
			ImGui::DockBuilderAddNode(dockspace_id,dockspace_flags | ImGuiDockNodeFlags_DockSpace );
			ImGui::DockBuilderSetNodeSize(dockspace_id, size);

			auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.3f, nullptr, &dockspace_id);
			// auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.7f, nullptr, &dockspace_id);
			ImGui::DockBuilderDockWindow("Project Directory", dock_id_left);
			ImGui::DockBuilderDockWindow("#editor_container", dockspace_id);
			// ImGui::DockBuilderDockWindow("#terminal", dockspace_id);
			#ifdef GL_DEBUG
			ImGui::DockBuilderDockWindow("Dear ImGui Demo", dock_id_left);
			ImGui::DockBuilderDockWindow("Project", dock_id_left);
			#endif
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
	terminal.Render();
	StatusBarManager::Render(size,viewport);

}



bool CoreSystem::Init(){return true;}

bool CoreSystem::InitImGui(){return true; }

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

	ImFontConfig font_config;
	font_config.FontDataOwnedByAtlas = false;
	const float font_size=GetFontSize();
	// io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf",font_size+3,&font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)JetBrainsMonoNLRegular, IM_ARRAYSIZE(JetBrainsMonoNLRegular), font_size+2, &font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)FontAwesomeSolid, IM_ARRAYSIZE(FontAwesomeSolid), (font_size+4.0f) * 2.0f / 3.0f, &icon_config, icons_ranges);

	// io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuii.ttf",font_size+3,&font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)JetBrainsMonoNLItalic, IM_ARRAYSIZE(JetBrainsMonoNLItalic), font_size+2, &font_config);

	io.Fonts->AddFontFromMemoryTTF((void*)MonoLisaRegular, IM_ARRAYSIZE(MonoLisaRegular), font_size-4.0f, &font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)FontAwesomeRegular, IM_ARRAYSIZE(FontAwesomeRegular), (font_size+4.0f) * 2.0f / 3.0f, &icon_config, icons_ranges);
}
