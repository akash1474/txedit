#include "pch.h"
#include "Editor.h"

#include <ctype.h>
#include <stdint.h>

#include <cctype>
#include <cmath>
#include <future>
#include <stdio.h>
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
	int idx = GetCurrentLineIndex();
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
	int idx = GetCurrentLineIndex();
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

	mState.mCursorPosition.mLine++;
	mState.mCursorPosition.mColumn = 0;

	GetCurrentLineLength();
	mLinePosition.y += mLineHeight;
}


void Editor::Backspace()
{
	if (mSelectionMode == SelectionMode::Word || mSelectionMode==SelectionMode::Line) {
		mSelectionMode = SelectionMode::Normal;

		if(mState.mSelectionStart > mState.mSelectionEnd) std::swap(mState.mSelectionStart,mState.mSelectionEnd);

		mState.mCursorPosition = mState.mSelectionEnd;
		int end=GetCurrentLineIndex();

		mState.mCursorPosition = mState.mSelectionStart;
		int start=GetCurrentLineIndex();

		uint8_t word_len = end-start;
		mLines[mCurrentLineIndex].erase(start, word_len);
		return;
	}

	// Inside line
	if (mCurrentLineIndex >= 0 && mCurrentLineIndex < mLines.size() && mState.mCursorPosition.mColumn > 0 &&
	    mState.mCursorPosition.mColumn <= GetCurrentLineLength()) {

		int idx = GetCurrentLineIndex();
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
	mCurrLineTabCounts = 0;

	int max = mLines[currLineIndex].size();
	for (int i = 0; i < max; i++) if(mLines[currLineIndex][i] == '\t') mCurrLineTabCounts++;

	return max - mCurrLineTabCounts + (mCurrLineTabCounts * mTabWidth);
}


void Editor::UpdateTabCountsUptoCursor()
{
	mCurrLineTabCounts = 0;

	int width = 0;
	int i = 0;

	for (i = 0; width < mState.mCursorPosition.mColumn; i++) {
		if (mLines[mCurrentLineIndex][i] == '\t') {
			mCurrLineTabCounts++;
			width += mTabWidth;
			continue;
		}
		width++;
	}
}


uint32_t Editor::GetCurrentLineIndex()
{
	UpdateTabCountsUptoCursor();
	//(mTabWidth-1) as each tab replaced by mTabWidth containing one '/t' for each tabcount
	int val = mState.mCursorPosition.mColumn - (mCurrLineTabCounts * (mTabWidth - 1));
	return val > 0 ? val : 0;
}


void Editor::UpdateBounds()
{
	GL_WARN("UPDATING BOUNDS");
	mEditorPosition = mEditorWindow->Pos;

	mEditorSize = ImVec2(mEditorWindow->ContentRegionRect.Max.x, mLines.size() * (mLineSpacing + mCharacterSize.y) + 50.0f);
	mEditorBounds = ImRect(mEditorPosition, ImVec2(mEditorPosition.x + mEditorSize.x, mEditorPosition.y + mEditorSize.y));

	reCalculateBounds = false;
}


float Editor::GetSelectionPosFromCoords(const Coordinates& coords)const{
	float offset{0.0f};
	if(coords==mState.mSelectionStart) offset=-1.0f;
	return mEditorPosition.x + mLineBarWidth + mPaddingLeft - offset + (coords.mColumn * mCharacterSize.x);
}

float EasingFunction(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

bool Editor::render()
{
	static bool isInit = false;
	if (!isInit) {
		mEditorWindow = ImGui::GetCurrentWindow();
		mCharacterSize = ImVec2(ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr));

		mLineBarMaxCountWidth=GetNumberWidth(mLines.size());
		mLineBarWidth=ImGui::CalcTextSize(std::to_string(mLines.size()).c_str()).x + 2 * mLineBarPadding;

		mLinePosition = ImVec2({mEditorPosition.x + mLineBarWidth + mPaddingLeft, 0});
		mLineHeight = mLineSpacing + mCharacterSize.y;
		mTitleBarHeight = ImGui::GetWindowHeight() - ImGui::GetContentRegionAvail().y;

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
	mEditorWindow->DrawList->AddRectFilled(mEditorPosition, {mEditorPosition.x + mLineBarWidth, mEditorSize.y}, mGruvboxPalletDark[(size_t)Pallet::Background]); // LineNo
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition.x + mLineBarWidth, mEditorPosition.y}, mEditorBounds.Max,mGruvboxPalletDark[(size_t)Pallet::Background]); // Code

	// static float animationTime=0.0f;
	// static float scrollDuration=4.0f;
	// static float scrollAmount=80.0f;
	// static float currentAmount=0.0f;

    // float deltaTime = 1.0f / io.Framerate;
    // animationTime += deltaTime;
    // currentAmount += deltaTime*scrollAmount;
    // float t = fminf(animationTime / scrollDuration, 1.0f);
    // float t = fminf(currentAmount / scrollAmount, 1.0f);

	// ImGui::SetScrollY(EasingFunction(t)*currentAmount);
    // if(animationTime>=scrollDuration) animationTime=0.0f;
    // if(currentAmount>=scrollAmount) currentAmount=0.0f;

	if (io.MouseWheel != 0.0f) {
		GL_INFO("SCROLLY:{} LineY:{}",ImGui::GetScrollY(),mLinePosition.y);
		// ImGui::SetScrollY(ImGui::GetScrollY()+mLineHeight*io.MouseWheel);
	}


	mMinLineVisible = fmax(0.0f,ImGui::GetScrollY() / mLineHeight) ;
	mLinePosition.y = (mTitleBarHeight + (mLineSpacing * 0.5f) + (mState.mCursorPosition.mLine-floor(mMinLineVisible)) * mLineHeight);


	// Drawing Current Lin
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition.x,mLinePosition.y},{mEditorPosition.x+mLineBarWidth, mLinePosition.y + mLineHeight},mGruvboxPalletDark[(size_t)Pallet::Highlight]); // Code
	mLineHeight = mLineSpacing + mCharacterSize.y;


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
				mLinePosition.y = (mTitleBarHeight + (mLineSpacing * 0.5f) + (selectionEnd.mLine-floor(mMinLineVisible)) * mLineHeight);
			}

			ImVec2 start(GetSelectionPosFromCoords(selectionStart), mLinePosition.y-mLineHeight);
			ImVec2 end(mEditorPosition.x+mLineBarWidth+mPaddingLeft+GetCurrentLineLength(selectionStart.mLine)*mCharacterSize.x+mCharacterSize.x, mLinePosition.y);

			mEditorWindow->DrawList->AddRectFilled(start, end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);


			start={mEditorPosition.x+mLineBarWidth+mPaddingLeft, mLinePosition.y};
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
				mLinePosition.y = (mTitleBarHeight + (mLineSpacing * 0.5f) + (selectionEnd.mLine-floor(mMinLineVisible)) * mLineHeight);
			}

			ImVec2 p_start(GetSelectionPosFromCoords(selectionStart), mLinePosition.y-(diff+1)*mLineHeight);
			ImVec2 p_end(mEditorPosition.x+mLineBarWidth+mPaddingLeft+GetCurrentLineLength(selectionStart.mLine)*mCharacterSize.x+mCharacterSize.x, mLinePosition.y-diff*mLineHeight);

			mEditorWindow->DrawList->AddRectFilled(p_start, p_end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);


			while(start<end){
				diff=end-start;

				ImVec2 p_start(mEditorPosition.x+mLineBarWidth+mPaddingLeft,mLinePosition.y-diff*mLineHeight);
				ImVec2 p_end(p_start.x+GetCurrentLineLength(start)*mCharacterSize.x+mCharacterSize.x,mLinePosition.y-(diff-1)*mLineHeight);

				mEditorWindow->DrawList->AddRectFilled(p_start, p_end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);
				start++;
			}


			p_start={mEditorPosition.x+mLineBarWidth+mPaddingLeft, mLinePosition.y};
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
		float linePosY = mLineSpacing + (lineNo * mLineHeight) + mTitleBarHeight;
		float linePosX=mEditorPosition.x + mLineBarPadding + (mLineBarMaxCountWidth-GetNumberWidth(start+1))*mCharacterSize.x;

		mEditorWindow->DrawList->AddText({linePosX, linePosY}, (start==mCurrentLineIndex) ? mGruvboxPalletDark[(size_t)Pallet::Text] : mGruvboxPalletDark[(size_t)Pallet::Comment], std::to_string(start + 1).c_str());
		mEditorWindow->DrawList->AddText({mEditorPosition.x + mLineBarWidth + mPaddingLeft, linePosY}, mGruvboxPalletDark[(size_t)Pallet::Text], mLines[start].c_str());

		start++;
		lineNo++;
	}


	// Cursor
	ImVec2 cursorPosition(mEditorPosition.x+mPaddingLeft + mLineBarWidth - 1.0f + (mState.mCursorPosition.mColumn * mCharacterSize.x), mLinePosition.y);
	mEditorWindow->DrawList->AddRectFilled(cursorPosition, {cursorPosition.x + 2.0f, cursorPosition.y + mLineHeight},ImColor(255, 255, 255, 255));

	HandleKeyboardInputs();
	HandleMouseInputs();

	return true;
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
				if (!ctrl) {
					if (mSelectionMode == SelectionMode::Line) mSelectionMode = SelectionMode::Normal;
					else
						mSelectionMode = SelectionMode::Word;

					int idx = GetCurrentLineIndex();
					int start_idx{idx};
					int end_idx{idx};
					bool a = true, b = true;
					while ((a || b)) {
						char chr{0};
						if (start_idx == 0 || start_idx < 0) {
							a = false;
							start_idx = 0;
						} else {
							chr = mLines[mCurrentLineIndex][start_idx - 1];
						}
						if (a && (isalnum(chr) || chr == '_')) start_idx--;
						else
							a = false;
						if (end_idx >= mLines[mCurrentLineIndex].size()) {
							b = false;
							end_idx = mLines[mCurrentLineIndex].size();
						} else {
							chr = mLines[mCurrentLineIndex][end_idx];
							if (b && (isalnum(chr) || chr == '_')) {
								end_idx++;
							} else {
								b = false;
							}
						}
					}
					GL_WARN("SELECTION IDX:{} START:{} END:{}", idx, start_idx, end_idx);
					GL_INFO("TAB COUNT:{}", mCurrLineTabCounts);

					mState.mSelectionStart = Coordinates(mState.mCursorPosition.mLine, start_idx + (mCurrLineTabCounts * (mTabWidth - 1)));
					mState.mSelectionEnd = Coordinates(mState.mCursorPosition.mLine, end_idx + (mCurrLineTabCounts * (mTabWidth - 1)));

					mState.mCursorPosition = mState.mSelectionEnd;
				}

				mLastClick = (float)ImGui::GetTime();
			}

			// Left mouse button click
			else if (click) {
				GL_INFO("MOUSE CLICK");

				mState.mSelectionStart=mState.mSelectionEnd=mState.mCursorPosition=MapScreenPosToCoordinates(ImGui::GetMousePos());
				mSelectionMode = SelectionMode::Normal;

				mState.mCursorDirectionChanged=false;
				mLastClick = (float)ImGui::GetTime();
			}

			//Mouse Click And Dragging
			else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0)) {
				io.WantCaptureMouse = true;

				mState.mCursorPosition=mState.mSelectionEnd=MapScreenPosToCoordinates(ImGui::GetMousePos());
				mSelectionMode=SelectionMode::Word;

				if (mState.mSelectionStart > mState.mSelectionEnd) mState.mCursorDirectionChanged=true;
			}
		}
	}
}


