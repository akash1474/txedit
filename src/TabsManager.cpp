#include "pch.h"
#include "FontAwesome6.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdint>

#include "TextEditor.h"
#include "FileNavigation.h"
#include "TabsManager.h"
#include "StatusBarManager.h"
// #include "uuid_v4.h"

Editor* TabsManager::GetCurrentActiveTextEditor(){
	Editor* editor=nullptr;
	for(auto& tab:Get().mTabs)
		if(tab.isActive)
			editor=tab.editor;

	return editor;
}

FileTab* TabsManager::GetCurrentActiveTab()
{
	FileTab* rTab=nullptr;
	for(auto& tab:Get().mTabs)
		if(tab.isActive)
			rTab=&tab;


	return rTab;
}

TabsManager::~TabsManager(){
	auto& aTabs=mTabs;
	for(auto& tab:aTabs)
		free(tab.editor);
}

TabsManager::TabsManager(){
	if(!FileNavigation::AreIconsLoaded())
		FileNavigation::Init();
}

void TabsManager::SetNewTabsDockSpaceId(ImGuiID aDockSpaceId){
	Get().mDockSpaceId=aDockSpaceId;
}

bool TabsManager::OpenNewEmptyFile(){
	return OpenFile("",true);
}

void TabsManager::OpenFileWithAtLineNumber(const std::string& aFilePath,int aLineNumber){
	OpenFile(aFilePath);
	Get().mLineNumberToScroll=aLineNumber;
}

bool TabsManager::OpenFile(std::string aFilePath,bool aIsTemp)
{
	GL_INFO("Opening File:{}",aFilePath);
	// static UUIDv4::UUIDGenerator<std::mt19937_64> mUIDGenerator;
	// UUIDv4::UUID uuid = mUIDGenerator.getUUID();
	std::filesystem::path path(aFilePath);
	std::string uuid=path.filename().generic_u8string() + "##" + std::to_string((int)&Get());
	GL_INFO(uuid);

	if(aFilePath.empty())
	{
		Get().mTabs.emplace_back(aFilePath,"Untitled",aIsTemp,true,false,"Untitled##"+std::to_string((int)&Get()));
		FileTab& aTab=Get().mTabs.back();
		aTab.editor=new Editor();
		aTab.editor->LoadFile(aFilePath.c_str());
		return true;
	}


	auto it=std::find_if(
		Get().mTabs.begin(),
		Get().mTabs.end(),
		[&](const FileTab& tab)
		{
			return tab.filepath==aFilePath;
		}
	);

	//Adding new tab
	if(it==Get().mTabs.end())
	{
		for(auto&tab:Get().mTabs) 
			tab.isActive=false;

		//Replace the temp file with curr temp file if a temp file is found
		// auto it=std::find_if(Get().mTabs.begin(),Get().mTabs.end(),[&](const FileTab& tab){return tab.isTemp;});
		// if(it!=Get().mTabs.end())
		// {
		// 	it->filepath=aFilePath;
		// 	it->filename=path.filename().generic_u8string();
		// }
		// else
		// {
			GL_INFO("Added:{}",aFilePath);
			Get().mTabs.emplace_back(aFilePath,path.filename().generic_u8string(),aIsTemp,true,true,uuid);
			FileTab& aTab=Get().mTabs.back();
			aTab.editor=new Editor();
			aTab.editor->LoadFile(aFilePath.c_str());
		// }

	}
	else //Reusing/Reactivating the previous one
	{
		GL_INFO("ReActivating:{}",aFilePath);
		for(auto&tab:Get().mTabs) 
			tab.isActive=false;

		it->isActive=true;
		it->isTemp=false;
		ImGui::FocusWindow(it->winPtr);
	}



	return true;
}


