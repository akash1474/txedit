#include "Timer.h"
#include "pch.h"
#include "TextEditor.h"

#include <ctype.h>
#include <iterator>
#include <stdint.h>

#include <cctype>
#include <cmath>
#include <future>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "GLFW/glfw3.h"
#include "Log.h"
#include "imgui.h"


Editor::Editor()
{
	InitPallet();
	// #undef IM_TABSIZE
	// #define IM_TABSIZE mTabWidth
}
Editor::~Editor() {}


void Editor::SetBuffer(const std::string& text)
{
	mLines.clear();
	std::string currLine;

	for (auto chr : text) {
		if (chr == '\r') continue;

		if (chr == '\n') {
			mLines.push_back(currLine);
			currLine.clear();
		} else currLine += chr;
		
	}
	if(currLine.size()>400) currLine="";
	mLines.push_back(currLine);
}


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) {
		std::cout << "Key pressed: " << (char)key << std::endl;
	}
}


// Function to insert a character at a specific position in a line
void Editor::InsertCharacter(char newChar)
{
	int idx = GetCurrentLineIndex(mState.mCursorPosition);
	GL_INFO("INSERT IDX:{}",idx);

	if (mCurrentLineIndex >= 0 && mCurrentLineIndex < mLines.size() && mState.mCursorPosition.mColumn >= 0 &&
	    idx <= mLines[mCurrentLineIndex].size()) {

		if (newChar == '\"' || newChar == '\'' || newChar == '(' || newChar == '{' || newChar == '[') {

			mLines[mCurrentLineIndex].insert(idx, 1, newChar);
			switch (newChar) {
				case '(': newChar = ')'; break;
				case '[': newChar = ']'; break;
				case '{': newChar = '}'; break;
			}


			mLines[mCurrentLineIndex].insert(idx + 1, 1, newChar);
			mCurrLineLength++;
		} else {

			if ((newChar == ')' || newChar == ']' || newChar == '}') && mLines[mCurrentLineIndex][idx] == newChar) {
				// Avoiding ')' reentry after '(' pressed aka "()"
			} else {
				mLines[mCurrentLineIndex].insert(idx, 1, newChar);
			}

		}

		mState.mCursorPosition.mColumn++;
		mCurrLineLength++;
	}
}


void Editor::InsertLine()
{
	if(mSearchState.isValid()) mSearchState.reset();
	int idx = GetCurrentLineIndex(mState.mCursorPosition);
	reCalculateBounds = true;


	if (idx != mLines[mCurrentLineIndex].size()) {
		GL_INFO("CR BETWEEN");

		std::string substr = mLines[mCurrentLineIndex].substr(idx);
		mLines[mCurrentLineIndex].erase(idx);

		mCurrentLineIndex++;
		mLines.insert(mLines.begin() + mCurrentLineIndex, substr);

	} else {
		mCurrentLineIndex++;
		mLines.insert(mLines.begin() + mCurrentLineIndex, std::string(""));
	}

	int prev_line=mCurrentLineIndex-1;
	uint8_t tabCounts=GetTabCountsUptoCursor(mState.mCursorPosition);

	if(mLines[prev_line].size()>0){
		std::string begin="";
		while(begin.size()<tabCounts) begin+='\t';

		bool isOpenParen=false;
		if(mLines[prev_line].back()=='{') {
			isOpenParen=true;
			begin+='\t';
			tabCounts++;
		}
		if(isOpenParen && mLines[mCurrentLineIndex].size() > 0 && mLines[mCurrentLineIndex].back()=='}'){
			mLines.insert(mLines.begin()+mCurrentLineIndex,begin);
			mLines[mCurrentLineIndex+1].insert(0,begin.substr(0,begin.size()-1));
		}else{
			mLines[mCurrentLineIndex].insert(0,begin.c_str());
		}
	}
	GetCurrentLineLength();
	mState.mCursorPosition.mLine++;
	mState.mCursorPosition.mColumn = tabCounts*mTabWidth;
}


void Editor::Backspace()
{
	if(mSearchState.isValid()) mSearchState.reset();
	if (mSelectionMode == SelectionMode::Word || mSelectionMode==SelectionMode::Line) {
		mSelectionMode = SelectionMode::Normal;

		if(mState.mSelectionStart > mState.mSelectionEnd)
			std::swap(mState.mSelectionStart,mState.mSelectionEnd);

		uint8_t end=GetCurrentLineIndex(mState.mSelectionEnd);
		uint8_t start=GetCurrentLineIndex(mState.mSelectionStart);

		mState.mCursorPosition = mState.mSelectionStart;
		mCurrentLineIndex=mState.mSelectionStart.mLine;


		//Words on Single Line
		if(mState.mSelectionStart.mLine==mState.mSelectionEnd.mLine){
			uint8_t word_len = end-start;
			GL_INFO("WORD LEN:{}",word_len);
			mLines[mCurrentLineIndex].erase(start, word_len);
		}

		else{

			//end
			uint8_t start=0;
			uint8_t end=GetCurrentLineIndex(mState.mSelectionEnd);
			uint8_t word_len=end-start;
			std::string remainingStr=mLines[mState.mSelectionEnd.mLine].substr(end,mLines[mState.mSelectionEnd.mLine].size()-end);
			mLines.erase(mLines.begin()+mState.mSelectionEnd.mLine);

			//start
			start=GetCurrentLineIndex(mState.mSelectionStart);
			end=mLines[mState.mSelectionStart.mLine].size();
			word_len=end-start;
			mLines[mState.mSelectionStart.mLine].erase(start, word_len);
			mLines[mState.mSelectionStart.mLine].append(remainingStr);

			if((mState.mSelectionEnd.mLine - mState.mSelectionStart.mLine) > 1)
				mLines.erase(mLines.begin()+mState.mSelectionStart.mLine+1,mLines.begin()+mState.mSelectionEnd.mLine);

		}

		return;
	}

	// Inside line
	if (mCurrentLineIndex >= 0 && mCurrentLineIndex < mLines.size() && mState.mCursorPosition.mColumn > 0 &&
	    mState.mCursorPosition.mColumn <= GetCurrentLineLength()) {

		int idx = GetCurrentLineIndex(mState.mCursorPosition);
		GL_INFO("BACKSPACE IDX:{}", idx);


		char x = mLines[mCurrentLineIndex][idx];

		if ((x == ')' || x == ']' || x == '}')) {
			bool shouldDelete = false;
			char y = mLines[mCurrentLineIndex][idx - 1];

			if (x == ')' && y == '(') shouldDelete = true;
			else if (x == ']' && y == '[') shouldDelete = true;
			else if (x == '}' && y == '{') shouldDelete = true;


			if (shouldDelete) {
				mLines[mCurrentLineIndex].erase(idx - 1, 2);
				mCurrLineLength = GetCurrentLineLength();
				mState.mCursorPosition.mColumn--;
				return;
			}

		}


		//Character Deletion
		if (mLines[mCurrentLineIndex][idx - 1] == '\t') {
			GL_INFO("TAB REMOVE");
			mLines[mCurrentLineIndex].erase(idx - 1, 1);
			mCurrLineLength = GetCurrentLineLength();
			mState.mCursorPosition.mColumn -= mTabWidth;
		} else {
			GL_INFO("CHAR REMOVE");
			mLines[mCurrentLineIndex].erase(idx - 1, 1);
			mCurrLineLength = GetCurrentLineLength();
			mState.mCursorPosition.mColumn--;
		}


	} else if (mCurrentLineIndex > 0 && mState.mCursorPosition.mColumn == 0) {
		reCalculateBounds = true;


		GL_INFO("DELETING LINE");
		if (mLines[mCurrentLineIndex].size() == 0) {
			GL_INFO("EMPTY LINE");
			mLines.erase(mLines.begin() + mCurrentLineIndex);

			mCurrentLineIndex--;
			mState.mCursorPosition.mLine--;

			mState.mCursorPosition.mColumn = GetCurrentLineLength();
		} else {
			GL_INFO("LINE BEGIN");
			int tempCursorX = GetCurrentLineLength(mCurrentLineIndex - 1);

			mLines[mCurrentLineIndex - 1] += mLines[mCurrentLineIndex];
			mLines.erase(mLines.begin() + mCurrentLineIndex);

			mCurrentLineIndex--;
			mState.mCursorPosition.mLine--;

			mState.mCursorPosition.mColumn = tempCursorX;
		}
	}
}


