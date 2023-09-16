#include "GLFW/glfw3.h"
#include "Log.h"
#include "imgui.h"
#include "pch.h"
#include "Editor.h"
#include <cctype>
#include <cmath>
#include <ctype.h>
#include <future>
#include <stdint.h>

Editor::Editor(){
	// InitPallet();
	// #undef IM_TABSIZE
	// #define IM_TABSIZE mTabWidth
}
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
	GetCurrentLineLengthUptoCursor();
	int idx=GetCurrentLineIndex();
    if (mCurrentLineIndex >= 0 && mCurrentLineIndex < mLines.size() && mState.mCursorPosition.mColumn >= 0 && idx <= mLines[mCurrentLineIndex].size()) {
    	if(newChar=='\"' || newChar=='\'' || newChar=='(' || newChar=='{' || newChar=='['){
    		mLines[mCurrentLineIndex].insert(idx, 1, newChar);
    		switch(newChar){
    			case '(':newChar=')';break;
    			case '[':newChar=']';break;
    			case '{':newChar='}';break;
    		}
    		mLines[mCurrentLineIndex].insert(idx+1, 1, newChar);
    		mCurrLineLength++;
    	}else{
    		if((newChar==')'|| newChar==']' || newChar=='}') && mLines[mCurrentLineIndex][idx]==newChar){
    			//Avoiding ')' reentry after '(' pressed aka "()"
    		}else{
		        mLines[mCurrentLineIndex].insert(idx, 1, newChar);
    		}
    	}
	    mState.mCursorPosition.mColumn++;
    	mCurrLineLength++;
    }
}

void Editor::InsertLine(){
	int idx=GetCurrentLineIndex();
    reCalculateBounds=true;
	if(idx!=mLines[mCurrentLineIndex].size()){
		GL_INFO("CR BETWEEN");
		std::string substr=mLines[mCurrentLineIndex].substr(idx);
		mLines[mCurrentLineIndex].erase(idx);
		mCurrentLineIndex++;
		mLines.insert(mLines.begin()+mCurrentLineIndex,substr);
	}else{
		mCurrentLineIndex++;
		mLines.insert(mLines.begin()+mCurrentLineIndex,std::string(""));
	}
	mState.mCursorPosition.mLine++;
	mState.mCursorPosition.mColumn=0;
	GetCurrentLineLength();
	mLinePosition.y+=mLineHeight;
	mCurrentLineNo++;
}

// Function to handle the backspace keypress and remove a character
//	/tJohn is GoodMan
//	    John is GoodMan
void Editor::Backspace() {
	//Inside line
    if (mCurrentLineIndex >= 0 && mCurrentLineIndex < mLines.size() && mState.mCursorPosition.mColumn > 0 && mState.mCursorPosition.mColumn <= GetCurrentLineLength()) {
    	// GetCurrentLineLengthUptoCursor();	 //check it
	    int idx=GetCurrentLineIndex();
	    GL_INFO("IDX:{}",idx);

		if(mSelectionMode==SelectionMode::Word){
			mSelectionMode=SelectionMode::Normal;
			uint8_t word_len=mState.mSelectionEnd.mColumn-mState.mSelectionStart.mColumn;
			mLines[mCurrentLineIndex].erase(idx-word_len,word_len);
			mState.mCursorPosition=mState.mSelectionStart;
			return;
	    }

	    char x=mLines[mCurrentLineIndex][idx];
	    if((x==')'|| x==']' || x=='}')){
	    	bool shouldDelete=false;
	    	char y=mLines[mCurrentLineIndex][idx-1];
	    	if(x==')' && y=='(') shouldDelete=true;
	    	else if(x==']' && y=='[') shouldDelete=true;
	    	else if(x=='}' && y=='{') shouldDelete=true;
	    	if(shouldDelete){
		    	mLines[mCurrentLineIndex].erase(idx - 1, 2);
		    	mCurrLineLength=GetCurrentLineLength();
		        mState.mCursorPosition.mColumn--;
		        return;
	    	}
	    }
    	if(mLines[mCurrentLineIndex][idx-1]=='\t'){
    		GL_INFO("TAB REMOVE");
    		mLines[mCurrentLineIndex].erase(idx-1, 1);
    		mCurrLineLength=GetCurrentLineLength();
    		mState.mCursorPosition.mColumn-=mTabWidth;
    	}else{
    		GL_INFO("CHAR REMOVE");
	        mLines[mCurrentLineIndex].erase(idx - 1, 1);
    		mCurrLineLength=GetCurrentLineLength();
	        mState.mCursorPosition.mColumn--;
    	}
    } else if (mCurrentLineIndex > 0 && mState.mCursorPosition.mColumn == 0) {
    	reCalculateBounds=true;
    	GL_INFO("DELETING LINE");
    	if(mLines[mCurrentLineIndex].size() == 0){
    		GL_INFO("EMPTY LINE");
	        mLines.erase(mLines.begin() + mCurrentLineIndex);
	        mCurrentLineIndex--;
	        mCurrentLineNo--;
	        mState.mCursorPosition.mColumn=GetCurrentLineLength();
    	}else{
    		GL_INFO("LINE BEGIN");
	        int tempCursorX=GetCurrentLineLength(mCurrentLineIndex-1);
	        mLines[mCurrentLineIndex - 1] += mLines[mCurrentLineIndex];
	        mLines.erase(mLines.begin() + mCurrentLineIndex);
	        mCurrentLineIndex--;
	        mCurrentLineNo--;
	        mState.mCursorPosition.mColumn=tempCursorX;
	        mState.mCursorPosition.mLine--;
    	}
    }
}


