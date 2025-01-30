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
#include "DirectoryFinder.h"
#include "tree_sitter/api.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <winnt.h>

#include "HighlightType.h"
#include "LanguageConfigManager.h"


FileNavigation::FileNavigation(){};


FileNavigation::~FileNavigation(){ 
	Get().mDirectoryMonitor.Stop();
	Get().mDirectoryData.clear(); 
}

void FileNavigation::Init(){
	GL_INFO("FileNavigation::Init");
	LoadIconData("./assets/icons.json");
	Get().mDirectoryMonitor.Start();
	Get().mAreIconsLoaded=true;
}

std::string FileNavigation::GetFileTypeNameFromFilePath(const std::string& aFilePath)
{
	std::string extension=std::filesystem::path(aFilePath).extension().string();
	if(extension[0]=='.')
		extension=extension.substr(1);
	
	for(auto& [name,iconData]:Get().mIconDatabase)
	{
		if(std::find(iconData.extensions.begin(),iconData.extensions.end(),extension)!=iconData.extensions.end())
		{
			return iconData.name;
		}
	}

	return "Text (Unknown)";
}

void FileNavigation::AddFolder(std::string aPath)
{
	if(!Get().mAreIconsLoaded)
	{
		Init();
	}

	Get().mFolders.push_back(aPath);
	Get().mDirectoryMonitor.AddDirectoryToWatchList(StringToWString(aPath));
}