size_t Editor::GetCurrentLineLength(int currLineIndex)
{
	if (currLineIndex == -1) currLineIndex = mCurrentLineIndex;
	if(mLines[currLineIndex].empty()) return 0;

	mCurrLineTabCounts = 0;

	int max = mLines[currLineIndex].size();
	for (int i = 0; i < max; i++) if(mLines[currLineIndex][i] == '\t') mCurrLineTabCounts++;

	return max - mCurrLineTabCounts + (mCurrLineTabCounts * mTabWidth);
}


uint8_t Editor::GetTabCountsUptoCursor(const Coordinates& coords)const
{
	uint8_t tabCounts = 0;

	int width = 0;
	int i = 0;

	for (i = 0; width < coords.mColumn; i++) {
		if (mLines[coords.mLine][i] == '\t') {
			tabCounts++;
			width += mTabWidth;
			continue;
		}
		width++;
	}

	return tabCounts;
}


uint32_t Editor::GetCurrentLineIndex(const Coordinates& cursorPosition)const
{
	uint8_t tabCounts=GetTabCountsUptoCursor(cursorPosition);
	//(mTabWidth-1) as each tab replaced by mTabWidth containing one '/t' for each tabcount
	int val = cursorPosition.mColumn - (tabCounts * (mTabWidth - 1));
	return val > 0 ? val : 0;
}


void Editor::UpdateBounds()
{
	GL_WARN("UPDATING BOUNDS");
	mEditorPosition = mEditorWindow->Pos;
	GL_INFO("EditorPosition: x:{} y:{}",mEditorPosition.x,mEditorPosition.y);

	mEditorSize = ImVec2(mEditorWindow->ContentRegionRect.Max.x, mLines.size() * (mLineSpacing + mCharacterSize.y) + mTitleBarHeight+0.5*mLineSpacing);
	mEditorBounds = ImRect(mEditorPosition, ImVec2(mEditorPosition.x + mEditorSize.x, mEditorPosition.y + mEditorSize.y));

	reCalculateBounds = false;
}


float Editor::GetSelectionPosFromCoords(const Coordinates& coords)const{
	float offset{0.0f};
	if(coords==mState.mSelectionStart) offset=-1.0f;
	return mLinePosition.x - offset + (coords.mColumn * mCharacterSize.x);
}