size_t Editor::GetCurrentLineLength(int currLineIndex){
	if(currLineIndex==-1) currLineIndex=mCurrentLineIndex;
	mCurrLineTabCounts=0;
	int max=mLines[currLineIndex].size();
	for(int i=0;i<max;i++){ if(mLines[currLineIndex][i]=='\t') mCurrLineTabCounts++; }
	return max-mCurrLineTabCounts+(mCurrLineTabCounts*mTabWidth);
}

size_t Editor::GetCurrentLineLengthUptoCursor(){
	//
	mCurrLineTabCounts=0;
	// int max=GetCurrentLineIndex()+1; //Wrong -> idx based on all the tabs present in line
	int width=0;
	int i=0;
	for(i=0;width<(mState.mCursorPosition.mColumn+1);i++){
		if(mLines[mCurrentLineIndex][i]=='\t'){
			mCurrLineTabCounts++;
			width+=mTabWidth;
			continue;
		}	
		width++;
	}
	return i-mCurrLineTabCounts+(mCurrLineTabCounts*mTabWidth);
}


inline uint32_t Editor::GetCurrentLineIndex(){
	//(mTabWidth-1) as each tab replaced by mTabWidth containing one '/t' for each tabcount
	int val=mState.mCursorPosition.mColumn-(mCurrLineTabCounts*(mTabWidth-1));
	return val > 0 ? val : 0;
}

void Editor::UpdateBounds(){
	GL_WARN("UPDATING BOUNDS");
	mEditorPosition=mEditorWindow->Pos;
	mEditorSize=ImVec2(mEditorWindow->ContentRegionRect.Max.x,mLines.size()*(mLineSpacing+mCharacterSize.y)+50.0f);
	mEditorBounds=ImRect(mEditorPosition,ImVec2(mEditorPosition.x+mEditorSize.x,mEditorPosition.y+mEditorSize.y));
	reCalculateBounds=false;
}

void Editor::SwapLines(bool up){
	int value=up ? -1 : 1;
	std::swap(mLines[mCurrentLineIndex],mLines[mCurrentLineIndex+value]);
	mCurrentLineNo+=value;
	mCurrentLineIndex+=value;
	mState.mCursorPosition.mLine+=value;	
}


void Editor::MoveUp(bool ctrl,bool shift){
	if(!shift && mSelectionMode!=SelectionMode::Normal){
		mSelectionMode=SelectionMode::Normal;
		return;
	}
	mCurrentLineNo--;
	mCurrentLineIndex--;
	mCurrLineLength=GetCurrentLineLength();
	if(mState.mCursorPosition.mColumn > mCurrLineLength) mState.mCursorPosition.mColumn=mCurrLineLength;
}


