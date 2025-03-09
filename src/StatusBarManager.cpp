#include "pch.h"
#include "imgui.h"
#include <cstdio>
#include <filesystem>
#include <stdio.h>
#include <string.h>

#include "StatusBarManager.h"
#include "TabsManager.h"
#include "FileNavigation.h"


void StatusBarManager::Init(){
	mNotificationAnimation.SetDuration(2.0f);
}

bool StatusBarManager::IsInputPanelOpen(){ return mIsInputPanelOpen;}
bool StatusBarManager::IsCaseSensitiveSearch(){return mIsCaseSensitive;}
bool StatusBarManager::IsRegexSearch(){return mIsRegexSearch;}
bool StatusBarManager::IsWholeWordSearch(){return mIsWholeWordSearch;}


ImVec4 StatusBarManager::GetNotificationColor(NotificationType type){
	ImVec4 color;
	switch(type){
		case NotificationType::Info:
			color=ImVec4(1.0f,1.0f,1.0f,1.0f);
			break;
		case NotificationType::Error:
			color=ImVec4(1.0f,0.0f,0.0f,1.0f);
			break;
		case NotificationType::Success:
			color=ImVec4(0.165,0.616,0.561,1.000);
			break;
		case NotificationType::Warning:
			color=ImVec4(0.5f,0.5f,0.0f,1.0f);
			break;
		default:
			color=ImVec4(1.0f,1.0f,1.0f,1.0f);
			break;
	}
	return color;
}

std::string StatusBarManager::GetCurrentGitBranch(const std::string& aDirectory){
	std::string gitDirectory=aDirectory+"/.git/";
	std::string currentBranch;
	if(std::filesystem::exists(gitDirectory))
	{
		std::ifstream headFile(gitDirectory+"HEAD");
		if(headFile.good())
		{
			std::getline(headFile,currentBranch);
			size_t idx=currentBranch.find_last_of('/');
			if(idx!=std::string::npos)
				currentBranch=currentBranch.substr(idx+1);
		}
	}
	return std::move(currentBranch);
}


void StatusBarManager::Render(ImVec2& size,const ImGuiViewport* viewport){

	static const ImGuiIO& io=ImGui::GetIO();

	Editor* textEditor=TabsManager::GetCurrentActiveTextEditor();

	if(mIsInputPanelOpen){
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.0f); //Popped at end of StatusBar
		RenderInputPanel(size,viewport);
		ImGui::PopStyleVar();
	}else if(mIsFileSearchPanelOpen){
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.0f); //Popped at end of StatusBar
		RenderFileSearchPanel(size,viewport);
		ImGui::PopStyleVar();
	}


	ImGui::SetNextWindowPos(ImVec2(0,IsAnyPanelOpen() ? size.y+PanelSize : size.y));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x,StatusBarSize));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(4.0f,0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg,ImGui::GetStyle().Colors[ImGuiCol_TitleBg]);
	ImGui::Begin("Status Bar",0,ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize );

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(6.0f,4.0f));
		ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyle().Colors[ImGuiCol_Border]);
		if(ImGui::Button(ICON_FA_BARS)) 
			FileNavigation::ToggleSideBar();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();

		float textPosY=(ImGui::GetWindowHeight()-ImGui::CalcTextSize("#").y)*0.5f;

		if(textEditor)
		{
			ImGui::SameLine();
			ImGui::Text("Line:%d Column:%d",textEditor->GetCurrentCursor().mCursorPosition.mLine+1,textEditor->GetCurrentCursor().mCursorPosition.mColumn+1);

			int count=textEditor->GetCursorCount();
			if(count >1)
			{
				ImGui::SameLine(0.0f,20.0f);
				ImGui::Text("nCursors:%d",count);
			}
		}


		if(mDisplayNotification){
			mDisplayNotification=false;
			mNotificationAnimation.start();
		}
		if(mNotificationAnimation.hasStarted){
			ImGui::SameLine(0.0f,20.0f);
			ImGui::TextColored(mNotificationColor,"%s", mNotificationString.c_str());
			mNotificationAnimation.update();
		}



		static bool branchLoaded =false;
		if(!branchLoaded){
			mGitBranch=GetCurrentGitBranch(".");
			branchLoaded=true;
		}
		float textWidth=ImGui::CalcTextSize(mGitBranch.c_str()).x;
		if(!mGitBranch.empty()){
			ImGui::PushFont(io.Fonts->Fonts[0]);
			
			ImGui::SameLine(ImGui::GetWindowWidth()-50.0f-textWidth);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY());
			ImGui::Text("%s %s",ICON_FA_CODE_BRANCH,mGitBranch.c_str());
			ImGui::PopFont();
		}

		// FileType
		if(textEditor)
		{
			float width=ImGui::CalcTextSize(textEditor->GetFileTypeName().c_str()).x;
			ImGui::SameLine(ImGui::GetWindowWidth()-width-100.0f-textWidth);
			ImGui::Text("%s", textEditor->GetFileTypeName().c_str());
		}

	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);



}