bool Editor::render()
{
	static bool isInit = false;
	if (!isInit) {
		mEditorWindow = ImGui::GetCurrentWindow();
		mEditorPosition = mEditorWindow->Pos;

		mCharacterSize = ImVec2(ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr));

		mLineBarMaxCountWidth=GetNumberWidth(mLines.size());
		mLineBarWidth=ImGui::CalcTextSize(std::to_string(mLines.size()).c_str()).x + 2 * mLineBarPadding;

		mLinePosition = ImVec2({mEditorPosition.x + mLineBarWidth + mPaddingLeft, mEditorPosition.y});
		mLineHeight = mLineSpacing + mCharacterSize.y;

		mTitleBarHeight = ImGui::GetWindowHeight() - ImGui::GetContentRegionAvail().y;
		mSelectionMode = SelectionMode::Normal;

		GL_WARN("LINE HEIGHT:{}", mLineHeight);
		GL_WARN("TITLE HEIGHT:{}", mTitleBarHeight);
		isInit = true;
	}

	if (mEditorPosition.x != mEditorWindow->Pos.x || mEditorPosition.y != mEditorWindow->Pos.y) reCalculateBounds = true;
	if (reCalculateBounds) UpdateBounds();


	const ImGuiIO& io = ImGui::GetIO();
	const ImGuiID id = ImGui::GetID("##Editor");


	ImGui::ItemSize(mEditorBounds, 0.0f);
	if (!ImGui::ItemAdd(mEditorBounds, id)) return false;

	// BackGrounds
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition.x + mLineBarWidth, mEditorPosition.y}, mEditorBounds.Max,mGruvboxPalletDark[(size_t)Pallet::Background]); // Code

	if(mScrollAnimation.hasStarted){
		static bool isFirst=true;

		if(isFirst){
			mInitialScrollY=ImGui::GetScrollY();
			isFirst=false;
		}

		ImGui::SetScrollY(mInitialScrollY+(mScrollAnimation.update()*mScrollAmount));
		if(!mScrollAnimation.hasStarted) isFirst=true;
	}

	if (io.MouseWheel != 0.0f) {
		GL_INFO("SCROLLX:{} SCROLLY:{}",ImGui::GetScrollX(),ImGui::GetScrollY());
		if(mSearchState.isValid() && !mSearchState.mIsGlobal)SearchWordInCurrentVisibleBuffer();
	}


	mMinLineVisible = fmax(0.0f,ImGui::GetScrollY() / mLineHeight) ;
	mLinePosition.y = (mEditorPosition.y+mTitleBarHeight + (mLineSpacing * 0.5f) + (mState.mCursorPosition.mLine-floor(mMinLineVisible)) * mLineHeight);
	mLinePosition.x = mEditorPosition.x + mLineBarWidth + mPaddingLeft-ImGui::GetScrollX();




	if (mSelectionMode == SelectionMode::Word || mSelectionMode==SelectionMode::Line) {
		Coordinates selectionStart=mState.mSelectionStart;
		Coordinates selectionEnd=mState.mSelectionEnd;

		if(selectionStart > selectionEnd)
			std::swap(selectionStart,selectionEnd);

		if(selectionStart.mLine==selectionEnd.mLine){

			ImVec2 start(GetSelectionPosFromCoords(selectionStart), mLinePosition.y);
			ImVec2 end(GetSelectionPosFromCoords(selectionEnd), mLinePosition.y + mLineHeight);

			mEditorWindow->DrawList->AddRectFilled(start, end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);

		}else if ((selectionStart.mLine+1)==selectionEnd.mLine){

			float prevLinePositonY=mLinePosition.y;
			if(mState.mCursorDirectionChanged){
				mLinePosition.y = (mEditorPosition.y+mTitleBarHeight + (mLineSpacing * 0.5f) + (selectionEnd.mLine-floor(mMinLineVisible)) * mLineHeight);
			}

			ImVec2 start(GetSelectionPosFromCoords(selectionStart), mLinePosition.y-mLineHeight);
			ImVec2 end(mLinePosition.x+GetCurrentLineLength(selectionStart.mLine)*mCharacterSize.x+mCharacterSize.x, mLinePosition.y);

			mEditorWindow->DrawList->AddRectFilled(start, end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);


			start={mLinePosition.x, mLinePosition.y};
			end={GetSelectionPosFromCoords(selectionEnd), mLinePosition.y + mLineHeight};

			mEditorWindow->DrawList->AddRectFilled(start, end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);


			if(mState.mCursorDirectionChanged){
				mLinePosition.y = prevLinePositonY;
			}
		}else if((selectionEnd.mLine-selectionStart.mLine) > 1){
			int start=selectionStart.mLine+1;
			int end=selectionEnd.mLine;
			int diff=end-start;


			float prevLinePositonY=mLinePosition.y;
			if(mState.mCursorDirectionChanged){
				mLinePosition.y = (mEditorPosition.y+mTitleBarHeight + (mLineSpacing * 0.5f) + (selectionEnd.mLine-floor(mMinLineVisible)) * mLineHeight);
			}

			ImVec2 p_start(GetSelectionPosFromCoords(selectionStart), mLinePosition.y-(diff+1)*mLineHeight);
			ImVec2 p_end(mLinePosition.x+GetCurrentLineLength(selectionStart.mLine)*mCharacterSize.x+mCharacterSize.x, mLinePosition.y-diff*mLineHeight);

			mEditorWindow->DrawList->AddRectFilled(p_start, p_end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);


			while(start<end){
				diff=end-start;

				ImVec2 p_start(mLinePosition.x,mLinePosition.y-diff*mLineHeight);
				ImVec2 p_end(p_start.x+GetCurrentLineLength(start)*mCharacterSize.x+mCharacterSize.x,mLinePosition.y-(diff-1)*mLineHeight);

				mEditorWindow->DrawList->AddRectFilled(p_start, p_end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);
				start++;
			}


			p_start={mLinePosition.x, mLinePosition.y};
			p_end={GetSelectionPosFromCoords(selectionEnd), mLinePosition.y + mLineHeight};

			mEditorWindow->DrawList->AddRectFilled(p_start, p_end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);

			if(mState.mCursorDirectionChanged){
				mLinePosition.y = prevLinePositonY;
			}
		}
	}


	int start = std::min(int(mMinLineVisible),(int)mLines.size());
	int lineCount = (mEditorWindow->Size.y) / mLineHeight;
	int end = std::min(start + lineCount + 1,(int)mLines.size());


	int lineNo = 0;
	while (start != end) {
		float linePosY = mEditorPosition.y+mLineSpacing + (lineNo * mLineHeight) + mTitleBarHeight;
		mEditorWindow->DrawList->AddText({mLinePosition.x, linePosY}, mGruvboxPalletDark[(size_t)Pallet::Text], mLines[start].c_str());

		start++;
		lineNo++;
	}




	// Cursor
	ImVec2 cursorPosition(mLinePosition.x - 1.0f + (mState.mCursorPosition.mColumn * mCharacterSize.x), mLinePosition.y);
	mEditorWindow->DrawList->AddRectFilled(cursorPosition, {cursorPosition.x + 2.0f, cursorPosition.y + mLineHeight},ImColor(255, 255, 255, 255));




	start = std::min(int(mMinLineVisible),(int)mLines.size());
	lineCount = (mEditorWindow->Size.y) / mLineHeight;
	end = std::min(start + lineCount + 1,(int)mLines.size());



	bool isTrue=mSearchState.isValid() && mSelectionMode!=SelectionMode::Line;
	if(isTrue)
		HighlightCurrentWordInBuffer();

	//Line Number Background
	mEditorWindow->DrawList->AddRectFilled(mEditorPosition, {mEditorPosition.x + mLineBarWidth, mEditorSize.y}, mGruvboxPalletDark[(size_t)Pallet::Background]); // LineNo
	// Highlight Current Lin
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition.x,mLinePosition.y},{mEditorPosition.x+mLineBarWidth, mLinePosition.y + mLineHeight},mGruvboxPalletDark[(size_t)Pallet::Highlight]); // Code
	mLineHeight = mLineSpacing + mCharacterSize.y;



	if(isTrue){
		bool isNormalMode=mSelectionMode==SelectionMode::Normal;
		for(const auto& coord:mSearchState.mFoundPositions){
			float linePosY = (mEditorPosition.y+mTitleBarHeight  + (coord.mLine-floor(mMinLineVisible)) * mLineHeight)+0.5f*mLineSpacing;
			ImVec2 start{mEditorPosition.x,linePosY};
			ImVec2 end{mEditorPosition.x+4.0f,linePosY+(isNormalMode ? 0 : mLineHeight)};

			mEditorWindow->DrawList->AddRectFilled(start,end, mGruvboxPalletDark[(size_t)Pallet::Text]);
		}
	}

	if(ImGui::GetScrollX()>0.0f){
		ImVec2 pos_start{mEditorPosition.x+mLineBarWidth,0.0f};
		mEditorWindow->DrawList->AddRectFilledMultiColor(pos_start,{pos_start.x+10.0f,mEditorWindow->Size.y}, ImColor(19,21,21,130),ImColor(19,21,21,0),ImColor(19,21,21,0),ImColor(19,21,21,130));
	}

	lineNo = 0;
	while (start != end) {
		float linePosY =mEditorPosition.y+ mLineSpacing + (lineNo * mLineHeight) + mTitleBarHeight;
		float linePosX=mEditorPosition.x + mLineBarPadding + (mLineBarMaxCountWidth-GetNumberWidth(start+1))*mCharacterSize.x;

		mEditorWindow->DrawList->AddText({linePosX, linePosY}, (start==mCurrentLineIndex) ? mGruvboxPalletDark[(size_t)Pallet::Text] : mGruvboxPalletDark[(size_t)Pallet::Comment], std::to_string(start + 1).c_str());

		start++;
		lineNo++;
	}


	HandleKeyboardInputs();
	HandleMouseInputs();

	return true;
}


