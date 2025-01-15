#include "pch.h"
#include "ImageTexture.h"
#include "Log.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "FileNavigation.h"
#include "DirectoryHandler.h"
#include "StatusBarManager.h"
#include "TabsManager.h"
#include <filesystem>
#include "MultiThreading.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <winnt.h>


FileNavigation::FileNavigation(){
	mIconDatabase=LoadIconData("./assets/icons.json");
	mDirectoryMonitor.Start();
};


FileNavigation::~FileNavigation(){ 
	mDirectoryMonitor.Stop();
	mDirectoryData.clear(); 
}


void FileNavigation::AddFolder(std::string aPath)
{
	Get().mFolders.push_back(aPath);
	Get().mDirectoryMonitor.AddDirectoryToWatchList(StringToWString(aPath));
}

// Load JSON and parse icon data
std::unordered_map<std::string, IconData> FileNavigation::LoadIconData(const std::string& aJsonPath){
    OpenGL::ScopedTimer timer("IconData Preparation");
    std::ifstream file(aJsonPath);
    nlohmann::json json;
    file >> json;

    std::unordered_map<std::string, IconData> icons;
	for (const auto& [key, value] : json.items()) {
	    IconData data;
	    if (value.contains("extensions")) {
	        data.extensions = value["extensions"].get<std::vector<std::string>>();
	    }
	    if (value.contains("name")) {
	        data.name = value["name"];
	    }
	    std::string filePath="./assets/icons/"+key+".png";
	    icons[key] = {data.name,data.extensions};
	    icons[key].texture.SetPath(filePath);
	}

    return std::move(icons);
}

// Match file extensions
std::pair<const std::string,IconData>* FileNavigation::GetIconForExtension(const std::string& aExtension){
    // OpenGL::ScopedTimer timer("IconData Search");
    std::pair<const std::string,IconData>* defaultIcon=nullptr;

    for (auto& element: Get().mIconDatabase) 
    {
    	auto& data=element.second;
    	auto& type=element.first;

    	//storing a ptr so that I can return in case no match found.
    	if(type=="file_type_default")
    		defaultIcon=&element;

        if (std::find(data.extensions.begin(), data.extensions.end(), aExtension) != data.extensions.end()) 
        {
        	if(!data.texture.IsLoaded())
				MultiThreading::ImageLoader::PushImageToQueue(&data.texture);
            
            return &element;
        }
    }

    if(!defaultIcon->second.texture.IsLoaded())
		MultiThreading::ImageLoader::PushImageToQueue(&defaultIcon->second.texture);

    return defaultIcon;
}


