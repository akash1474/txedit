#include "imgui.h"
#include "pch.h"
#include "StatusBarManager.h"
#include <cstdio>


void StatusBarManager::Init(Editor* editorPtr,FileNavigation* fileNavigation){ 
	mTextEditor=editorPtr;
	mNotificationAnimation.SetDuration(2.0f); 
	mFileNavigation=fileNavigation;
}

bool StatusBarManager::IsInputPanelOpen(){ return mIsInputPanelOpen;}


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


void StatusBarManager::Render(ImVec2& size,const ImGuiViewport* viewport){

	static const ImGuiIO& io=ImGui::GetIO();


	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.0f); //Popped at end of StatusBar
	if(mIsInputPanelOpen)
		RenderInputPanel(size,viewport);


	ImGui::SetNextWindowPos(ImVec2(0,mIsInputPanelOpen ? size.y+PanelSize : size.y));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x,StatusBarSize));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(4.0f,0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg,ImGui::GetStyle().Colors[ImGuiCol_TitleBg]);
	ImGui::PushFont(io.Fonts->Fonts[1]);
	ImGui::Begin("Status Bar",0,ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize );

		ImGui::PushFont(io.Fonts->Fonts[0]);
		if(ImGui::Button(ICON_FA_BARS)) mFileNavigation->ToggleSideBar();
		ImGui::PopFont();

		ImGui::SameLine();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY()+2.0f);
		ImGui::Text("Line:%d",mTextEditor->GetEditorState()->mCursorPosition.mLine+1);

		ImGui::SameLine();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY()+2.0f);
		ImGui::Text("Column:%d",mTextEditor->GetEditorState()->mCursorPosition.mColumn+1);


		if(mDisplayNotification){
			mDisplayNotification=false;
			mNotificationAnimation.start();
		}
		if(mNotificationAnimation.hasStarted){
			ImGui::SameLine();
			ImGui::SetCursorPosY(ImGui::GetCursorPosY()+2.0f);
			ImGui::TextColored(mNotificationColor,"%s", mNotificationString.c_str());
			mNotificationAnimation.update();
		}


		static bool branchLoaded =false;
		static std::string branch;
		if(!branchLoaded){
			// branch=exec("git rev-parse --abbrev-ref HEAD > git.txt");
			branchLoaded=true;
		}
		if(!branch.empty()){
			ImGui::PushFont(io.Fonts->Fonts[0]);
			ImGui::SameLine(ImGui::GetWindowWidth()-60.0f);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY());
			ImGui::Text("%s %s",ICON_FA_CODE_BRANCH,branch.c_str());
			ImGui::PopFont();
		}

		// FileType
		float width=ImGui::CalcTextSize(mTextEditor->fileType.c_str()).x;
		ImGui::SameLine(ImGui::GetWindowWidth()-width-10.0f);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY()+2.0f);
		ImGui::Text("%s", mTextEditor->fileType.c_str());

	ImGui::End();
	ImGui::PopFont();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(3);

}




void StatusBarManager::RenderInputPanel(ImVec2& size,const ImGuiViewport* viewport){

	static const ImGuiIO& io=ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(0,size.y));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x,PanelSize));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(4.0f,0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg,ImGui::GetStyle().Colors[ImGuiCol_TitleBg]);
	ImGui::PushFont(io.Fonts->Fonts[1]);
	ImGui::Begin("InputBar",0,ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize );

		const ImVec2 txt_dim=ImGui::CalcTextSize(mPanelTitle.c_str());
		const float y=(ImGui::GetWindowHeight()-txt_dim.y)*0.5f;

		ImGui::PushFont(io.Fonts->Fonts[0]);
		ImGui::SetCursorPosY(y-ImGui::GetStyle().FramePadding.y*2);
		ImGui::Text("%s", mPanelTitle.c_str());


		ImGui::SetCursorPos({txt_dim.x,5.0f});
		static char buff[256]="";
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(4.0f,6.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBg,ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg]);
		ImGui::PushItemWidth(viewport->WorkSize.x-txt_dim.x-5.0f);
		if(ImGui::InputText("##InputBar",buff,IM_ARRAYSIZE(buff),ImGuiInputTextFlags_EnterReturnsTrue)){
			GL_INFO("Submit");
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		ImGui::PopFont();
		if(ImGui::IsKeyPressed(ImGuiKey_Escape)){
			mIsInputPanelOpen=false;
		}
	ImGui::End();
	ImGui::PopFont();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);

}




void StatusBarManager::ShowNotification(const char* title,const char* info,NotificationType type){
	char buffer[128];
	std::sprintf(buffer,"%s:%s",title,info);
	mNotificationString=buffer;
	mDisplayNotification=true;
	mNotificationColor=GetNotificationColor(type);
}



void StatusBarManager::ShowInputPanel(const char* title,const char* placeholder,bool showButton,const char* btnName){
	mPanelTitle=title;
	if(placeholder) mPlaceholder=placeholder;
	if(showButton){
		mShowButton=true;
		mButtonTitle=btnName;
	}
	mIsInputPanelOpen=true;
}
