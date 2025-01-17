#include "pch.h"
#include "FontAwesome6.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdint>

#include "TextEditor.h"
#include "FileNavigation.h"
#include "TabsManager.h"
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
		if(it->editor->Render(&it->isOpen,it->id) && !it->isActive){
			for(auto&tab:Get().mTabs) 
				tab.isActive=false;

			it->isActive=true;
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

}


bool TabsManager::RenderTab(std::vector<FileTab>::iterator tab,bool& shouldDelete)
{
	ImGuiWindow* window=ImGui::GetCurrentWindow();
	if(window->SkipItems) return false;

	ImGuiID id=window->GetID(tab->id.c_str());

	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[ tab->isTemp ? (uint8_t)Fonts::JetBrainsMonoNLItalic : (uint8_t)Fonts::JetBrainsMonoNLRegular]);
	const ImVec2 fontSize=ImGui::CalcTextSize(tab->filename.c_str());
	const ImVec2 tabSize{fontSize.x>140.0f ? fontSize.x+50.0f: 200.0f,40.0f};
	ImDrawList* drawlist=ImGui::GetWindowDrawList();

	const ImVec2 pos=window->DC.CursorPos;	
	const ImRect rect(pos,{pos.x+tabSize.x,pos.y+tabSize.y});


	ImGui::ItemSize(rect,0.0f);
	if(!ImGui::ItemAdd(rect, id))
	{
		ImGui::PopFont();
		return false;
	}

	bool isHovered,isHeld;
	bool isClicked=ImGui::ButtonBehavior(rect, id,&isHovered,&isHeld,0);
	ImU32 bgColor=isHovered ? IM_COL32(25, 25,25, 255) : IM_COL32(17, 19,20, 255);

	drawlist->AddRectFilled(rect.Min,rect.Max , tab->isActive ? IM_COL32(29, 32,33, 255) : bgColor ,0.0f);
	if(!tab->isActive) 
		drawlist->AddLine({rect.Max.x-1,rect.Min.y+2.0f},{rect.Max.x-1,rect.Max.y-2.0f},IM_COL32(70, 70, 70, 255));

	const ImVec2 startText{pos.x+10.0f,pos.y+((tabSize.y-fontSize.y)*0.5f)};
	drawlist->AddText(startText,IM_COL32_WHITE,tab->filename.c_str());
	ImGui::PopFont();
	// ImGui::RenderText(startText, tab->filename.c_str());

	if(isHovered && GImGui->HoveredIdTimer > 1.0f)
	{
		// GL_INFO("TEXTSIZE:{}",fontSize.x);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{10.0,5.0f});
		ImGui::BeginTooltip();
		ImGui::Text("%s", tab->filepath.c_str());
		ImGui::EndTooltip();
		ImGui::PopStyleVar();
	}

	if(!tab->isSaved) 
		drawlist->AddCircleFilled({pos.x+tabSize.x-20.0f,pos.y+((tabSize.y-10.0f)*0.5f)+5.0f}, 5.0f, IM_COL32(100,100,100,255));
	else
	{
		ImVec2 btnPos{pos.x+tabSize.x-22.0f,pos.y+((tabSize.y-10.0f)*0.5f)-2.0f};
		ImRect btnRect(btnPos,{btnPos.x+14,btnPos.y+14});
		bool isHovered;

		ImGui::ButtonBehavior(btnRect, id,&isHovered,NULL,0);
		drawlist->AddText({pos.x+tabSize.x-20.0f,pos.y+((tabSize.y-ImGui::CalcTextSize(ICON_FA_XMARK).y)*0.5f)},isHovered? IM_COL32(255,255,255,255) :IM_COL32(100,100,100,255),ICON_FA_XMARK);
		
		if(isHovered && isClicked) 
			shouldDelete=true;
	}


	return isClicked;
}