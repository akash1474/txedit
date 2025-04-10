#include "pch.h"

#include <commctrl.h>
#include <filesystem>

#include "fs/DirectoryMonitor.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "core/Log.h"
#include "core/utils.h"

#include "editor/Trie.h"
#include "editor/TextEditor.h"

#include "ui/FileNavigation.h"
#include "ui/TabsManager.h"
#include "ui/StatusBarManager.h"


Editor* TabsManager::GetCurrentActiveTextEditor(){
	Editor* editor=nullptr;
	for(auto& tab:Get().mTabs)
		if(tab.isActive)
			editor=tab.editor;

	return editor;
}

FileTab* TabsManager::GetTabWithFileName(const std::string& aFileName){
	FileTab* rTab=nullptr;
	for(auto& tab:Get().mTabs)
		if(tab.filename==aFileName)
			rTab=&tab;

	return rTab;
}

FileTab* TabsManager::GetCurrentActiveTab()
{
	FileTab* rTab=nullptr;
	for(auto& tab:Get().mTabs)
		if(tab.isActive)
			rTab=&tab;


	return rTab;
}

Trie::Node* TabsManager::GetTrieRootNode(){
	if(Get().mTokenSuggestionsRoot)
		return Get().mTokenSuggestionsRoot;
	
	Get().mTokenSuggestionsRoot=new Trie::Node();
	return Get().mTokenSuggestionsRoot;
}

TabsManager::~TabsManager(){
	auto& aTabs=mTabs;
	for(auto& tab:aTabs)
		free(tab.editor);

	if (mTokenSuggestionsRoot)
		Trie::Free(mTokenSuggestionsRoot);
}

TabsManager::TabsManager(){
	if(!FileNavigation::AreIconsLoaded())
		FileNavigation::Init();
}

void TabsManager::SetNewTabsDockSpaceId(ImGuiID aDockSpaceId){
	Get().mDockSpaceId=aDockSpaceId;
}

bool TabsManager::OpenNewEmptyFile(){
	return OpenTabWithFilePath("",true);
}

void TabsManager::OpenFileWithAtLineNumber(const std::string& aFilePath,int aLineNumber,int aStartIndex,int aEndIndex){
	FileTab* openedTab=OpenTabWithFilePath(aFilePath);
	if(openedTab)
	{
		Get().mLineNumberToScroll=aLineNumber;
		openedTab->editor->CreateHighlight(aLineNumber, aStartIndex, aEndIndex);
	}
}

void TabsManager::InitializeTabFromCache(std::string aWindowId,std::string aFilePath){
	if(!std::filesystem::exists(aFilePath))
	{
		GL_CRITICAL("TabsManager::OpenTabWithFilePath::Failed - Path doesn't exist - {}",aFilePath);
		return;
	}
	GL_INFO("Opening File:{}",aFilePath);
	std::filesystem::path path(aFilePath);
	for(auto&tab:Get().mTabs) 
		tab.isActive=false;

	Get().mTabs.emplace_back(aFilePath,path.filename().generic_u8string(),false,true,true,aWindowId);
	FileTab& aTab=Get().mTabs.back();
	aTab.editor=new Editor();
	aTab.editor->LoadFile(aFilePath.c_str());
}