void StatusBarManager::RenderInputPanel(ImVec2& size,const ImGuiViewport* viewport)
{

	static const ImGuiIO& io=ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(0,size.y));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x,PanelSize));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(4.0f,0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg,ImGui::GetStyle().Colors[ImGuiCol_TitleBg]);
	ImGui::Begin("InputBar",0,ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize );

		const ImVec2 txt_dim=ImGui::CalcTextSize(mPanelTitle.c_str());
		const float y=(ImGui::GetWindowHeight()-txt_dim.y)*0.5f;

		ImGui::SetCursorPosY(y);
		ImGui::Text("%s", mPanelTitle.c_str());


		ImGui::SetCursorPos({txt_dim.x+8.0f,5.0f});

		static const float buttonWidth=ImGui::CalcTextSize(mButtonTitle.c_str()).x+25.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(4.0f,6.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBg,ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg]);
		const float inputWidth=mShowButton ? viewport->WorkSize.x-txt_dim.x-5.0f-buttonWidth : viewport->WorkSize.x-txt_dim.x-15.0f;
		ImGui::PushItemWidth(inputWidth);
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
   			ImGui::SetKeyboardFocusHere(0);
		if(ImGui::InputText("##InputBar",mInputTextBuffer,IM_ARRAYSIZE(mInputTextBuffer),ImGuiInputTextFlags_EnterReturnsTrue))
		{
			GL_INFO("Submit");
			mIsCallbackEx ? mCallbackFnEx(mInputTextBuffer,mPlaceholder.c_str()): mCallbackFn(mInputTextBuffer);
			StatusBarManager::CloseInputPanel();
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		if(mShowButton)
		{
			ImGui::SetCursorPos({txt_dim.x+inputWidth+10.0f,5.0f});
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(4.0f,8.0f));
			ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyle().Colors[ImGuiCol_Border]);

			if(ImGui::Button(mButtonTitle.c_str()))
			{
				mIsCallbackEx ? mCallbackFnEx(mInputTextBuffer,mPlaceholder.c_str()): mCallbackFn(mInputTextBuffer);
				StatusBarManager::CloseInputPanel();
			}

			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
		}

		if(ImGui::IsKeyPressed(ImGuiKey_Escape)){
			mIsInputPanelOpen=false;
		}
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);

}

bool Button(const char* aTitle,const char* aTooltip,ImVec2 aFramePadding,const ImVec4& aColor)
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,aFramePadding);
	ImGui::PushStyleColor(ImGuiCol_Button,aColor);

	bool isClicked=ImGui::Button(aTitle);

	if(strlen(aTooltip) >0 && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("%s", aTooltip);

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	return isClicked;
}

void StatusBarManager::ShowFileSearchPanel()
{
	mIsFileSearchPanelOpen=true;
	Editor* cEditor=TabsManager::GetCurrentActiveTextEditor();
	
	std::string selectedText=cEditor->GetSelectedText();	
	if(!selectedText.empty())
	{
		strcpy_s(mSearchInputBuffer,selectedText.c_str());
	}

}