void Editor::HandleDoubleClick(){
		if (mSelectionMode == SelectionMode::Line) mSelectionMode = SelectionMode::Normal;
		else
			mSelectionMode = SelectionMode::Word;

	#ifdef GL_DEBUG
		int idx = GetCurrentLineIndex(mState.mCursorPosition);
	#endif
		auto [start_idx,end_idx] = GetIndexOfWordAtCursor(mState.mCursorPosition);
		if(start_idx==end_idx) return;
	#ifdef GL_DEBUG
		GL_WARN("SELECTION IDX:{} START:{} END:{}", idx, start_idx, end_idx);
		GL_INFO("TAB COUNT:{}", mCurrLineTabCounts);
	#endif

		mState.mSelectionStart = Coordinates(mState.mCursorPosition.mLine, start_idx + (mCurrLineTabCounts * (mTabWidth - 1)));
		mState.mSelectionEnd = Coordinates(mState.mCursorPosition.mLine, end_idx + (mCurrLineTabCounts * (mTabWidth - 1)));

		mState.mCursorPosition = mState.mSelectionEnd;
}

void Editor::HandleMouseInputs()
{
	ImGuiIO& io = ImGui::GetIO();

	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowHovered()) {
		if (!shift && !alt) {

			auto click = ImGui::IsMouseClicked(0);
			auto doubleClick = ImGui::IsMouseDoubleClicked(0);

			auto t = ImGui::GetTime();
			auto tripleClick = click && !doubleClick && (mLastClick != -1.0f && (t - mLastClick) < io.MouseDoubleClickTime);

			//Left mouse button triple click
			if (tripleClick) {
				if(mSearchState.isValid()) mSearchState.reset();
				if (!ctrl) {
					GL_INFO("TRIPLE CLICK");
					mSelectionMode=SelectionMode::Line;
					mState.mSelectionStart.mColumn=0;
					mState.mSelectionEnd.mColumn=GetCurrentLineLength();
					mState.mCursorPosition.mColumn=mState.mSelectionEnd.mColumn;
				}
				mLastClick = -1.0f;
			}

			// Left mouse button double click
			else if (doubleClick) {

				if(!ctrl) HandleDoubleClick();
				mLastClick = (float)ImGui::GetTime();

			}

			// Left mouse button click
			else if (click) {
				GL_INFO("MOUSE CLICK");

				mState.mSelectionStart=mState.mSelectionEnd=mState.mCursorPosition=MapScreenPosToCoordinates(ImGui::GetMousePos());
				mSelectionMode = SelectionMode::Normal;

				SearchWordInCurrentVisibleBuffer();

				mState.mCursorDirectionChanged=false;
				mLastClick = (float)ImGui::GetTime();
			}

			//Mouse Click And Dragging
			else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0)) {
				if((ImGui::GetMousePos().y-mEditorPosition.y) < mTitleBarHeight) return;

				io.WantCaptureMouse = true;
				if(mSearchState.isValid()) mSearchState.reset();

				mState.mCursorPosition=mState.mSelectionEnd=MapScreenPosToCoordinates(ImGui::GetMousePos());
				mSelectionMode=SelectionMode::Word;

				if (mState.mSelectionStart > mState.mSelectionEnd) mState.mCursorDirectionChanged=true;
			}
		}
	}
}

void Editor::HighlightCurrentWordInBuffer() const {
	bool isNormalMode=mSelectionMode==SelectionMode::Normal;
	int minLine=int(mMinLineVisible);
	int count=int(mEditorWindow->Size.y/mLineHeight);

	for(const Coordinates& coord:mSearchState.mFoundPositions){
		if(coord.mLine==mState.mSelectionStart.mLine && coord.mColumn==mState.mSelectionStart.mColumn) continue;
		if(mSearchState.mIsGlobal && (coord.mLine < minLine || coord.mLine > minLine+count)) break;

		float offset=(mSelectionMode==SelectionMode::Normal) ? (mCharacterSize.y+1.0f+mLineSpacing) : 0.5f*mLineSpacing;
		float linePosY = (mEditorPosition.y+mTitleBarHeight  + (coord.mLine-floor(mMinLineVisible)) * mLineHeight)+offset;

		ImVec2 start{mLinePosition.x+coord.mColumn*mCharacterSize.x-!isNormalMode,linePosY};
		ImVec2 end{start.x+mSearchState.mWord.size()*mCharacterSize.x+(!isNormalMode*2),linePosY+(isNormalMode ? 0 : mLineHeight)};
		ImDrawList* drawlist=ImGui::GetCurrentWindow()->DrawList;

		if(isNormalMode)
			drawlist->AddLine(start,end, mGruvboxPalletDark[(size_t)Pallet::Text]);
		else
			drawlist->AddRect(start,end, mGruvboxPalletDark[(size_t)Pallet::Text]);

	}
}