FileTab* TabsManager::OpenTabWithFilePath(std::string aFilePath,bool aIsTemp)
{
	if(!aFilePath.empty() && !std::filesystem::exists(aFilePath))
	{
		GL_CRITICAL("TabsManager::OpenTabWithFilePath::Failed - Path doesn't exist - {}",aFilePath);
		return nullptr;
	}
	GL_INFO("Opening File:{}",aFilePath);

	std::filesystem::path path(aFilePath);
	auto now = std::chrono::steady_clock::now().time_since_epoch().count();
	const std::string uuid=GetUIDWithBase(path.filename().generic_u8string());
	GL_INFO(uuid);

	if(aFilePath.empty())
	{
		Get().mTabs.emplace_back(aFilePath,"Untitled",aIsTemp,true,false,"Untitled##"+std::to_string(now));
		FileTab& aTab=Get().mTabs.back();
		aTab.editor=new Editor();
		aTab.editor->LoadFile(aFilePath.c_str());
		return &aTab;
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
			Get().mTabs.emplace_back(
				aFilePath,path.filename().generic_u8string(), //filePath
				aIsTemp, //isTemp
				true, // isActive
				true, // isSaved
				uuid //uid
			);
			FileTab& aTab=Get().mTabs.back();
			aTab.editor=new Editor();
			aTab.editor->LoadFile(aFilePath.c_str());
		// }
		return &aTab;

	}
	else //Reusing/Reactivating the previous one
	{
		GL_INFO("ReActivating:{}",aFilePath);
		for(auto&tab:Get().mTabs) 
			tab.isActive=false;

		it->isActive=true;
		it->isTemp=false;
		ImGui::FocusWindow(it->winPtr);
		return &(*it);
	}



	return nullptr;
}

int ShowSavePrompt(HWND hwnd, const wchar_t* filename)
{
    TASKDIALOGCONFIG config = { sizeof(config) };
    config.hwndParent = hwnd;
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
    config.dwCommonButtons = 0;
    config.pszWindowTitle = L"TxEdit";
    config.pszMainIcon = TD_WARNING_ICON;

    std::wstring mainInstruction = L"Do you want to save the changes you made to ";
    mainInstruction += filename;
    mainInstruction += L"?";

    config.pszMainInstruction = mainInstruction.c_str();
    config.pszContent = L"Your changes will be lost if you don't save them.";

    TASKDIALOG_BUTTON buttons[] = {
        { 1001, L"&Save" },
        { 1002, L"&Don't Save" },
        { 1003, L"&Cancel" }
    };
    config.pButtons = buttons;
    config.cButtons = ARRAYSIZE(buttons);

    int buttonPressed = 0;
    TaskDialogIndirect(&config, &buttonPressed, nullptr, nullptr);

    return buttonPressed;
}


void TabsManager::Render(){
	std::vector<FileTab>& tabs=Get().mTabs;
	FileTab* aRemovedTab=nullptr;	
	for(auto it=tabs.begin();it!=tabs.end();it++)
	{

		ImGui::SetNextWindowDockID(Get().mDockSpaceId, ImGuiCond_FirstUseEver);

		//Rendering the editor and updating the active ptr
		if(it->editor->Render(&it->isOpen,it->id,it->isTemp) && !it->isActive){
			for(auto&tab:Get().mTabs) 
				tab.isActive=false;

			it->isActive=true;
			FileNavigation::MarkFileAsOpen(it->filepath);
		}

		//Updating the windowptr
		if(it->winPtr==nullptr)
		{
			it->winPtr=it->editor->GetImGuiWindowPtr();
		}

		if(!it->isOpen)
			aRemovedTab=&(*it);

		// if(ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::GetIO().MouseDoubleClicked[0])
		// {
		// 	it->isTemp=false;
		// 	ImGui::GetIO().MouseDoubleClicked[0]=0;
		// }

		// if(ImGui::IsItemHovered() && ImGui::IsItemClicked(ImGuiMouseButton_Right)) 
		// 	ImGui::OpenPopup("##tab_menu");
		
		// if(!it->isOpen)
		// 	CloseTab(&(*it));
		// else
			// it++;

		// ImGui::SameLine(0.0f,0.0f);

	}
	if(ImGui::IsKeyDown(ImGuiKey_ModCtrl) && ImGui::IsKeyPressed(ImGuiKey_W)){
		aRemovedTab=GetCurrentActiveTab();
	}


	if(aRemovedTab){
		if(aRemovedTab->editor->IsBufferModified())
		{
		    int result = ShowSavePrompt(nullptr, StringToWString(aRemovedTab->filename).c_str());

		    switch (result)
		    {
		    case 1001:
		    	GL_INFO("Save");
		    	SaveFile(aRemovedTab);
		    case 1002:
		    	CloseTab(aRemovedTab);
		    	break;
		    case 1003:
		    	GL_INFO("Cancel"); 
		    	break;
		    }
		}
		else
			CloseTab(aRemovedTab);
	}

	ImGuiIO& io = ImGui::GetIO();

	if (io.KeyCtrl && !io.KeyShift && !io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_S))
		SaveFile();
	else if (io.KeyCtrl && !io.KeyShift && !io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_N))
		OpenTabWithFilePath("",true);
	if(ImGui::IsKeyPressed(ImGuiKey_Escape) && GetCurrentActiveTextEditor())
		GetCurrentActiveTextEditor()->ClearSuggestions();

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
    // Always center this window when appearing



    // ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    // ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    // if (ImGui::BeginPopupModal("Unsaved Changes", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    // {
    //     ImGui::Text("The file has been modified.\nDo you want to save your changes?");
    //     ImGui::Separator();

    //     if (ImGui::Button("Save", ImVec2(120, 0))) { 
    //     	GL_INFO("Tab Closed");

    //     	ImGui::CloseCurrentPopup(); 
    //     }
    //     ImGui::SetItemDefaultFocus();
    //     ImGui::SameLine();
    //     if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
    //     ImGui::EndPopup();
    // }


	if(Get().mLineNumberToScroll>-1)
	{
		GetCurrentActiveTextEditor()->ScrollToLineNumber(Get().mLineNumberToScroll);
		Get().mLineNumberToScroll=-1;
	}

}


