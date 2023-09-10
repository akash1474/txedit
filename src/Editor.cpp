#include "GLFW/glfw3.h"
#include "Log.h"
#include "imgui.h"
#include "pch.h"
#include "Editor.h"

Editor::Editor(){}
Editor::~Editor(){}


void Editor::SetBuffer(const std::string& text){
    mLines.clear();
	std::string currLine;
	for(auto chr:text){
		if(chr=='\r') continue;
		// if(chr=='\t') {
		// 	currLine+=mTabWidth*;
		// 	continue;
		// }
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

// Function to insert a character at a specific position in a line
void Editor::InsertCharacter(char newChar) {
    if (mCurrentLineIndex >= 0 && mCurrentLineIndex < mLines.size() && mCursorPosition.x >= 0 && mCursorPosition.x <= mLines[mCurrentLineIndex].length()) {
        mLines[mCurrentLineIndex].insert(mCursorPosition.x, 1, newChar);
        mCursorPosition.x++;
    }
}

void Editor::InsertLine(){
	int actualPosition=mCursorPosition.x-(mCurrLineTabCounts*mTabWidth);
	GL_INFO("STR IDX:{}",actualPosition);
    reCalculateBounds=true;
	if(actualPosition!=mLines[mCurrentLineIndex].size()){
		GL_INFO("CR BETWEEN");
		std::string substr=mLines[mCurrentLineIndex].substr(actualPosition);
		mLines[mCurrentLineIndex].erase(actualPosition);
		mCurrentLineIndex++;
		mLines.insert(mLines.begin()+mCurrentLineIndex,substr);
	}else{
		mCurrentLineIndex++;
		mLines.insert(mLines.begin()+mCurrentLineIndex,std::string(""));
	}
	mCursorPosition.y++;
	mCursorPosition.x=0;
	GetCurrentLineLength();
	mLinePosition.y+=mLineHeight;
	mCurrentLineNo++;
}

// Function to handle the backspace keypress and remove a character
void Editor::Backspace() {
	//Inside line
    if (mCurrentLineIndex >= 0 && mCurrentLineIndex < mLines.size() && mCursorPosition.x > 0 && mCursorPosition.x <= mLines[mCurrentLineIndex].length()) {
        mLines[mCurrentLineIndex].erase(mCursorPosition.x - 1, 1);
        mCursorPosition.x--;
    } else if (mCurrentLineIndex > 0 && mCursorPosition.x == 0) {
    	reCalculateBounds=true;
    	GL_INFO("DELETING LINE");
    	if(mLines[mCurrentLineIndex].size() == 0){
    		GL_INFO("EMPTY LINE");
	        mLines.erase(mLines.begin() + mCurrentLineIndex);
	        mCurrentLineIndex--;
	        mLinePosition.y-=mLineHeight;
	        mCurrentLineNo--;
	        mCursorPosition.x=GetCurrentLineLength();
	        mCursorPosition.y--;
    	}else{
	        // mLines[mCurrentLineIndex - 1].pop_back();
	        int pos=GetCurrentLineLength(mCurrentLineIndex-1);
	        mLines[mCurrentLineIndex - 1] += mLines[mCurrentLineIndex];
	        mLines.erase(mLines.begin() + mCurrentLineIndex);
	        mCurrentLineIndex--;
	        mLinePosition.y-=mLineHeight;
	        mCurrentLineNo--;
	        mCursorPosition.x=pos;
	        mCursorPosition.y--;
    	}
    }
}

void Editor::UpdateBounds(){
	GL_WARN("UPDATING BOUNDS");
	mEditorPosition=mEditorWindow->Pos;
	mEditorSize=ImVec2(mEditorWindow->ContentRegionRect.Max.x,mLines.size()*(mLineSpacing+mCharacterSize.y)+50.0f);
	mEditorBounds=ImRect(mEditorPosition,ImVec2(mEditorPosition.x+mEditorSize.x,mEditorPosition.y+mEditorSize.y));
	reCalculateBounds=false;
}

size_t Editor::GetCurrentLineLength(int currLineIndex){
	if(currLineIndex==-1) currLineIndex=mCurrentLineIndex;
	mCurrLineTabCounts=0;
	int max=mLines[currLineIndex].size() < 10 ? mLines[currLineIndex].size(): 10;
	for(int i=0;i<max;i++){ if(mLines[currLineIndex][i]=='\t') mCurrLineTabCounts++; }
	return mLines[currLineIndex].size()+(mCurrLineTabCounts*mTabWidth);
}


bool Editor::render(){
	static bool isInit=false;
	if(!isInit){
		mEditorWindow=ImGui::GetCurrentWindow();	
		mCharacterSize=ImVec2(ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " "));
		mCursorPosition=ImVec2(10,2);
		mLinePosition=ImVec2({mEditorPosition.x+45.0f,0});
		mLineHeight=mLineSpacing+mCharacterSize.y;
		isInit=true;
	}
	if(mEditorPosition.x!=mEditorWindow->Pos.x || mEditorPosition.y != mEditorWindow->Pos.y) reCalculateBounds=true;
	if(reCalculateBounds) UpdateBounds();


	const ImGuiIO& io=ImGui::GetIO();
	const ImGuiID id=ImGui::GetID("##Editor");
	ImGui::ItemSize(mEditorBounds,0.0f);
	if(!ImGui::ItemAdd(mEditorBounds, id)) return false;
	HandleKeyboardInputs();


	//BackGrounds
	mEditorWindow->DrawList->AddRectFilled(mEditorPosition,{mEditorPosition.x+40.0f,mEditorSize.y}, ImColor(37,37,56,255)); // LineNo
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition.x+41.0f,mEditorPosition.y},mEditorBounds.Max, ImColor(26,26,33,255));// Code

	if(ImGui::IsMouseClicked(ImGuiMouseButton_Left,false)) {
		float currLine=(ImGui::GetScrollY()+ImGui::GetMousePos().y-mEditorPosition.y-mLineSpacing*0.5f)/mLineHeight;
		mCurrentLineNo=currLine;
		mMinLineVisible=ImGui::GetScrollY()/mLineHeight;
		currLine-=mMinLineVisible;
		mCursorPosition.x=round((ImGui::GetMousePos().x-mEditorPosition.x-45.0f)/mCharacterSize.x);
		mLinePosition.y=mLineSpacing*0.5f+(int)currLine*mLineHeight;
		mCurrentLineIndex=floor(mCurrentLineNo-1);
		if(mCurrentLineIndex<0) mCurrentLineIndex=0;
		mCurrLineLength=GetCurrentLineLength();
		if(mCursorPosition.x > mCurrLineLength) mCursorPosition.x=mCurrLineLength;
		GL_INFO("LINE INDEX: {}",mCurrentLineIndex);
		GL_INFO("Line No:{}  ScrollY:{}, CursorPositionX:{}",mCurrentLineNo,mEditorWindow->Scroll.y,mCursorPosition.x);
	}

	mMinLineVisible=ImGui::GetScrollY()/mLineHeight;
	mLinePosition.y=mLineSpacing*0.5f+(int)(mCurrentLineNo-mMinLineVisible)*mLineHeight;

	if(ImGui::IsKeyPressed(ImGuiKey_DownArrow)){
		mCurrentLineIndex++;
		mCurrLineLength=GetCurrentLineLength();
		if(mCursorPosition.x > mCurrLineLength) mCursorPosition.x=mCurrLineLength;
		mLinePosition.y+=mLineHeight;
		mCurrentLineNo++;
	}else if(ImGui::IsKeyPressed(ImGuiKey_UpArrow)){
		mCurrentLineNo--;
		mCurrentLineIndex--;
		mCurrLineLength=GetCurrentLineLength();
		if(mCursorPosition.x > mCurrLineLength) mCursorPosition.x=mCurrLineLength;
		mLinePosition.y-=mLineHeight;
	}else if(ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && mCursorPosition.x > 0){
		mCursorPosition.x--;
	}else if(ImGui::IsKeyPressed(ImGuiKey_RightArrow) && mCursorPosition.x < mCurrLineLength){
		mCursorPosition.x++;
	}

	//Drawing Current Line
	mEditorWindow->DrawList->AddRectFilled(mLinePosition,{mEditorBounds.Max.x,mLinePosition.y+mLineHeight}, ImColor(50,50,50,150));// Code

	int start=ImGui::GetScrollY()/mLineHeight;
	if(start>mLines.size()) start=mLines.size();
	int lineCount=mEditorWindow->Size.y/mLineHeight;
	int end=start+lineCount;
	if(end>mLines.size()) end=mLines.size();



	int lineNo=1;
	while(start!=end){
		float posY=mLineSpacing+lineNo*mLineHeight;
		mEditorWindow->DrawList->AddText({mEditorPosition.x+10.0f,posY},ImColor(72,171,159,255),std::to_string(start+1).c_str());
		mEditorWindow->DrawList->AddText({mEditorPosition.x+45.0f,posY},ImColor(186,186,186,255),mLines[start].c_str());
		start++;
		lineNo++;
	}

	//Cursor
	ImVec2 cursorPosition(mEditorPosition.x+45.0f-1.0f+(mCursorPosition.x*mCharacterSize.x),mLinePosition.y);
	mEditorWindow->DrawList->AddRectFilled(cursorPosition,{cursorPosition.x+2.0f,cursorPosition.y+mLineHeight},ImColor(255,255,255,255));

	return true;
}