// Load JSON and parse icon data
void FileNavigation::LoadIconData(const std::string& aJsonPath){
    OpenGL::ScopedTimer timer("IconData Preparation");
    std::ifstream file(aJsonPath);
    nlohmann::json json;
    file >> json;

    auto& iconDatabase=Get().mIconDatabase;
	for (const auto& [key, value] : json.items()) 
	{
	    IconData& data=iconDatabase[key];

	    if (value.contains("extensions"))
	        data.extensions = value["extensions"].get<std::vector<std::string>>();
	    
	    if (value.contains("name"))
	        data.name = value["name"];

	    std::string filePath="./assets/icons/"+key+".png";
	    data.texture.SetPath(filePath);
	}
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
			if (!data.texture.IsLoaded())
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

    if (ImGui::BeginPopup(path.c_str()))
    {
	    const char* options[] = {ICON_FA_CARET_RIGHT"  Find In Folder",ICON_FA_CARET_RIGHT"  New File",ICON_FA_CARET_RIGHT"  Rename",ICON_FA_CARET_RIGHT"  Open Folder",ICON_FA_CARET_RIGHT"  Open Terminal",ICON_FA_CARET_RIGHT"  New Folder",ICON_FA_CARET_RIGHT"  Delete Folder"};
	    if(!isFolder)
	    {
	    	options[6]=ICON_FA_CARET_RIGHT"  Delete File";
	    	options[0]=ICON_FA_CARET_RIGHT"  Find in File";
	    }

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(6.0f,6.0f));
        for (int i = 0; i < IM_ARRAYSIZE(options); i++)
        {
        	if(i==4 || i==1) ImGui::Separator();
            if (ImGui::Selectable(options[i]))
            {
            	selected = i;
                std::string fpath=path;
            	switch(selected)
            	{
            		case 0:
            		{
            			if(isFolder){
            				DirectoryFinder::Setup(path,true);
            			}
            			else
            			{
            				StatusBarManager::ShowFileSearchPanel();
            			}
            		}
            		break;
                	case 1:
                	{
                		GL_INFO("New File:{}",path);
                		if(!std::filesystem::is_directory(fpath)) 
                			fpath=std::filesystem::path(fpath).parent_path().generic_string()+"/";


                		StatusBarManager::ShowInputPanel(
                			"Filename:",
                			[](const char* filePath){
	                			if(std::filesystem::is_directory(filePath)){
	                				StatusBarManager::ShowNotification("Failed",filePath,StatusBarManager::NotificationType::Error);
	                			}else{
		                			if(DirectoryHandler::CreateFile(filePath)){
		                				StatusBarManager::ShowNotification("Created",filePath,StatusBarManager::NotificationType::Success);
		                			}
	                			}
                			},
                			fpath.c_str()
                		);
                	}
                		break;
                	case 2:
                		GL_INFO("Rename");
                		{
	                		StatusBarManager::ShowInputPanelEx(
	                			isFolder ? "FolderName:" : "FileName:",
	                			[](const char* new_path,const char* old_path)
	                			{
	                				GL_INFO(new_path);
		                			if(std::filesystem::path(new_path).has_extension())
		                			{
		                				DirectoryHandler::RenameFile(old_path, new_path);
		                				StatusBarManager::ShowNotification("Renamed:",new_path,StatusBarManager::NotificationType::Success);
		                			}
		                			else
		                			{
			                			if(DirectoryHandler::RenameFolder(old_path, new_path))
			                			{
			                				StatusBarManager::ShowNotification("Renamed:",new_path,StatusBarManager::NotificationType::Success);
			                			}
		                			}
	                			},
	                			fpath.c_str()
	                		);
                		}
                		break;
                	case 3:
                		GL_INFO("Rename");
						if(!std::filesystem::is_directory(path))
							fpath = (std::filesystem::path(path)).parent_path().generic_string();
						DirectoryHandler::OpenExplorer(fpath);
                		break;
                	case 4:
                		GL_INFO("Open Folder");
						if(!std::filesystem::is_directory(path))
							fpath = (std::filesystem::path(path)).parent_path().generic_string();
						DirectoryHandler::StartTerminal(fpath);
                		break;
                	case 5:
                		GL_INFO("New Folder");
                		{
                			std::string parentDir=isFolder ? fpath: std::filesystem::path(fpath).parent_path().generic_string();
	                		StatusBarManager::ShowInputPanel(
	                			"FolderName:",
	                			[](const char* folderPath)
	                			{
	                				GL_INFO(folderPath);
		                			if(std::filesystem::path(folderPath).has_extension())
		                			{
		                				StatusBarManager::ShowNotification("Failed",folderPath,StatusBarManager::NotificationType::Error);
		                			}
		                			else
		                			{
			                			if(DirectoryHandler::CreateFolder(folderPath))
			                			{
			                				StatusBarManager::ShowNotification("Created",folderPath,StatusBarManager::NotificationType::Success);
			                			}
		                			}
	                			},
	                			parentDir.c_str()
	                		);

                		}
                		break;
                	case 6:
                		GL_INFO("Delete Folder");
                		if(!isFolder)
                		{ 
    	            		if(std::filesystem::exists(path)&&!std::filesystem::is_directory(path)){
    	            			if(DirectoryHandler::DeleteFile(path))
    	            				StatusBarManager::ShowNotification("Deleted", path.c_str());
    	            			else
    	            				StatusBarManager::ShowNotification("Failed Deletion", path.c_str(),StatusBarManager::NotificationType::Error);
    	            		}
                		}
                		else
                		{
    	            		if(std::filesystem::exists(path)&&std::filesystem::is_directory(path)){
    	            			if(DirectoryHandler::DeleteFolder(path))
    	            				StatusBarManager::ShowNotification("Deleted", path.c_str());
    	            			else
    	            				StatusBarManager::ShowNotification("Failed Deletion", path.c_str(),StatusBarManager::NotificationType::Error);
    	            		}

                		}
                		break;
            	}
            	ImGui::CloseCurrentPopup();
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
        col = IM_COL32(60, 60, 60, 255);
    else
        col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? IM_COL32(50, 50, 50, 255) : ImGuiCol_WindowBg);

	ImGui::RenderFrame(bb.Min, bb.Max, col, false, 0.0f);

	pos.x+=7.0f;
	float img_size=20.0f;
	ImVec2 img_min(pos.x,pos.y+((height-img_size)*0.5f));
	ImVec2 img_max(img_min.x+img_size,img_min.y+img_size);
	if(icondata->second.texture.IsLoaded())
		window->DrawList->AddImage((ImTextureID)(intptr_t)icondata->second.texture.GetTextureId(),img_min,img_max);

	const ImVec2 text_min(pos.x+25.0f,pos.y+(height-label_size.y)*0.5f);
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
			// ShowContextMenu(path,true);
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
		if(item.is_directory) 
		{
			const char* icon = item.is_explored ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER;
			oss << icon << " " << item.filename.c_str();
			if(ImGui::TreeNodeEx(oss.str().c_str(),ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Selected))
			{
				std::stringstream wss;
				if(ImGui::IsItemHovered(0))
				{
					Get().mCurrentEntity=&item;
					Get().mHoveringThisFrame=true;
				}
				// ShowContextMenu(item.path,item.is_directory); //Explored Folder
				RenderFolderItems(item.path,false);
				ImGui::TreePop();
			}
			else
			{
				if(ImGui::IsItemHovered(0)){
					Get().mCurrentEntity=&item;
					Get().mHoveringThisFrame=true;
				}
			}

			// ShowContextMenu(item.path,item.is_directory); // UnExplored Folder
			oss.str("");
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

				TabsManager::OpenFile(item.path);

			}
			if(ImGui::IsItemHovered(0))
			{
				Get().mCurrentEntity=&item;
				Get().mHoveringThisFrame=true;
			}
			ImGui::PopFont();
			// ShowContextMenu(item.path);
			
		}
	}
}