void Editor::FindAllOccurancesOfWord(std::string word){

	mSearchState.mIsGlobal=true;
	mSearchState.mFoundPositions.clear();

	int start = std::min(0,(int)mLines.size()-1);
	int lineCount = (mEditorWindow->Size.y) / mLineHeight;
	int end = mLines.size();

	while(start<end){
        const std::string& line = mLines[start];
        size_t wordIdx = 0;

        while ((wordIdx = line.find(word, wordIdx)) != std::string::npos) {

            size_t startIndex = wordIdx;
            size_t endIndex = wordIdx + word.length() - 1;

            if(
            	(startIndex>0 && isalnum(line[startIndex-1])) || 
            	(endIndex < line.size()-2 && isalnum(line[endIndex+1]))
            )
            {
            	wordIdx=endIndex+1;
            	continue;
            }

            GL_TRACE("Line {} : Found '{}' at [{},{}] ",start+1,word,startIndex,endIndex);

            mSearchState.mFoundPositions.push_back({start,GetColumnNumberFromIndex(startIndex,start)-1});
            wordIdx = endIndex + 1;
        }

		start++;
	}

}


void Editor::SearchWordInCurrentVisibleBuffer(){

	OpenGL::ScopedTimer timer("WordSearch");
	mSearchState.reset();

	auto [start_idx,end_idx]=GetIndexOfWordAtCursor(mState.mCursorPosition);
	if(start_idx==end_idx) return;


	int start = std::min(int(mMinLineVisible),(int)mLines.size()-1);
	int lineCount = (mEditorWindow->Size.y) / mLineHeight;
	int end = std::min(start + lineCount + 1,(int)mLines.size()-1);


	std::string currentWord=mLines[mState.mCursorPosition.mLine].substr(start_idx,end_idx-start_idx);
	mSearchState.mWord=currentWord;


	GL_WARN("Searching: {}",currentWord);


	while(start<=end){
        const std::string& line = mLines[start];
        size_t wordIdx = 0;

        while ((wordIdx = line.find(currentWord, wordIdx)) != std::string::npos) {

            size_t startIndex = wordIdx;
            size_t endIndex = wordIdx + currentWord.length() - 1;

            if(
            	(startIndex>0 && isalnum(line[startIndex-1])) || 
            	(endIndex < line.size()-2 && isalnum(line[endIndex+1]))
            )
            {
            	wordIdx=endIndex+1;
            	continue;
            }

            GL_TRACE("Line {} : Found '{}' at [{},{}] ",start+1,currentWord,startIndex,endIndex);

            mSearchState.mFoundPositions.push_back({start,GetColumnNumberFromIndex(startIndex,start)-1});
            wordIdx = endIndex + 1;
        }

		start++;
	}

}


std::pair<int,int> Editor::GetIndexOfWordAtCursor(const Coordinates& coords)const{

	int idx = GetCurrentLineIndex(coords);
	int start_idx{idx};
	int end_idx{idx};
	bool a = true, b = true;
	while ((a || b)) {
		char chr{0};
		if (start_idx == 0 || start_idx < 0) {
			a = false;
			start_idx = 0;
		} else 
			chr = mLines[coords.mLine][start_idx - 1];


		if (a && (isalnum(chr) || chr == '_')) start_idx--;
		else a = false;

		if (end_idx >= mLines[coords.mLine].size()) {

			b = false;
			end_idx = mLines[coords.mLine].size();

		} else {
			chr = mLines[coords.mLine][end_idx];

			if (b && (isalnum(chr) || chr == '_')) end_idx++;
			else b = false;

		}
	}
	return {start_idx,end_idx};	
}



Coordinates Editor::MapScreenPosToCoordinates(const ImVec2& mousePosition) {
	// OpenGL::ScopedTimer timer("MouseClick");
	Coordinates coords;

	float currentLineNo=(ImGui::GetScrollY() + (mousePosition.y-mEditorPosition.y) - (0.5f * mLineSpacing) - mTitleBarHeight) / mLineHeight;
	coords.mLine = std::max(0,(int)floor(currentLineNo - (mMinLineVisible - floor(mMinLineVisible))));	
	if(coords.mLine > mLines.size()-1) coords.mLine=mLines.size()-1;

	mLineFloatPart=currentLineNo-floor(currentLineNo);
	coords.mColumn = std::max(0,(int)round((ImGui::GetScrollX()+mousePosition.x - mEditorPosition.x - mLineBarWidth-mPaddingLeft) / mCharacterSize.x));

	//Snapping to nearest tab char
	int col=0;
	size_t i=0;
	while(i < mLines[coords.mLine].size()){
		if(mLines[coords.mLine][i]=='\t') {
			col+=mTabWidth;
			if(coords.mColumn > (col-mTabWidth) && coords.mColumn < col){
				coords.mColumn=col-mTabWidth;
				break;
			}
		}
		else col++;

		i++;
	}

	mCurrentLineIndex = coords.mLine;

	mCurrLineLength=GetCurrentLineLength();
	if(coords.mColumn > mCurrLineLength) coords.mColumn=mCurrLineLength;

	return coords;
}


int Editor::GetColumnNumberFromIndex(int idx,int lineIdx){
	if(lineIdx>=mLines.size()) return 0;
	if(idx<0) return 0;
	if(idx>mLines[lineIdx].size()-1) return GetCurrentLineLength(lineIdx);

	int column{0};
	for(int i=0;i<=idx;i++){
		if(mLines[lineIdx][i]=='\t') column+=mTabWidth;
		else column++;
	}

	return column;
}