void TabsManager::Render(){
	std::vector<FileTab>& tabs=Get().mTabs;
	bool removeTab=false;
	for(auto it=tabs.begin();it!=tabs.end();)
	{

		ImGui::SetNextWindowDockID(Get().mDockSpaceId, ImGuiCond_FirstUseEver);

		//Rendering the editor and updating the active ptr
		if(it->editor->Render(&it->isOpen,it->id,it->isTemp) && !it->isActive){
			for(auto&tab:Get().mTabs) 
				tab.isActive=false;

			it->isActive=true;
		}

		//Updating the windowptr
		if(it->winPtr==nullptr)
		{
			it->winPtr=it->editor->GetImGuiWindowPtr();
		}

		// if(ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::GetIO().MouseDoubleClicked[0])
		// {
		// 	it->isTemp=false;
		// 	ImGui::GetIO().MouseDoubleClicked[0]=0;
		// }

		// if(ImGui::IsItemHovered() && ImGui::IsItemClicked(ImGuiMouseButton_Right)) 
		// 	ImGui::OpenPopup("##tab_menu");
		
		if(!it->isOpen)
		{
			bool wasDetetedTabFocused=it->isActive;
			it=tabs.erase(it);
			std::vector<FileTab>::iterator currTab=it;

			if(currTab==tabs.end() && currTab!=tabs.begin())
			{
				currTab=it-1;
				currTab->isActive=true;
			}

			if(wasDetetedTabFocused)
				ImGui::SetNextWindowFocus();
		}
		else
		{
			it++;
		}

		// ImGui::SameLine(0.0f,0.0f);

	}

	ImGuiIO& io = ImGui::GetIO();

	if (io.KeyCtrl && !io.KeyShift && !io.KeyAlt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_S)))
		SaveFile();
	else if (io.KeyCtrl && !io.KeyShift && !io.KeyAlt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_N)))
		OpenFile("",true);

	// static const char* names[] = { 
	// 	"Close Tabs to the Right", 
	// 	"Close UnModified Tabs", 
	// 	"Close UnModified Tabs to Right", 
	// 	"Close Tabs with Deleted Files" 
	// };

	// bool selected=-1;
	// ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 5.0f));
	// if(ImGui::BeginPopup("##tab_menu"))
	// {
	// 	if(ImGui::Selectable("Close Tab"))
	// 	{
	// 		// ImGui::GetHoveredID()
	// 	}

	// 	ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
    //     for (int i = 0; i < IM_ARRAYSIZE(names); i++)
    //         if (ImGui::Selectable(names[i]))
    //             selected = i;
		
	// 	ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
	// 	ImGui::Selectable("New File");
	// 	ImGui::Selectable("Open File");
	// 	ImGui::EndPopup();
	// }
	// ImGui::PopStyleVar();
	if(Get().mLineNumberToScroll>-1)
	{
		GetCurrentActiveTextEditor()->ScrollToLineNumber(Get().mLineNumberToScroll);
		Get().mLineNumberToScroll=-1;
	}

}


void TabsManager::SaveFile()
{
	FileTab* currTab=GetCurrentActiveTab();
	std::string textContent=currTab->editor->GetText();

	size_t size=textContent.size()-1;
	if(size>0 && textContent[size-1] == textContent[size])
		textContent.pop_back();

	if(currTab->filepath.empty())
	{
		std::string savePath=SaveFileAs(textContent);

		if(!savePath.empty())
		{
			currTab->isOpen=false;
			OpenFile(savePath);
		}
	}
	else
	{
		std::ofstream file(currTab->filepath, std::ios::trunc);
		if (!file.is_open()) 
		{
			GL_INFO("ERROR SAVING");
			return;
		}

		file << textContent;
		file.close();
		StatusBarManager::ShowNotification("Saved",currTab->filepath.c_str(), StatusBarManager::NotificationType::Success);
		currTab->editor->SetIsBufferModified(false);
	}
}

void TabsManager::DisableSearchForAllTabs(){
	for(auto& aTab:Get().mTabs)
	{
		aTab.editor->DisableSearch();
	}
}