Editor::Coordinates Editor::MapScreenPosToCoordinates(const ImVec2& mousePosition) {
	Coordinates coords;

	float currentLineNo=(ImGui::GetScrollY() + mousePosition.y - (0.5f * mLineSpacing) - mTitleBarHeight) / mLineHeight;
	coords.mLine = (int)floor(currentLineNo - (mMinLineVisible - floor(mMinLineVisible)));	

	mLineFloatPart=currentLineNo-floor(currentLineNo);
	coords.mColumn = std::max(0,(int)round((mousePosition.x - mEditorPosition.x - mLineBarWidth-mPaddingLeft) / mCharacterSize.x));

	mCurrentLineIndex = std::max(coords.mLine,0);

	mCurrLineLength=GetCurrentLineLength();
	if(coords.mColumn > mCurrLineLength) coords.mColumn=mCurrLineLength;

	return coords;
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

			if (mSelectionMode == SelectionMode::Normal) {
				int idx = GetCurrentLineIndex();
				GL_INFO("IDX:", idx);

				mLines[mCurrentLineIndex].insert(mLines[mCurrentLineIndex].begin() + idx, 1, '\t');
				mCurrLineLength = GetCurrentLineLength();

				mState.mCursorPosition.mColumn += mTabWidth;
			}

		}

		if (!mReadOnly && !io.InputQueueCharacters.empty()) {

			if (mSelectionMode == SelectionMode::Word) Backspace();
			auto c = io.InputQueueCharacters[0];

			if (c != 0 && (c == '\n' || c >= 32)) InsertCharacter(c);

			GL_INFO("{}", (char)c);
			io.InputQueueCharacters.resize(0);
		}
	}
}