void StatusBarManager::RenderFileSearchPanel(ImVec2& size,const ImGuiViewport* viewport)
{
	static const ImGuiIO& io=ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(0,size.y));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x,PanelSize));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(4.0f,2.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg,ImGui::GetStyle().Colors[ImGuiCol_TitleBg]);
	ImGui::Begin("FileSearchPanel",0,ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize );


		if(mIsCaseSensitive)
		{
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImColor(215, 153, 33, 255).Value);
			ImGui::PushStyleColor(ImGuiCol_Text,ImColor(0,0,0).Value);
		}
		bool clicked=Button("Aa","Case Sensitive",{7.0f,6.0f},mIsCaseSensitive ? ImColor(250, 189, 47, 255).Value : ImGui::GetStyle().Colors[ImGuiCol_Border]);
		if(mIsCaseSensitive) ImGui::PopStyleColor(2);

		if(clicked)
		{
			mIsCaseSensitive=!mIsCaseSensitive;
			Editor* currentEditor=TabsManager::GetCurrentActiveTextEditor();
			currentEditor->DisableSearch();
			currentEditor->ExecuteSearch(mSearchInputBuffer);
		}


		ImGui::SameLine();

		if(mIsRegexSearch)
		{
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImColor(215, 153, 33, 255).Value);
			ImGui::PushStyleColor(ImGuiCol_Text,ImColor(0,0,0).Value);
		}
		clicked=Button(".*","Regular Expression",{7.0f,6.0f},mIsRegexSearch ? ImColor(250, 189, 47, 255).Value : ImGui::GetStyle().Colors[ImGuiCol_Border]);
		if(mIsRegexSearch) ImGui::PopStyleColor(2);

		if(clicked)
		{
			mIsRegexSearch=!mIsRegexSearch;
			Editor* currentEditor=TabsManager::GetCurrentActiveTextEditor();
			currentEditor->DisableSearch();
			currentEditor->ExecuteSearch(mSearchInputBuffer);
		}


		ImGui::SameLine();

		if(mIsWholeWordSearch)
		{
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImColor(215, 153, 33, 255).Value);
			ImGui::PushStyleColor(ImGuiCol_Text,ImColor(0,0,0).Value);
		}
		clicked=Button("\"\"","Whole Word",{7.0f,6.0f},mIsWholeWordSearch ? ImColor(250, 189, 47, 255).Value : ImGui::GetStyle().Colors[ImGuiCol_Border]);
		if(mIsWholeWordSearch) ImGui::PopStyleColor(2);

		if(clicked)
		{
			mIsWholeWordSearch=!mIsWholeWordSearch;
			Editor* currentEditor=TabsManager::GetCurrentActiveTextEditor();
			currentEditor->DisableSearch();
			currentEditor->ExecuteSearch(mSearchInputBuffer);
		}



		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(4.0f,6.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBg,ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg]);
		const float inputWidth= viewport->WorkSize.x-400.0f;
		ImGui::PushItemWidth(inputWidth);
		ImGui::SameLine();
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
   			ImGui::SetKeyboardFocusHere(0);
		if(ImGui::InputText("##SearchBar",mSearchInputBuffer,IM_ARRAYSIZE(mSearchInputBuffer)))
		{
			Editor* currentEditor=TabsManager::GetCurrentActiveTextEditor();
			if(!currentEditor->HasSearchStarted(mSearchInputBuffer))
				currentEditor->ExecuteSearch(mSearchInputBuffer);
			else
				currentEditor->GotoNextMatch();
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();


		ImGui::SameLine();
		if(Button("Find","",{8.0f,6.0f},ImGui::GetStyle().Colors[ImGuiCol_Border]))
		{
			Editor* currentEditor=TabsManager::GetCurrentActiveTextEditor();
			if(!currentEditor->HasSearchStarted(mSearchInputBuffer))
				currentEditor->ExecuteSearch(mSearchInputBuffer);
			else
				currentEditor->GotoNextMatch();
		}
		ImGui::SameLine();
		if(Button("Find Prev","",{8.0f,6.0f},ImGui::GetStyle().Colors[ImGuiCol_Border]))
		{
			TabsManager::GetCurrentActiveTextEditor()->GotoPreviousMatch();
		}
		ImGui::SameLine();
		if(Button("Find All","",{8.0f,6.0f},ImGui::GetStyle().Colors[ImGuiCol_Border]))
		{
			Editor* currentEditor=TabsManager::GetCurrentActiveTextEditor();

			if(!currentEditor->HasSearchStarted(mSearchInputBuffer))
				currentEditor->ExecuteSearch(mSearchInputBuffer);

			currentEditor->HighlightAllMatches();
			CloseFileSearchPanel();
		}




	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	if(ImGui::IsKeyPressed(ImGuiKey_Escape))
		CloseFileSearchPanel();
}

void StatusBarManager::CloseFileSearchPanel()
{
	mIsFileSearchPanelOpen=false;
	TabsManager::DisableSearchForAllTabs();
}

bool StatusBarManager::IsAnyPanelOpen()
{
	return mIsFileSearchPanelOpen || mIsInputPanelOpen;
}




void StatusBarManager::ShowNotification(const char* title,const char* info,NotificationType type){
	char buffer[128];
	std::sprintf(buffer,"%s:%s",title,info);
	mNotificationString=buffer;
	mDisplayNotification=true;
	mNotificationColor=GetNotificationColor(type);
}



void StatusBarManager::ShowInputPanel(const char* title,void(*callback)(const char*),const char* placeholder,bool showButton,const char* btnName){
	mPanelTitle=title;
	if(placeholder) mPlaceholder=placeholder;
	if(showButton){
		mShowButton=true;
		mButtonTitle=btnName;
	}
	mIsCallbackEx=false;
	mIsInputPanelOpen=true;
	mCallbackFn=callback;
	sprintf(mInputTextBuffer,"%s",mPlaceholder.c_str());
}

void StatusBarManager::ShowInputPanelEx(const char* title,void(*callback)(const char*,const char*),const char* placeholder,bool showButton,const char* btnName){
	mPanelTitle=title;
	if(placeholder) mPlaceholder=placeholder;
	if(showButton){
		mShowButton=true;
		mButtonTitle=btnName;
	}
	mIsInputPanelOpen=true;
	mCallbackFnEx=callback;
	mIsCallbackEx=true;
	sprintf(mInputTextBuffer,"%s",mPlaceholder.c_str());
}

void StatusBarManager::CloseInputPanel(){mIsInputPanelOpen=false;}