void Editor::MoveDown(bool ctrl,bool shift){
	if(!shift && mSelectionMode!=SelectionMode::Normal){
		mSelectionMode=SelectionMode::Normal;
		return;
	}
	mCurrentLineIndex++;
	mCurrLineLength=GetCurrentLineLength();
	if(mState.mCursorPosition.mColumn > mCurrLineLength) mState.mCursorPosition.mColumn=mCurrLineLength;
	mCurrentLineNo++;
}

void Editor::MoveLeft(bool ctrl,bool shift){
	if(!shift && mSelectionMode!=SelectionMode::Normal){
		mSelectionMode=SelectionMode::Normal;
		mState.mCursorPosition=mState.mSelectionStart;
		GetCurrentLineIndex();
		return;
	}

	if(ctrl && mSelectionMode==SelectionMode::Normal){
		int idx=GetCurrentLineIndex();
		if(idx>0 && isalnum(mLines[mCurrentLineIndex][idx-1])){
			GL_INFO("WORD JUMP LEFT");
			uint8_t count=0;
			while(idx>0 && isalnum(mLines[mCurrentLineIndex][idx-1])){
				idx--;
				count++;
			}
			mState.mCursorPosition.mColumn-=count;
			return;
		}
	}

	if(mState.mCursorPosition.mColumn==0){
		mCurrentLineNo--;
		mCurrentLineIndex--;
		mCurrLineLength=GetCurrentLineLength();
		GL_INFO(mCurrLineLength);
		mState.mCursorPosition.mColumn=mCurrLineLength;
	}else{
		// GetCurrentLineLengthUptoCursor();
		// GL_INFO(GetCurrentLineIndex());
		if(GetCurrentLineIndex()==0 && mLines[mCurrentLineIndex][0]=='\t'){
			mState.mCursorPosition.mColumn=0;
			return;
		}
		if(GetCurrentLineIndex()>0 && mLines[mCurrentLineIndex][GetCurrentLineIndex()-1]=='\t') {
			mState.mCursorPosition.mColumn-=mTabWidth;
		}else{
			mState.mCursorPosition.mColumn--;
		}
	}
}

void Editor::MoveRight(bool ctrl,bool shift){
	if(!shift && mSelectionMode!=SelectionMode::Normal){
		mSelectionMode=SelectionMode::Normal;
		mState.mCursorPosition=mState.mSelectionEnd;
		GetCurrentLineIndex();
		return;
	}

	if(ctrl && mSelectionMode==SelectionMode::Normal){
		int idx=GetCurrentLineIndex();
		if(isalnum(mLines[mCurrentLineIndex][idx])){
			GL_INFO("WORD JUMP RIGHT");
			uint8_t count=0;
			while(isalnum(mLines[mCurrentLineIndex][idx])){
				idx++;
				count++;
			}
			mState.mCursorPosition.mColumn+=count;
			return;
		}
	}

	if(mState.mCursorPosition.mColumn==mCurrLineLength){
		mCurrentLineIndex++;
		mCurrLineLength=GetCurrentLineLength();
		mState.mCursorPosition.mColumn=0;
		mCurrentLineNo++;
	}else if(mState.mCursorPosition.mColumn < mCurrLineLength){
		if(mState.mCursorPosition.mColumn==0 && mLines[mCurrentLineIndex][0]=='\t'){
			mState.mCursorPosition.mColumn+=mTabWidth;
		}else if(mLines[mCurrentLineIndex][GetCurrentLineIndex()]=='\t'){
			mState.mCursorPosition.mColumn+=mTabWidth;
		}else{
			mState.mCursorPosition.mColumn++;
		}
	}
}