void FileNavigation::ShowContextMenu(std::string& path,bool isFolder){
    static int selected=-1;

    if (ImGui::BeginPopupContextItem(path.c_str()))
    {
	    const char* options[] = {ICON_FA_CARET_RIGHT"  New File",ICON_FA_CARET_RIGHT"  Rename",ICON_FA_CARET_RIGHT"  Open Folder",ICON_FA_CARET_RIGHT"  Open Terminal",ICON_FA_CARET_RIGHT"  New Folder",ICON_FA_CARET_RIGHT"  Delete Folder"};
	    if(!isFolder)
	    	options[5]=ICON_FA_CARET_RIGHT"  Delete File";

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(6.0f,8.0f));
        for (int i = 0; i < IM_ARRAYSIZE(options); i++){
        	if(i==4) ImGui::Separator();
            if (ImGui::Selectable(options[i])){
            	selected = i;
                std::string fpath=path;
            	switch(selected){
                	case 0:{
                		GL_INFO("New File:{}",path);
                		if(!std::filesystem::is_directory(fpath)) fpath=std::filesystem::path(fpath).parent_path().generic_string();
                		StatusBarManager::ShowInputPanel(
                			"Filename:",
                			[](const char* filePath){
	                			if(std::filesystem::is_directory(filePath)){
	                				StatusBarManager::ShowNotification("Failed",filePath,StatusBarManager::NotificationType::Error);
	                			}else{
		                			if(DirectoryHandler::CreateFile(filePath)){
		                				StatusBarManager::ShowNotification("Created",filePath,StatusBarManager::NotificationType::Success);

		                				std::string dir=std::filesystem::path(filePath).parent_path().generic_string();
		                				FileNavigation::ScanDirectory(dir);
		                			}
	                			}
                			},
                			fpath.c_str()
                		);
                	}
                		break;
                	case 1:
                		GL_INFO("Rename");
                		break;
                	case 2:
						if(!std::filesystem::is_directory(path))
							fpath = (std::filesystem::path(path)).parent_path().generic_string();
						DirectoryHandler::OpenExplorer(fpath);
                		break;
                	case 3:
						if(!std::filesystem::is_directory(path))
							fpath = (std::filesystem::path(path)).parent_path().generic_string();
						DirectoryHandler::StartTerminal(fpath);
                		break;
                	case 4:
                		GL_INFO("New Folder");
                		break;
                	case 5:
                		GL_INFO("Delete Folder");
                		if(!isFolder){ //File
    	            		if(std::filesystem::exists(path)&&!std::filesystem::is_directory(path)){
    	            			if(DirectoryHandler::DeleteFile(path)){
    	            				StatusBarManager::ShowNotification("Deleted", path.c_str());
    	            				FileNavigation::ScanDirectory(std::filesystem::path(path).parent_path().generic_string());
    	            			}
    	            			else
    	            				StatusBarManager::ShowNotification("Failed Deletion", path.c_str(),StatusBarManager::NotificationType::Error);
    	            		}
                		}else{
    	            		if(std::filesystem::exists(path)&&std::filesystem::is_directory(path)){
    	            			// if(DirectoryHandler::DeleteFolder(path)){
    	            				StatusBarManager::ShowNotification("Deleted", path.c_str());
    	            			// 	StatusBarManager::GetFileNavigation()->ScanDirectory(std::filesystem::path(path).parent_path().generic_string());
    	            			// }
    	            			// else
    	            			// 	StatusBarManager::ShowNotification("Failed Deletion", path.c_str(),StatusBarManager::NotificationType::Error);
    	            		}

                		}
                		break;
            	}

            }
        }
        ImGui::PopStyleVar();
        ImGui::EndPopup();
    }
}

bool FileNavigation::CustomSelectable(std::string& aFileName,bool aIsSelected)
{

	std::string ext=std::filesystem::path(aFileName).extension().generic_string();
	if(!ext.empty())
		ext.erase(ext.begin());
	else
	{
		ext=std::filesystem::path(aFileName).filename().generic_string();
		ext.erase(ext.begin());
	}

	std::pair<const std::string,IconData>* icondata=Get().GetIconForExtension(ext);


	//Settingup custom component
	ImGuiWindow* window=ImGui::GetCurrentWindow();
	if(window->SkipItems) return false;

	const ImGuiID id=window->GetID(aFileName.c_str());
	const ImVec2 label_size=ImGui::CalcTextSize(aFileName.c_str());

	const ImVec2 itemSize(window->WorkRect.Max.x-window->WorkRect.Min.x,label_size.y+2.0f+(2*ImGui::GetStyle().FramePadding.y));
	ImVec2 pos=window->DC.CursorPos;


	const ImRect bb(pos,ImVec2(pos.x+itemSize.x,pos.y+itemSize.y));
	const float height=bb.Max.y-bb.Min.y;


	ImGui::ItemSize(bb,0.0f);
	if(!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);


    ImU32 col;
    if (aIsSelected && !hovered)
        col = IM_COL32(50, 50, 50, 255);
    else
        col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? IM_COL32(40, 40, 40, 255) : ImGuiCol_WindowBg);

	ImGui::RenderFrame(bb.Min, bb.Max, col, false, 0.0f);

	pos.x+=7.0f;
	float img_size=20.0f;
	ImVec2 img_min(pos.x,pos.y+((height-img_size)*0.5f));
	ImVec2 img_max(img_min.x+img_size,img_min.y+img_size);
	if(icondata->second.texture.IsLoaded())
		window->DrawList->AddImage((void*)(intptr_t)icondata->second.texture.GetTextureId(),img_min,img_max);

	const ImVec2 text_min(pos.x+25.0f,pos.y+ImGui::GetStyle().FramePadding.y);
    const ImVec2 text_max(text_min.x + label_size.x, text_min.y + label_size.y);
	ImGui::RenderTextClipped(text_min,text_max,aFileName.c_str(),0,&label_size, ImGui::GetStyle().SelectableTextAlign, &bb);


	return pressed;
}