void Editor::HandleMouseInputs()
{
	ImGuiIO& io = ImGui::GetIO();
	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowHovered())
	{
		// if (!shift && !alt)
		// {
		// 	auto click = ImGui::IsMouseClicked(0);
		// 	auto doubleClick = ImGui::IsMouseDoubleClicked(0);
		// 	auto t = ImGui::GetTime();
		// 	auto tripleClick = click && !doubleClick && (mLastClick != -1.0f && (t - mLastClick) < io.MouseDoubleClickTime);

		// 	/*
		// 	Left mouse button triple click
		// 	*/

		// 	if (tripleClick)
		// 	{
		// 		if (!ctrl)
		// 		{
		// 			mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
		// 			mSelectionMode = SelectionMode::Line;
		// 			SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
		// 		}

		// 		mLastClick = -1.0f;
		// 	}

		// 	/*
		// 	Left mouse button double click
		// 	*/

		// 	else if (doubleClick)
		// 	{
		// 		if (!ctrl)
		// 		{
					// mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
		// 			if (mSelectionMode == SelectionMode::Line)
		// 				mSelectionMode = SelectionMode::Normal;
		// 			else
		// 				mSelectionMode = SelectionMode::Word;
		// 			SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
		// 		}

		// 		mLastClick = (float)ImGui::GetTime();
		// 	}

		// 	/*
		// 	Left mouse button click
		// 	*/
		// 	else if (click)
		// 	{
		// 		mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
		// 		if (ctrl)
		// 			mSelectionMode = SelectionMode::Word;
		// 		else
		// 			mSelectionMode = SelectionMode::Normal;
		// 		SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);

		// 		mLastClick = (float)ImGui::GetTime();
		// 	}
		// 	// Mouse left button dragging (=> update selection)
		// 	else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0))
		// 	{
		// 		io.WantCaptureMouse = true;
		// 		mState.mCursorPosition = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
		// 		SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
		// 	}
		// }
	}
}

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
		if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
			Backspace();
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
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
			InsertLine();
		// else if (!IsReadOnly() && !ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab)))
		// 	EnterCharacter('\t', shift);

		if (!mReadOnly && !io.InputQueueCharacters.empty())
		{
			auto c = io.InputQueueCharacters[0];
			if (c != 0 && (c == '\n' || c >= 32))
				// EnterCharacter(c, shift);
				GL_INFO("{}",(char)c);
				InsertCharacter(c);
			io.InputQueueCharacters.resize(0);
		}
	}
}