void Editor::ScrollToLineNumber(int lineNo){

	lineNo=std::max(1,lineNo);
	if(lineNo > mLines.size()) lineNo=mLines.size();

	mCurrentLineIndex=lineNo-1;
	mState.mCursorPosition.mLine=lineNo-1;


	int lineLength=GetCurrentLineLength();
	if( lineLength < mState.mCursorPosition.mColumn) 
		mState.mCursorPosition.mColumn=lineLength;


	int totalLines=0;
	totalLines=lineNo-(int)floor(mMinLineVisible);

	float lineCount=floor((mEditorWindow->Size.y) / mLineHeight)*0.5f;
	totalLines-=lineCount;

	mScrollAmount=totalLines*mLineHeight;

	//Handling quick change in nextl
	if((ImGui::GetTime()-mLastClick)<0.5f){

		ImGui::SetScrollY(ImGui::GetScrollY()+mScrollAmount);

	}else{
		mInitialScrollY=ImGui::GetScrollY();
		mScrollAnimation.start();
	}

	mLastClick=(float)ImGui::GetTime();
	if(mSelectionMode!=SelectionMode::Normal){
		mState.mSelectionStart=mState.mSelectionEnd=mState.mCursorPosition;
	}
}


void Editor::HandleKeyboardInputs()
{
	ImGuiIO& io = ImGui::GetIO();

	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowFocused()) {
		if (ImGui::IsWindowHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);

		io.WantCaptureKeyboard = true;
		io.WantTextInput = true;

		// if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
		// 	Undo();
		// else if (!IsReadOnly() && !ctrl && !shift && alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
		// 	Undo();
		// else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y)))
		// 	Redo();
		if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) MoveUp();
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
			MoveDown();
		else if (ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
			SwapLines(true);
		else if (ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
			SwapLines(false);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
			MoveLeft(ctrl, shift);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
			MoveRight(ctrl, shift);
		else if (!alt && ctrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_D))){

			bool condition=(mSelectionMode==SelectionMode::Word) && mSearchState.mFoundPositions.empty();

			if(mSelectionMode==SelectionMode::Normal || condition){
				if(!condition) HandleDoubleClick();
				if(mState.mSelectionStart==mState.mSelectionEnd) return;
				
				if(condition){
					int start_idx=GetCurrentLineIndex(mState.mSelectionStart);
					int end_idx=GetCurrentLineIndex(mState.mSelectionEnd);

					if(start_idx>end_idx) std::swap(start_idx,end_idx);
					mSearchState.reset();
					mSearchState.mWord=mLines[mState.mCursorPosition.mLine].substr(start_idx,end_idx-start_idx);
					GL_INFO("Search: {}",mSearchState.mWord);
				}
				FindAllOccurancesOfWord(mSearchState.mWord);

				//Finding Index of Position same as currentLine to get next occurance 
				auto it=std::find_if(mSearchState.mFoundPositions.begin(),mSearchState.mFoundPositions.end(),[&](const auto& coord){
					return coord.mLine==mCurrentLineIndex;
				});

				if(it!=mSearchState.mFoundPositions.end())
					mSearchState.mIdx=std::min((int)mSearchState.mFoundPositions.size()-1,(int)std::distance(mSearchState.mFoundPositions.begin(),it)+1);
			}else{
				GL_INFO("Finding Next");
				const Coordinates& coord=mSearchState.mFoundPositions[mSearchState.mIdx];
				ScrollToLineNumber(coord.mLine+1);

				mState.mSelectionStart=mState.mSelectionEnd=coord;
				mState.mSelectionEnd.mColumn=coord.mColumn+mSearchState.mWord.size();
				GL_INFO("[{}  {} {}]",mState.mSelectionStart.mColumn,mState.mSelectionEnd.mColumn,mSearchState.mWord.size());

				mState.mCursorPosition=mState.mSelectionEnd;
				mSearchState.mIdx++;

				if(mSearchState.mIdx==mSearchState.mFoundPositions.size()) mSearchState.mIdx=0;
			}
		}else if(!alt && !ctrl && !shift && ImGui::IsKeyPressed(ImGuiKey_Escape)){
			if(mSelectionMode!=SelectionMode::Normal) mSelectionMode=SelectionMode::Normal;
		}
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
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			Copy();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
			Copy();
		else if (!IsReadOnly() && !ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			Paste();
		else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_V)))
			Paste();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_X)))
			Cut();
		else if (!ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
			Cut();
		// else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
		// 	SelectAll();
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
			InsertLine();
		else if (!IsReadOnly() && !ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab))) {

			if(mSearchState.isValid()) mSearchState.reset();

			if (mSelectionMode == SelectionMode::Normal) {
				int idx = GetCurrentLineIndex(mState.mCursorPosition);
				GL_INFO("IDX:", idx);

				mLines[mCurrentLineIndex].insert(mLines[mCurrentLineIndex].begin() + idx, 1, '\t');
				mCurrLineLength = GetCurrentLineLength();

				mState.mCursorPosition.mColumn += mTabWidth;
				return;
			}

			if(mSelectionMode==SelectionMode::Line){

				int value=shift ? -1 : 1;

				if(shift){
					if(mLines[mCurrentLineIndex][0]!='\t') return;
					if(mState.mSelectionStart.mColumn==0)
						mState.mSelectionStart.mColumn+=mTabWidth;
					mLines[mCurrentLineIndex].erase(0,1);
				}
				else
					mLines[mCurrentLineIndex].insert(mLines[mCurrentLineIndex].begin(), 1, '\t');


				mCurrLineLength = GetCurrentLineLength();
				mState.mCursorPosition.mColumn += mTabWidth*value;
				mState.mSelectionStart.mColumn += mTabWidth*value;
				mState.mSelectionEnd.mColumn +=mTabWidth*value;
				return;
			}


			if(mSelectionMode==SelectionMode::Word && mState.mSelectionStart.mLine!=mState.mSelectionEnd.mLine){
				GL_INFO("INDENTING");


				int startLine=mState.mSelectionStart.mLine;
				int endLine=mState.mSelectionEnd.mLine;

				if(startLine>endLine) std::swap(startLine,endLine);


				int value=shift ? -1 : 1;
				while(startLine<=endLine){

					if(shift){
						if(mLines[startLine][0]=='\t') mLines[startLine].erase(0,1);
					}
					else
						mLines[startLine].insert(mLines[startLine].begin(), 1, '\t');


					startLine++;
				}
				mState.mSelectionStart.mColumn += mTabWidth*value;
				mState.mSelectionEnd.mColumn +=mTabWidth*value;
				mState.mCursorPosition.mColumn+=mTabWidth*value;

			}

		}

		if (!mReadOnly && !io.InputQueueCharacters.empty()) {

			if(mSearchState.isValid()) mSearchState.reset();

			if (mSelectionMode == SelectionMode::Word) Backspace();
			auto c = io.InputQueueCharacters[0];

			if (c != 0 && (c == '\n' || c >= 32)) InsertCharacter(c);

			GL_INFO("{}", (char)c);
			io.InputQueueCharacters.resize(0);
		}
	}
}


