#include "pch.h"
#include "FileNavigation.h"
#include "DirectoryHandler.h"

void FileNavigation::ShowContextMenu(std::string& path,std::string& name,bool isFolder){
    static int selected=-1;

    if (ImGui::BeginPopupContextItem(name.c_str()))
    {
	    const char* options[] = {ICON_FA_CARET_RIGHT"  New File",ICON_FA_CARET_RIGHT"  Rename",ICON_FA_CARET_RIGHT"  Open Folder",ICON_FA_CARET_RIGHT"  Open Terminal",ICON_FA_CARET_RIGHT"  New Folder",ICON_FA_CARET_RIGHT"  Delete Folder"};
	    if(!isFolder){
	    	options[5]=ICON_FA_CARET_RIGHT"  Delete File";
	    }else{
	    	options[5]=ICON_FA_CARET_RIGHT"  Delete Folder";
	    }
        for (int i = 0; i < IM_ARRAYSIZE(options); i++){
        	if(i==4) ImGui::Separator();
            if (ImGui::Selectable(options[i])){
            	selected = i;
                std::string fpath=path;
            	switch(selected){
                	case 0:
                		GL_INFO("New File");
                		GL_INFO("{}/{}",path,name);
                		break;
                	case 1:
                		GL_INFO("Rename");
                		break;
                	case 2:
						if(std::filesystem::is_directory(path))
							fpath = (std::filesystem::path(path)/name).generic_string();
						DirectoryHandler::OpenExplorer(fpath);
                		break;
                	case 3:
						if(std::filesystem::is_directory(path))
							fpath = (std::filesystem::path(path)/name).generic_string();
						DirectoryHandler::StartTerminal(fpath);
                		break;
                	case 4:
                		GL_INFO("New Folder");
                		break;
                	case 5:
                		GL_INFO("Delete Folder");
                		break;
            	}

            }
        }
        ImGui::EndPopup();
    }
}

void FileNavigation::RenderFolderItems(std::string path,bool isRoot){
	if(isRoot){
		std::string folderName=std::filesystem::path(path).filename().generic_string();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(6.0f,2.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(4.0f,2.0f));
		if(ImGui::TreeNodeEx(folderName.c_str(),ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen)){
    		RenderFolderItems(path);
    		ImGui::TreePop();
    	}
		ShowContextMenu(path,folderName);
    	ImGui::PopStyleVar(2);
    	return;
	}
	if(mDirectoryData.empty()  || mDirectoryData.find(path)==mDirectoryData.end()){
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
				ShowContextMenu(path,item.filename); //Explored Folder
				wss << path << "\\" << item.filename.c_str();
				RenderFolderItems(wss.str(),false);
				ImGui::TreePop();
			}
			ShowContextMenu(path,item.filename); // UnExplored Folder
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
				this->mTextEditor->LoadFile(item.path.c_str());
			}
			ImGui::PopFont();
			ShowContextMenu(path,item.filename,false);
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(2);
		}
		oss.str("");
	}
}


void FileNavigation::Render(){
	// static float s_width=250.0f;
	ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0.067f,0.075f,0.078f,1.000f));
	ImGui::SetNextWindowSize(ImVec2{250.0f,-1.0f},ImGuiCond_Once);
	ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.764,0.764,0.764,1.000));
	ImGui::Begin("Project Directory");
    	auto& folders=GetFolders();
    	for(const auto& folder:folders)
    		RenderFolderItems(folder,true);
	ImGui::End();
	ImGui::PopStyleColor(2);
}