void FileNavigation::RenderFolderItems(std::string path,bool isRoot)
{
	if(isRoot)
	{
		std::string folderName=std::filesystem::path(path).filename().generic_string();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(6.0f,2.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(4.0f,2.0f));

		if(ImGui::TreeNodeEx(folderName.c_str(),ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen))
		{
    		RenderFolderItems(path);
    		ImGui::TreePop();
    	}

		ShowContextMenu(path,true);
    	ImGui::PopStyleVar(2);
    	return;
	}

	// Folder not present in mDirectoryData
	if(Get().mDirectoryData.empty()  || Get().mDirectoryData.find(path)==Get().mDirectoryData.end())
	{
		ScanDirectory(path);
	}


	auto& entities=Get().mDirectoryData[path];

	if(entities.empty()) 
		return;
	
	std::stringstream oss;

	for(Entity& item:entities)
	{
		const char* icon = item.is_directory ? item.is_explored ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER : ICON_FA_FILE;
		oss << icon << " " << item.filename.c_str();
		if(item.is_directory) 
		{
			if(ImGui::TreeNodeEx(oss.str().c_str(),ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Selected))
			{
				std::stringstream wss;
				ShowContextMenu(item.path,item.is_directory); //Explored Folder
				RenderFolderItems(item.path,false);
				ImGui::TreePop();
			}

			ShowContextMenu(item.path,item.is_directory); // UnExplored Folder
		}
		else
		{
			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

			if(CustomSelectable(item.filename,item.is_explored))
			{
				for(Entity& en:entities) 
					en.is_explored=false;
				
				item.is_explored=true;

				if(!std::filesystem::exists(item.path))
					ScanDirectory(std::filesystem::path(item.path).parent_path().generic_string());

				TabsManager::Get().OpenFile(item.path);

			}
			ImGui::PopFont();
			ShowContextMenu(item.path);
			
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
    	{
    		Get().RenderFolderItems(folder,true);
    	}

	ImGui::End();
	ImGui::PopStyleColor(2);
}


void FileNavigation::ScanDirectory(const std::string& aDirectoryPath){
	GL_INFO("Updating:{}",aDirectoryPath);

	auto& entities=Get().mDirectoryData[aDirectoryPath];
	entities.clear();

	try
	{
		for(const auto& entity:std::filesystem::directory_iterator(aDirectoryPath))
		{
			if(!entity.is_directory())
			{
				//Ignoring the binary type files that are not viewable in editors
				std::string ext=entity.path().extension().generic_string();
				if(ext==".exe" || ext==".obj" || ext==".dll" || ext==".lib" || ext==".so" || ext==".o" || ext==".pdb")
					continue;
			}

			entities.push_back({
				entity.path().filename().generic_u8string(),
				entity.path().generic_string(),
				entity.is_directory(),
				false
			});
		}
	}
	catch(...)
	{
		GL_CRITICAL("ERROR:Directory Scan");
	}

	std::stable_partition(entities.begin(), entities.end(), [](const auto& entity) { return entity.is_directory; });
}