void Editor::Copy(){
	if(mSelectionMode==SelectionMode::Normal) return;

	Coordinates selectionStart=mState.mSelectionStart;
	Coordinates selectionEnd=mState.mSelectionEnd;

	if(selectionStart>selectionEnd) std::swap(selectionStart,selectionEnd);

	if(selectionStart.mLine==selectionEnd.mLine)
	{
		uint32_t start=GetCurrentLineIndex(selectionStart);
		uint32_t end=GetCurrentLineIndex(selectionEnd);
		uint8_t word_len = end-start;

		std::string selection = mLines[mCurrentLineIndex].substr(start,word_len);
		ImGui::SetClipboardText(selection.c_str());
	}

	else{

		std::string copyStr;

		//start
		uint8_t start=GetCurrentLineIndex(selectionStart);
		uint8_t end=mLines[selectionStart.mLine].size();

		uint8_t word_len=end-start;
		copyStr+=mLines[selectionStart.mLine].substr(start,word_len); copyStr+='\n';

		int startLine=selectionStart.mLine+1;

		while(startLine < selectionEnd.mLine){
			copyStr+=mLines[startLine];
			copyStr+='\n';
			startLine++;
		}

		//end
		start=0;
		end=GetCurrentLineIndex(selectionEnd);
		word_len=end-start;

		copyStr+=mLines[selectionEnd.mLine].substr(start,word_len);

		ImGui::SetClipboardText(copyStr.c_str());
	}
}


void Editor::Paste(){
	if(mSearchState.isValid()) mSearchState.reset();
	if(mSelectionMode==SelectionMode::Word) Backspace();
	int idx=GetCurrentLineIndex(mState.mCursorPosition);

	std::string data=ImGui::GetClipboardText();
	size_t foundIndex=data.find('\n');
	bool isMultiLineText=foundIndex!=std::string::npos;

	if(!isMultiLineText){
		mLines[mCurrentLineIndex].insert(idx,data);
		mState.mCursorPosition.mColumn+=data.size();
		return;
	}


	//Inserting into currentline
	std::string segment=data.substr(0,foundIndex);

	size_t word_len=mLines[mCurrentLineIndex].size()-idx;
	std::string end_segment=mLines[mCurrentLineIndex].substr(idx,word_len);
	mLines[mCurrentLineIndex].erase(idx,word_len);

	mLines[mCurrentLineIndex].insert(idx,segment);

	std::string line;
	int lineIndex=mCurrentLineIndex+1;
	for(size_t i=(foundIndex+1);i<data.size();i++){
		if(data[i]=='\r') continue;
		if(data[i]=='\n'){
			mLines.insert(mLines.begin()+lineIndex,line);
			line.clear();
			lineIndex++;
		}
		else line+=data[i];
	}



	//last string
	mLines.insert(mLines.begin()+lineIndex,line);

	mCurrentLineIndex=lineIndex;
	mState.mCursorPosition.mColumn=GetCurrentLineLength();
	mState.mCursorPosition.mLine=mCurrentLineIndex;

	mLines[mCurrentLineIndex].append(end_segment);
}

void Editor::Cut(){
	Copy();
	Backspace();
}

void Editor::SwapLines(bool up)
{
	int value = up ? -1 : 1;

	if(mSelectionMode==SelectionMode::Normal){

		if(mCurrentLineIndex==0 && up)  return;
		if(mCurrentLineIndex==mLines.size()-1 && !up) return;

		std::swap(mLines[mCurrentLineIndex], mLines[mCurrentLineIndex + value]);

	}else{

		if(mState.mSelectionStart > mState.mSelectionEnd) std::swap(mState.mSelectionStart,mState.mSelectionEnd);

		int startLine=mState.mSelectionStart.mLine;
		int endLine=mState.mSelectionEnd.mLine;


		if(startLine==0 && up) return;
		if(endLine==mLines.size()-1 && !up) return;

		if(up){
			std::string aboveLine=mLines[startLine-1];
			mLines.erase(mLines.begin()+startLine-1);
			mLines.insert(mLines.begin()+endLine,aboveLine);
		}else{
			std::string belowLine=mLines[endLine+1];
			mLines.erase(mLines.begin()+endLine+1);
			mLines.insert(mLines.begin()+startLine,belowLine);
		}
	}

	mState.mSelectionStart.mLine += value;
	mState.mSelectionEnd.mLine += value;
	mState.mCursorPosition.mLine += value;
	mCurrentLineIndex += value;
}

void Editor::MoveUp(bool ctrl, bool shift)
{
	if(mSearchState.isValid()) mSearchState.reset();
	if(mLinePosition.y<(mTitleBarHeight*2)) 
		ImGui::SetScrollY(ImGui::GetScrollY()-mLineHeight);

	if (!shift && mSelectionMode != SelectionMode::Normal) {
		mSelectionMode = SelectionMode::Normal;
		return;
	}

	if(mCurrentLineIndex==0) return;

	mState.mCursorPosition.mLine--;
	mCurrentLineIndex--;

	mCurrLineLength = GetCurrentLineLength();
	if (mState.mCursorPosition.mColumn > mCurrLineLength) mState.mCursorPosition.mColumn = mCurrLineLength;
}

void Editor::MoveDown(bool ctrl, bool shift)
{
	if(mSearchState.isValid()) mSearchState.reset();
	if(mLinePosition.y>mEditorWindow->Size.y-(mTitleBarHeight*2))
		ImGui::SetScrollY(ImGui::GetScrollY()+mLineHeight);

	if (!shift && mSelectionMode != SelectionMode::Normal) {
		mSelectionMode = SelectionMode::Normal;
		return;
	}

	if(mCurrentLineIndex==(int)mLines.size()-1) return;
	mCurrentLineIndex++;

	mCurrLineLength = GetCurrentLineLength();
	if (mState.mCursorPosition.mColumn > mCurrLineLength) mState.mCursorPosition.mColumn = mCurrLineLength;
	mState.mCursorPosition.mLine++;
}