void FileNavigation::Render(){
	// static float s_width=250.0f;
	ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0.067f,0.075f,0.078f,1.000f));
	ImGui::PushStyleColor(ImGuiCol_Header,ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImGui::GetStyle().Colors[ImGuiCol_SeparatorHovered]);
	ImGui::SetNextWindowSize(ImVec2{250.0f,-1.0f},ImGuiCond_Once);
	ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.764,0.764,0.764,1.000));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.0f);
	if(ImGui::Begin("Project Directory"))
	{
		Get().mHoveringThisFrame=false;
    	auto& folders=GetFolders();
    	for(const auto& folder:folders)
    	{
    		Get().RenderFolderItems(folder,true);
    	}

    	if(Get().mCurrentEntity)
    	{
	    	Entity* entty=Get().mCurrentEntity;
	    	ShowContextMenu(entty->path,entty->is_directory);

    		if(Get().mHoveringThisFrame && ImGui::IsMouseDown(ImGuiMouseButton_Right))
    			ImGui::OpenPopup(entty->path.c_str());
    	}
	}


	ImGui::End();
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar();
}


void FileNavigation::MarkFileAsOpen(const std::string &aOpenedFilePath){
	std::string	directoryPath=std::filesystem::path(aOpenedFilePath).parent_path().generic_string();

	// scan if parent dir not in mDirectoryData
	if(Get().mDirectoryData.find(directoryPath)==Get().mDirectoryData.end())
		ScanDirectory(directoryPath);

	auto& entities=Get().mDirectoryData[directoryPath];

	if(entities.empty()) return;

	for(auto& entity:entities)
		if(!entity.is_directory && entity.path==aOpenedFilePath)
			entity.is_explored=true;
		else
			entity.is_explored=false;
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


void FileNavigation::HandleEvent(DirectoryEvent aEvent,std::wstring& aPayLoad){

	std::filesystem::path path=std::filesystem::path(aPayLoad);
	GL_WARN("Event:{}, PayLoad:{}",(int)aEvent,ToUTF8(aPayLoad));
	if(path.has_extension() && path.extension().generic_string()==".scm"){
		auto type=TxEdit::GetHighlightType("main.lua");
		LanguageConfig* config=LanguageConfigManager::GetLanguageConfig(type);
		if (!config)
			return;
		ts_query_delete(config->pQuery);
		config->pQuery=nullptr;

		std::string queryString;
		if(LanguageConfigManager::LoadLanguageQuery(type, queryString)){
			config->pQueryString=queryString;
		}
		return;
	}
	std::string folderPath=std::filesystem::path(aPayLoad).parent_path().generic_u8string();

	switch(aEvent){
	case DirectoryEvent::FileAdded:
		break;
	case DirectoryEvent::FileRemoved:
		break;
	case DirectoryEvent::FileRenamedOldName:
		break;
	case DirectoryEvent::FileRenamedNewName:
		ScanDirectory(folderPath);
		break;
	case DirectoryEvent::FileModified:
		break;
	}
}
