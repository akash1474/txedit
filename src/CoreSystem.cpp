#include "pch.h"
#include "CoreSystem.h"



#ifdef GL_DEBUG

void CoreSystem::RenderDebugInfo(){
	static bool show_demo=true;
	ImGui::ShowDemoWindow(&show_demo);


	ImGui::Begin("Project");

	static int LineSpacing = 10.0f;
	if (ImGui::SliderInt("LineSpacing", &LineSpacing, 0, 20)) {
		this->mTextEditor.setLineSpacing(LineSpacing);
	}

    ImGui::Spacing();
	ImGui::Text("PositionY:%.2f", ImGui::GetMousePos().y);
    ImGui::Spacing();
    ImGui::Text("mCursorPosition: X:%d  Y:%d",this->mTextEditor.GetEditorState()->mCursorPosition.mColumn,this->mTextEditor.GetEditorState()->mCursorPosition.mLine);
    ImGui::Text("mSelectionStart: X:%d  Y:%d",this->mTextEditor.GetEditorState()->mSelectionStart.mColumn,this->mTextEditor.GetEditorState()->mSelectionStart.mLine);
    ImGui::Text("mSelectionEnd:   X:%d  Y:%d",this->mTextEditor.GetEditorState()->mSelectionEnd.mColumn,this->mTextEditor.GetEditorState()->mSelectionEnd.mLine);


    ImGui::Spacing();
    ImGui::Spacing();
    static std::string mode;
    ImColor color;
    switch(this->mTextEditor.GetSelectionMode()){
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
        this->mTextEditor.ScrollToLineNumber(v);

    if(ImGui::Button("Select File")) SelectFile();

    if(ImGui::Button("Select Files")){
    	auto files=SelectFiles();
    	for(auto& file:files)
    		std::wcout << file << std::endl;
    }

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
				this->mTextEditor.LoadFile(SelectFile().c_str());

			if(ImGui::MenuItem("Open Folder")){
				std::string path=SelectFolder();
				if(!path.empty()) this->mFileNavigation.AddFolder(path.c_str());
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
	this->RenderDebugInfo();
	#endif

	if(mFileNavigation.IsOpen()) mFileNavigation.Render();

	mTextEditor.Render();
	
	StatusBarManager::Render(size,viewport);

}


