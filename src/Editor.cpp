#include "GLFW/glfw3.h"
#include "Log.h"
#include "imgui.h"
#include "pch.h"
#include "Editor.h"

Editor::Editor(){}
Editor::~Editor(){}


void Editor::HandleKeyboardInputs()
{
	ImGuiIO& io = ImGui::GetIO();
	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowFocused())
	{
		if (ImGui::IsWindowHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
		//ImGui::CaptureKeyboardFromApp(true);

		io.WantCaptureKeyboard = true;
		io.WantTextInput = true;

		// if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
		// 	Undo();
		// else if (!IsReadOnly() && !ctrl && !shift && alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
		// 	Undo();
		// else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y)))
		// 	Redo();
		// else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
		// 	MoveUp(1, shift);
		// else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
		// 	MoveDown(1, shift);
		// else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
		// 	MoveLeft(1, shift, ctrl);
		// else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
		// 	MoveRight(1, shift, ctrl);
		// else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageUp)))
		// 	MoveUp(GetPageSize() - 4, shift);
		// else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageDown)))
		// 	MoveDown(GetPageSize() - 4, shift);
		// else if (!alt && ctrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home)))
		// 	MoveTop(shift);
		// else if (ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_End)))
		// 	MoveBottom(shift);
		// else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home)))
		// 	MoveHome(shift);
		// else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_End)))
		// 	MoveEnd(shift);
		// else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
		// 	Delete();
		// else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
		// 	Backspace();
		// else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
		// 	mOverwrite ^= true;
		// else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
		// 	Copy();
		// else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
		// 	Copy();
		// else if (!IsReadOnly() && !ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
		// 	Paste();
		// else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_V)))
		// 	Paste();
		// else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_X)))
		// 	Cut();
		// else if (!ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
		// 	Cut();
		// else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
		// 	SelectAll();
		// else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
		// 	EnterCharacter('\n', false);
		// else if (!IsReadOnly() && !ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab)))
		// 	EnterCharacter('\t', shift);

		if (!IsReadOnly() && !io.InputQueueCharacters.empty())
		{
			for (int i = 0; i < io.InputQueueCharacters.Size; i++)
			{
				auto c = io.InputQueueCharacters[i];
				if (c != 0 && (c == '\n' || c >= 32))
					// EnterCharacter(c, shift);
					GL_INFO("{}",(char)c);
			}
			io.InputQueueCharacters.resize(0);
		}
	}
}


void Editor::SetBuffer(const std::string& text){
    mLines.clear();
	std::string currLine;
	for(auto chr:text){
		if(chr=='\r') continue;
		if(chr=='\t') {
			currLine+="    ";
			continue;
		}
		if(chr=='\n'){
			mLines.push_back(currLine);
			currLine.clear();
		}else{
			currLine+=chr;
		}
	}
	mLines.push_back(currLine);
}

void keyboardCallback(GLFWwindow* window,int key, int scancode, int action, int mods){
	if (action == GLFW_PRESS){
        std::cout << "Key pressed: " << (char)key << std::endl;
	}
}


bool Editor::render(){
	static bool isInit=false;
	if(!isInit){
		mEditorWindow=ImGui::GetCurrentWindow();	
		mCharacterSize=ImVec2(ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " "));
		mEditorSize=ImVec2(mEditorWindow->ContentRegionRect.Max.x,mLines.size()*(mLineSpacing+mCharacterSize.y)+50.0f);
		mEditorBounds=ImRect(mEditorPosition,ImVec2(mEditorPosition.x+mEditorSize.x,mEditorPosition.y+mEditorSize.y));
		mCursorPosition=ImVec2(10,2);
		mLinePosition=ImVec2({mEditorPosition.x+45.0f,0});
		isInit=true;
	}
	if(mEditorPosition.x!=mEditorWindow->Pos.x || mEditorPosition.y != mEditorWindow->Pos.y) isInit=false;
	mEditorPosition=mEditorWindow->Pos;
	const ImGuiIO& io=ImGui::GetIO();
	const ImGuiID id=ImGui::GetID("##Editor");

	ImGui::ItemSize(mEditorBounds,0.0f);
	if(!ImGui::ItemAdd(mEditorBounds, id)) return false;

	HandleKeyboardInputs();


	//BackGrounds
	mEditorWindow->DrawList->AddRectFilled(mEditorPosition,{mEditorPosition.x+40.0f,mEditorSize.y}, ImColor(37,37,56,255)); // LineNo
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition.x+41.0f,mEditorPosition.y},mEditorBounds.Max, ImColor(26,26,33,255));// Code
	float lineHeight=mLineSpacing+mCharacterSize.y;

	static float prevLine=0.0f;


	if(ImGui::IsMouseClicked(ImGuiMouseButton_Left,false)) {
		float currLine=(ImGui::GetScrollY()+ImGui::GetMousePos().y-mEditorPosition.y)/lineHeight;
		prevLine=currLine;
		float lineMin=ImGui::GetScrollY()/lineHeight;
		currLine-=lineMin;
		mCursorPosition.x=round((ImGui::GetMousePos().x-mEditorPosition.x-45.0f)/mCharacterSize.x);
		if(mCursorPosition.x > mLines[prevLine-1].size()) mCursorPosition.x=mLines[prevLine-1].size();
		mLinePosition.y=mLineSpacing*0.5f+(int)currLine*lineHeight;
		GL_INFO("Line No:{}  scrollY:{}, mCursorPosition:{}",(int)currLine,mEditorWindow->Scroll.y,mCursorPosition.x);
	}

	float lineMin=ImGui::GetScrollY()/lineHeight;
	mLinePosition.y=mLineSpacing*0.5f+(int)(prevLine-lineMin)*lineHeight;

	if(ImGui::IsKeyPressed(ImGuiKey_DownArrow)) prevLine++;
	if(ImGui::IsKeyPressed(ImGuiKey_UpArrow)) prevLine--;
	if(ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && mCursorPosition.x > 0) mCursorPosition.x--;
	if(ImGui::IsKeyPressed(ImGuiKey_RightArrow) && mCursorPosition.x < mLines[prevLine-lineMin-1].size()) mCursorPosition.x++;


	mEditorWindow->DrawList->AddRectFilled(mLinePosition,{mEditorBounds.Max.x,mLinePosition.y+lineHeight}, ImColor(50,50,50,150));// Code

	int start=ImGui::GetScrollY()/lineHeight;
	if(start>mLines.size()) start=mLines.size();
	int lineCount=mEditorWindow->Size.y/lineHeight;
	int end=start+lineCount;
	if(end>mLines.size()) end=mLines.size();



	int lineNo=1;
	while(start!=end){
		float posY=mLineSpacing+lineNo*lineHeight;
		mEditorWindow->DrawList->AddText({mEditorPosition.x+10.0f,posY},ImColor(72,171,159,255),std::to_string(start+1).c_str());
		mEditorWindow->DrawList->AddText({mEditorPosition.x+45.0f,posY},ImColor(186,186,186,255),mLines[start].c_str());
		start++;
		lineNo++;
	}
	ImVec2 cursorPosition(mEditorPosition.x+45.0f-1.0f+(mCursorPosition.x*mCharacterSize.x),mLinePosition.y);
	mEditorWindow->DrawList->AddRectFilled(cursorPosition,{cursorPosition.x+2.0f,cursorPosition.y+lineHeight},ImColor(255,255,255,255));

	return true;
}