void Editor::MoveLeft(bool ctrl, bool shift)
{
	if(mSearchState.isValid()) mSearchState.reset();
	if (!shift && mSelectionMode != SelectionMode::Normal) {
		mSelectionMode = SelectionMode::Normal;
		if(mState.mSelectionStart < mState.mSelectionEnd)
			mState.mCursorPosition = mState.mSelectionStart;
		return;
	}

	if(shift && mSelectionMode == SelectionMode::Normal){
		mSelectionMode=SelectionMode::Word;

		mState.mSelectionEnd=mState.mCursorPosition;
		mState.mSelectionStart=mState.mCursorPosition;

		mState.mSelectionEnd.mColumn=std::max(0,--mState.mCursorPosition.mColumn);

		//Selection Started From Line Begin the -ve mColumn
		if(mState.mCursorPosition.mColumn < 0 ){
			mState.mCursorPosition.mLine--;
			mCurrentLineIndex--;

			mCurrLineLength=GetCurrentLineLength();
			mState.mCursorPosition.mColumn=mCurrLineLength;
			mState.mSelectionEnd=mState.mCursorPosition;
		}

		if (mState.mSelectionStart > mState.mSelectionEnd) mState.mCursorDirectionChanged=true;

		return;
	}


	//Doesn't consider tab's in between
	if (ctrl) {
		int idx = GetCurrentLineIndex(mState.mCursorPosition);

		// while((idx-1) > 0 && !isalnum(mLines[mCurrentLineIndex][idx-1])){
		// 	idx--;
		// 	mState.mCursorPosition.mColumn--;
		// 	if(idx==0) return;
		// }

		if (idx > 0 && isalnum(mLines[mCurrentLineIndex][idx - 1])) {
			GL_INFO("WORD JUMP LEFT");
			uint8_t count = 0;

			while (idx > 0 && isalnum(mLines[mCurrentLineIndex][idx - 1])) {
				idx--;
				count++;
			}

			mState.mCursorPosition.mColumn -= count;
			if(mSelectionMode==SelectionMode::Word) mState.mSelectionEnd=mState.mCursorPosition;

			return;
		}
	}

	if (mState.mCursorPosition.mColumn == 0 && mState.mCursorPosition.mLine>0) {

		mState.mCursorPosition.mLine--;
		mCurrentLineIndex--;

		mCurrLineLength = GetCurrentLineLength();
		mState.mCursorPosition.mColumn = mCurrLineLength;

	} else if(mState.mCursorPosition.mColumn > 0) {
		int idx=GetCurrentLineIndex(mState.mCursorPosition);

		if (idx == 0 && mLines[mCurrentLineIndex][0] == '\t') {
			mState.mCursorPosition.mColumn = 0;
			return;
		}

		if (idx > 0 && mLines[mCurrentLineIndex][idx - 1] == '\t') 
			mState.mCursorPosition.mColumn -= mTabWidth;
		else 
			mState.mCursorPosition.mColumn--;
	}

	if(shift && mSelectionMode==SelectionMode::Word) mState.mSelectionEnd=mState.mCursorPosition;
}


void Editor::MoveRight(bool ctrl, bool shift)
{
	if(mSearchState.isValid()) mSearchState.reset();
	if (!shift && mSelectionMode != SelectionMode::Normal) {
		mSelectionMode = SelectionMode::Normal;
		if(mState.mSelectionStart > mState.mSelectionEnd)
			mState.mCursorPosition = mState.mSelectionStart;
		return;
	}

	if(shift && mSelectionMode == SelectionMode::Normal){
		mSelectionMode=SelectionMode::Word;
		mState.mSelectionEnd=mState.mCursorPosition;

		mState.mSelectionStart=mState.mCursorPosition;
		mState.mSelectionEnd.mColumn=(++mState.mCursorPosition.mColumn);

		//Selection Started From Line End the Cursor mColumn > len
		if(mState.mCursorPosition.mColumn > GetCurrentLineLength()){
			mState.mCursorPosition.mLine++;
			mCurrentLineIndex++;

			mCurrLineLength=GetCurrentLineLength();
			mState.mCursorPosition.mColumn=0;
			mState.mSelectionEnd=mState.mCursorPosition;
		}

		return;
	}

	if (ctrl) {
		int idx = GetCurrentLineIndex(mState.mCursorPosition);
		// int size=mLines[mCurrentLineIndex].size();
		// while(idx < size && !isalnum(mLines[mCurrentLineIndex][idx])){
		// 	idx++;
		// 	mState.mCursorPosition.mColumn++;
		// 	if(idx==size) return;
		// }
		
		if (isalnum(mLines[mCurrentLineIndex][idx])) {
			GL_INFO("WORD JUMP RIGHT");
			uint8_t count = 0;

			while (isalnum(mLines[mCurrentLineIndex][idx])) {
				idx++;
				count++;
			}

			mState.mCursorPosition.mColumn += count;
			if(mSelectionMode==SelectionMode::Word) mState.mSelectionEnd=mState.mCursorPosition;

			return;
		}
	}


	mCurrLineLength=GetCurrentLineLength();
	if (mState.mCursorPosition.mColumn == mCurrLineLength) {

		mCurrentLineIndex++;
		mCurrLineLength = GetCurrentLineLength();

		mState.mCursorPosition.mColumn = 0;
		mState.mCursorPosition.mLine++;

	} else if (mState.mCursorPosition.mColumn < mCurrLineLength) {

		if (mState.mCursorPosition.mColumn == 0 && mLines[mCurrentLineIndex][0] == '\t') {
			mState.mCursorPosition.mColumn += mTabWidth;
		} else if (mLines[mCurrentLineIndex][GetCurrentLineIndex(mState.mCursorPosition)] == '\t') {
			mState.mCursorPosition.mColumn += mTabWidth;
		} else {
			mState.mCursorPosition.mColumn++;
		}

	}

	if(shift && mSelectionMode==SelectionMode::Word){
		mState.mSelectionEnd=mState.mCursorPosition;
	}

}