void TabsManager::SaveFile(FileTab* aCurrentFileTab)
{
	if(!aCurrentFileTab)
		aCurrentFileTab=GetCurrentActiveTab();

	if(!aCurrentFileTab) return;
	std::string textContent=aCurrentFileTab->editor->GetText();

	size_t size=textContent.size()-1;
	if(size>0 && textContent[size-1] == textContent[size])
		textContent.pop_back();

	if(aCurrentFileTab->filepath.empty())
	{
		std::string savePath=SaveFileAs(textContent);

		if(!savePath.empty())
		{
			GL_INFO("SaveAs:{}",savePath);
			CloseTab(aCurrentFileTab);
			OpenTabWithFilePath(savePath);
			FileNavigation::MarkFileAsOpen(aCurrentFileTab->filepath);
		}
	}
	else
	{
		std::ofstream file(aCurrentFileTab->filepath, std::ios::trunc);
		if (!file.is_open()) 
		{
			GL_INFO("ERROR SAVING");
			return;
		}
		DirectoryMonitor::RegisterFileModification(StringToWString(aCurrentFileTab->filepath));
		file << textContent;
		file.close();
		StatusBarManager::ShowNotification("Saved",aCurrentFileTab->filepath.c_str(), StatusBarManager::NotificationType::Success);
		aCurrentFileTab->editor->SetIsBufferModified(false);
	}
}

void TabsManager::DisableSearchForAllTabs(){
	for(auto& aTab:Get().mTabs)
	{
		aTab.editor->DisableSearch();
	}
}



void TabsManager::CloseTab(FileTab *aTab){
	GL_INFO("TabsManager::CloseTab - {}",aTab->filename);
	aTab->isOpen=false;
	auto& tabs=Get().mTabs;
	for(auto it=tabs.begin();it!=tabs.end();)
	{
		if(it->isOpen)
		{
			it++;
			continue;
		}

		bool wasDeletedTabFocused=it->isActive;
		it=tabs.erase(it);

		if(tabs.size()>0)
		{
			auto current=it;
			size_t idx=std::distance(tabs.begin(),it);
			if(idx>0)
			{
				current=it-1;
				current->isActive=true;
			}

			current->isActive=true;
			if(wasDeletedTabFocused){
				ImGui::FocusWindow(current->winPtr);
				FileNavigation::MarkFileAsOpen(current->filepath);
			}
		}
	}
}