bool Editor::render(){
	static bool isInit=false;
	if(!isInit){
		mEditorWindow=ImGui::GetCurrentWindow();	
		mCharacterSize=ImVec2(ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#",nullptr,nullptr));
		mLinePosition=ImVec2({mEditorPosition.x+45.0f,0});
		mLineHeight=mLineSpacing+mCharacterSize.y;
		mTitleBarHeight=ImGui::GetWindowHeight() - ImGui::GetContentRegionAvail().y;
		GL_WARN("LINE HEIGHT:{}",mLineHeight);
		GL_WARN("TITLE HEIGHT:{}",mTitleBarHeight);
		isInit=true;
	}
	if(mEditorPosition.x!=mEditorWindow->Pos.x || mEditorPosition.y != mEditorWindow->Pos.y) reCalculateBounds=true;
	if(reCalculateBounds) UpdateBounds();


	const ImGuiIO& io=ImGui::GetIO();
	const ImGuiID id=ImGui::GetID("##Editor");
	ImGui::ItemSize(mEditorBounds,0.0f);
	if(!ImGui::ItemAdd(mEditorBounds, id)) return false;
	HandleKeyboardInputs();
	HandleMouseInputs();


	//BackGrounds
	mEditorWindow->DrawList->AddRectFilled(mEditorPosition,{mEditorPosition.x+40.0f,mEditorSize.y}, ImColor(37,37,56,255)); // LineNo
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition.x+41.0f,mEditorPosition.y},mEditorBounds.Max, ImColor(26,26,33,255));// Code


	mMinLineVisible=(ImGui::GetScrollY())/mLineHeight;
	if(mMinLineVisible<0.0f) mMinLineVisible=0.0f;
	mLinePosition.y=mTitleBarHeight+(mLineSpacing*0.5f)+floor(mCurrentLineNo-mMinLineVisible)*mLineHeight;

	//Drawing Current Lin
	mEditorWindow->DrawList->AddRectFilled(mLinePosition,{mEditorBounds.Max.x,mLinePosition.y+mLineHeight}, ImColor(50,50,50,150));// Code
	mLineHeight=mLineSpacing+mCharacterSize.y;


	int start=int(mMinLineVisible);
	if(start>mLines.size()) start=mLines.size();
	int lineCount=(mEditorWindow->Size.y)/mLineHeight;
	int end=start+lineCount+1;
	if(end>mLines.size()) end=mLines.size();


	int lineNo=0;
	while(start!=end){
		float posY=mLineSpacing+(lineNo*mLineHeight)+mTitleBarHeight;
		mEditorWindow->DrawList->AddText({mEditorPosition.x+10.0f,posY},ImColor(72,171,159,255),std::to_string(start+1).c_str());
		mEditorWindow->DrawList->AddText({mEditorPosition.x+45.0f,posY},ImColor(186,186,186,255),mLines[start].c_str());
		start++;
		lineNo++;
	}
	if(mSelectionMode==SelectionMode::Word){
		ImVec2 start(mEditorPosition.x+45.0f-1.0f+(mState.mSelectionStart.mColumn*mCharacterSize.x),mLinePosition.y);
		ImVec2 end(mEditorPosition.x+45.0f+(mState.mSelectionEnd.mColumn*mCharacterSize.x),mLinePosition.y+mLineHeight);
		mEditorWindow->DrawList->AddRectFilled(start,end,ImColor(30,109,232,50));
	}

	//Cursor
	ImVec2 cursorPosition(mEditorPosition.x+45.0f-1.0f+(mState.mCursorPosition.mColumn*mCharacterSize.x),mLinePosition.y);
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
		if (!shift && !alt)
		{
			auto click = ImGui::IsMouseClicked(0);
			auto doubleClick = ImGui::IsMouseDoubleClicked(0);
			auto t = ImGui::GetTime();
			auto tripleClick = click && !doubleClick && (mLastClick != -1.0f && (t - mLastClick) < io.MouseDoubleClickTime);

			/*
			Left mouse button triple click
			*/

			if (tripleClick)
			{
				if (!ctrl)
				{

				}
				mLastClick = -1.0f;
			}

			/*
			Left mouse button double click
			*/

			else if (doubleClick)
			{
				if (!ctrl)
				{
					// mState.mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
					if (mSelectionMode == SelectionMode::Line)
						mSelectionMode = SelectionMode::Normal;
					else
						mSelectionMode = SelectionMode::Word;

					int idx=GetCurrentLineIndex();
					int start_idx{idx};
					int end_idx{idx};
					bool a=true,b=true;
					while((a || b)){
						// GL_WARN("SELECTION IDX:{} START:{} END:{}",idx,start_idx,end_idx);
						char chr{0};
						if(start_idx==0 || start_idx<0){
							a=false;
							start_idx=0;
						}else{
							chr=mLines[mCurrentLineIndex][start_idx-1];
						}
						if(a && (isalnum(chr) || chr=='_')) start_idx--;
						else a=false;
						if(end_idx>=mLines[mCurrentLineIndex].size()){ 
							b=false;
							end_idx=mLines[mCurrentLineIndex].size();
						}else{
							chr=mLines[mCurrentLineIndex][end_idx];
							if(b && (isalnum(chr) || chr=='_')) {end_idx++;} else {b=false;}
						}
					}
					GL_WARN("SELECTION IDX:{} START:{} END:{}",idx,start_idx,end_idx);
					GetCurrentLineLengthUptoCursor(); //updating mCurrLineTabCounts
					GL_INFO("TAB COUNT:{}",mCurrLineTabCounts);
					mState.mSelectionStart=Coordinates(mState.mCursorPosition.mLine,start_idx+(mCurrLineTabCounts*(mTabWidth-1)));
					mState.mSelectionEnd=Coordinates(mState.mCursorPosition.mLine,end_idx+(mCurrLineTabCounts*(mTabWidth-1)));
					mState.mCursorPosition=mState.mSelectionEnd;
				}

				mLastClick = (float)ImGui::GetTime();
			}

			/*
			Left mouse button click
			*/
			else if (click)
			{
				mCurrentLineNo=((ImGui::GetScrollY()+ImGui::GetMousePos().y-(0.5f*mLineSpacing)-mTitleBarHeight)/mLineHeight);
				mMinLineVisible=((ImGui::GetScrollY())/mLineHeight);
				GL_INFO("ScrollY:{0}  ,MouseY:{1}  ,currLine:{2}  ,minLine:{3}",ImGui::GetScrollY(),ImGui::GetMousePos().y,mCurrentLineNo,mMinLineVisible);
				mState.mCursorPosition.mColumn=round((ImGui::GetMousePos().x-mEditorPosition.x-45.0f)/mCharacterSize.x);
				mCurrentLineIndex=floor(mCurrentLineNo-(mMinLineVisible-floor(mMinLineVisible)));
				if(mCurrentLineIndex<0) mCurrentLineIndex=0;
				mCurrLineLength=GetCurrentLineLength();
				if(mState.mCursorPosition.mColumn > mCurrLineLength) mState.mCursorPosition.mColumn=mCurrLineLength;
				mSelectionMode=SelectionMode::Normal;
				mLastClick = (float)ImGui::GetTime();
			}
			// Mouse left button dragging (=> update selection)
			else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0))
			{
				// io.WantCaptureMouse = true;
				// mState.mState.mCursorPosition = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
				// SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
			}
		}
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
		if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
			MoveUp();
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
			MoveDown();
		else if (ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
			SwapLines(true);
		else if (ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
			SwapLines(false);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
			MoveLeft(ctrl,shift);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
			MoveRight(ctrl,shift);
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
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
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
		else if (!IsReadOnly() && !ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab))){
			if(mSelectionMode==SelectionMode::Normal){
				mCurrLineLength=GetCurrentLineLength();
				int idx=GetCurrentLineIndex();
				GL_INFO("IDX:",idx);
				mLines[mCurrentLineIndex].insert(mLines[mCurrentLineIndex].begin()+idx,1,'\t');
				mCurrLineLength=GetCurrentLineLength();
				mState.mCursorPosition.mColumn+=mTabWidth;
			}
		}

		if (!mReadOnly && !io.InputQueueCharacters.empty())
		{
			if(mSelectionMode==SelectionMode::Word) Backspace();
			auto c = io.InputQueueCharacters[0];
			if (c != 0 && (c == '\n' || c >= 32))
				// EnterCharacter(c, shift);
				GL_INFO("{}",(char)c);
				InsertCharacter(c);
			io.InputQueueCharacters.resize(0);
		}
	}
}