void Editor::Copy(){
	mState.mCursorPosition = mState.mSelectionStart;
	int start=GetCurrentLineIndex();

	mState.mCursorPosition = mState.mSelectionEnd;
	int end=GetCurrentLineIndex();

	uint8_t word_len = end-start;
	std::string selection = mLines[mCurrentLineIndex].substr(start,word_len);
	ImGui::SetClipboardText(selection.c_str());
}


void Editor::Paste(){
	if(mSelectionMode==SelectionMode::Word) Backspace();
	int idx=GetCurrentLineIndex();

	mLines[mCurrentLineIndex].insert(idx,ImGui::GetClipboardText());
	mState.mCursorPosition.mColumn+=strlen(ImGui::GetClipboardText());
}


void Editor::Cut(){
	Copy();
	Backspace();
}

void Editor::SwapLines(bool up)
{
	int value = up ? -1 : 1;
	std::swap(mLines[mCurrentLineIndex], mLines[mCurrentLineIndex + value]);

	mState.mCursorPosition.mLine += value;
	mCurrentLineIndex += value;
}

void Editor::MoveUp(bool ctrl, bool shift)
{
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
	if (!shift && mSelectionMode != SelectionMode::Normal) {
		mSelectionMode = SelectionMode::Normal;
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
		int idx = GetCurrentLineIndex();

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

	if (mState.mCursorPosition.mColumn == 0) {

		mState.mCursorPosition.mLine--;
		mCurrentLineIndex--;

		mCurrLineLength = GetCurrentLineLength();
		mState.mCursorPosition.mColumn = mCurrLineLength;

	} else {
		int idx=GetCurrentLineIndex();

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
	if (!shift && mSelectionMode != SelectionMode::Normal) {
		mSelectionMode = SelectionMode::Normal;
		mState.mCursorPosition = mState.mSelectionEnd;
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
		int idx = GetCurrentLineIndex();
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
		} else if (mLines[mCurrentLineIndex][GetCurrentLineIndex()] == '\t') {
			mState.mCursorPosition.mColumn += mTabWidth;
		} else {
			mState.mCursorPosition.mColumn++;
		}

	}

	if(shift && mSelectionMode==SelectionMode::Word){
		mState.mSelectionEnd=mState.mCursorPosition;
	}

}
