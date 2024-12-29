// #include "Coordinates.h"
// #include "Log.h"
// #include "UndoManager.h"
// #include "imgui.h"
#include "Coordinates.h"
#include "imgui_internal.h"
#include "pch.h"
#include "TextEditor.h"
#include "cctype"
// #include <algorithm>
// #include <unordered_map>
// #include <winnt.h>

void Editor::MoveUp(bool ctrl, bool shift)
{
	GL_INFO("MOVEUP");
	if (mCursors.empty())
		mCursors.push_back(mState);

	for (auto& cursorState : mCursors) {
		// if (mSearchState.isValid())
		// 	mSearchState.reset();

		//Note:
		// if (mLinePosition.y < (mTitleBarHeight * 2))
		// 	ImGui::SetScrollY(ImGui::GetScrollY() - mLineHeight);

		if (!shift && mSelectionMode != SelectionMode::None) {
			mSelectionMode = SelectionMode::None;
			return;
		}

		if (cursorState.mCursorPosition.mLine == 0)
			return;

		cursorState.mCursorPosition.mLine--;

		size_t lineLength = GetLineMaxColumn(cursorState.mCursorPosition.mLine);
		if (cursorState.mCursorPosition.mColumn > lineLength)
			cursorState.mCursorPosition.mColumn = lineLength;
	}
	mState = mCursors[0];
	if (mCursors.size() == 1)
		mCursors.clear();
	
	EnsureCursorVisible();
}

void Editor::MoveDown(bool ctrl, bool shift)
{
	GL_INFO("MOVEDOWN");
	if (mCursors.empty())
		mCursors.push_back(mState);

	for (auto& cursorState : mCursors) {
		// if (mSearchState.isValid())
		// 	mSearchState.reset();
		// if (mLinePosition.y > mEditorWindow->Size.y - (mTitleBarHeight * 2))
		// 	ImGui::SetScrollY(ImGui::GetScrollY() + mLineHeight);

		if (!shift && mSelectionMode != SelectionMode::None) {
			mSelectionMode = SelectionMode::None;
			return;
		}

		if (cursorState.mCursorPosition.mLine == (int)mLines.size() - 1)
			return;

		cursorState.mCursorPosition.mLine++;

		size_t lineLength = GetLineMaxColumn(cursorState.mCursorPosition.mLine);
		if (cursorState.mCursorPosition.mColumn > lineLength)
			cursorState.mCursorPosition.mColumn = lineLength;
	}
	mState = mCursors[0];
	if (mCursors.size() == 1)
		mCursors.clear();
	EnsureCursorVisible();
}

void Editor::MoveLeft(bool ctrl, bool shift)
{
	GL_INFO("MOVE-Left");
	if (mCursors.empty())
		mCursors.push_back(mState);
	SelectionMode mode = mSelectionMode;
	for (auto& cursorState : mCursors) {

		// if (mSearchState.isValid())
		// 	mSearchState.reset();


		if (!shift && HasSelection()) {
			mode = SelectionMode::None;
			ClearSelection();
			// check and remove this code if needed
			if (cursorState.mSelectionStart < cursorState.mSelectionEnd)
				cursorState.mCursorPosition = cursorState.mSelectionStart;
			continue;
		}

		if (shift && mSelectionMode == SelectionMode::Normal) {
			// mSelectionMode=SelectionMode::Word;
			mode = SelectionMode::Word;

			cursorState.mSelectionEnd = cursorState.mSelectionStart = cursorState.mCursorPosition;
			cursorState.mSelectionEnd.mColumn = std::max(0, --cursorState.mCursorPosition.mColumn);

			// Selection Started From LineBegin(0th idx) and mColumn will be -ve
			if (cursorState.mCursorPosition.mColumn < 0) {
				cursorState.mCursorPosition.mLine--;

				cursorState.mCursorPosition.mColumn = GetLineMaxColumn(cursorState.mCursorPosition.mLine);
				cursorState.mSelectionEnd = cursorState.mCursorPosition;
			}

			if (cursorState.mSelectionStart > cursorState.mSelectionEnd)
				cursorState.mCursorDirectionChanged = true;

			continue;
		}


		// Doesn't consider tab's in between
		if (ctrl) {
			int idx = 	GetCharacterIndex(cursorState.mCursorPosition);

			if (idx > 0 && std::isalnum(mLines[cursorState.mCursorPosition.mLine][idx - 1].mChar)) {
				GL_INFO("WORD JUMP LEFT");
				uint8_t count = 0;

				//FIX IT: use IsUTF8Sequence at each index
				while (idx > 0 && std::isalnum(mLines[cursorState.mCursorPosition.mLine][idx - 1].mChar)) {
					idx--;
					count++;
				}

				cursorState.mCursorPosition.mColumn -= count;
				if (mSelectionMode == SelectionMode::Word)
					cursorState.mSelectionEnd = cursorState.mCursorPosition;

				continue;
			}
		}

		// Cursor at line start
		if (cursorState.mCursorPosition.mColumn == 0 && cursorState.mCursorPosition.mLine > 0) {

			cursorState.mCursorPosition.mLine--;
			cursorState.mCursorPosition.mColumn = GetLineMaxColumn(cursorState.mCursorPosition.mLine);

		} else if (cursorState.mCursorPosition.mColumn > 0) {

			int idx = GetCharacterIndex(cursorState.mCursorPosition);
			if (idx == 0 && mLines[cursorState.mCursorPosition.mLine][0].mChar == '\t') {
				cursorState.mCursorPosition.mColumn = 0;
				continue;
			}

			if (idx > 0 && mLines[cursorState.mCursorPosition.mLine][idx - 1].mChar == '\t')
				cursorState.mCursorPosition.mColumn -= mTabSize;
			else
				cursorState.mCursorPosition.mColumn--;
		}

		if (shift && mSelectionMode == SelectionMode::Word)
			cursorState.mSelectionEnd = cursorState.mCursorPosition;
	}
	mSelectionMode = mode;
	mState = mCursors[0];
	if (mCursors.size() == 1)
		mCursors.clear();

}


void Editor::MoveRight(bool ctrl, bool shift)
{

	GL_INFO("MOVE-Right");
	if (mCursors.empty())
		mCursors.push_back(mState);

	SelectionMode mode = mSelectionMode;

	for (auto& cursorState : mCursors) {

		// if (mLinePosition.y > mEditorWindow->Size.y - (mTitleBarHeight * 2))
		// 	ImGui::SetScrollY(ImGui::GetScrollY() + mLineHeight);

		// if (mSearchState.isValid())
		// 	mSearchState.reset();

		// Disable selection if only right key pressed without
		if (!shift && HasSelection()) {
			mode = SelectionMode::None;
			if (cursorState.mSelectionStart > cursorState.mSelectionEnd)
				cursorState.mCursorPosition = cursorState.mSelectionStart;
			continue;
		}

		if (shift && mSelectionMode == SelectionMode::None) {
			// mSelectionMode=SelectionMode::Word;
			mode = SelectionMode::Word;
			cursorState.mSelectionStart = cursorState.mSelectionEnd = cursorState.mCursorPosition;
			cursorState.mSelectionEnd.mColumn = (++cursorState.mCursorPosition.mColumn);

			// Selection Started From Line End the Cursor mColumn > len
			if (cursorState.mCursorPosition.mColumn > GetLineMaxColumn(cursorState.mCursorPosition.mLine)) {
				cursorState.mCursorPosition.mLine++;

				cursorState.mCursorPosition.mColumn = 0;
				cursorState.mSelectionEnd = cursorState.mCursorPosition;
			}

			mState = mCursors[0];
			continue;
		}

		if (ctrl) {
			int idx = GetCharacterIndex(cursorState.mCursorPosition);
			// int size=mLines[mCurrentLineIndex].size();
			// while(idx < size && !isalnum(mLines[mCurrentLineIndex][idx])){
			// 	idx++;
			// 	cursorState.mCursorPosition.mColumn++;
			// 	if(idx==size) return;
			// }
			if(idx>=mLines[cursorState.mCursorPosition.mLine].size()){
				if(cursorState.mCursorPosition.mLine < mLines.size()-1){
					cursorState.mCursorPosition.mLine++;
					cursorState.mCursorPosition.mColumn=0;
					continue;
				}
			}

			if (std::isalnum(mLines[cursorState.mCursorPosition.mLine][idx].mChar)) {
				GL_INFO("WORD JUMP RIGHT");
				uint8_t count = 0;

				while (std::isalnum(mLines[cursorState.mCursorPosition.mLine][idx].mChar)) {
					idx++;
					count++;
				}

				cursorState.mCursorPosition.mColumn += count;
				if (mSelectionMode == SelectionMode::Word)
					cursorState.mSelectionEnd = cursorState.mCursorPosition;

				mState = mCursors[0];
				continue;
			}
		}


		size_t lineLength = GetLineMaxColumn(cursorState.mCursorPosition.mLine);
		if (cursorState.mCursorPosition.mColumn == lineLength) {

			cursorState.mCursorPosition.mColumn = 0;
			cursorState.mCursorPosition.mLine++;

		} else if (cursorState.mCursorPosition.mColumn < lineLength) {

			if (cursorState.mCursorPosition.mColumn == 0 && mLines[cursorState.mCursorPosition.mLine][0].mChar == '\t') {
				cursorState.mCursorPosition.mColumn += mTabSize;
			} else if (mLines[cursorState.mCursorPosition.mLine][GetCharacterIndex(cursorState.mCursorPosition)].mChar == '\t') {
				cursorState.mCursorPosition.mColumn += mTabSize;
			} else {
				cursorState.mCursorPosition.mColumn++;
			}
		}

		if (shift && mSelectionMode == SelectionMode::Word) {
			cursorState.mSelectionEnd = cursorState.mCursorPosition;
		}
	}
	mSelectionMode = mode;

	mState = mCursors[0];
	if (mCursors.size() == 1)
		mCursors.clear();
}

void Editor::Delete()
{
	assert(!mReadOnly);

	if (mLines.empty())
		return;


	if (HasSelection())
	{
		DeleteSelection();
	}
	else
	{
		auto pos = GetActualCursorCoordinates();
		SetCursorPosition(pos);
		auto& line = mLines[pos.mLine];

		if (pos.mColumn == GetLineMaxColumn(pos.mLine))
		{
			if (pos.mLine == (int)mLines.size() - 1)
				return;


			auto& nextLine = mLines[pos.mLine + 1];
			line.insert(line.end(), nextLine.begin(), nextLine.end());
			RemoveLine(pos.mLine + 1);
		}
		else
		{
			auto cindex = GetCharacterIndex(pos);

			auto d = UTF8CharLength(line[cindex].mChar);
			while (d-- > 0 && cindex < (int)line.size())
				line.erase(line.begin() + cindex);
		}

		mTextChanged = true;

		Colorize(pos.mLine, 1);
	}
}


void Editor::InsertCharacter(char chr){
	uint8_t aChar=chr;
	Coordinates& coord=mState.mCursorPosition;

	char buf[7];
	int e = ImTextCharToUtf8(buf,7, aChar);

	if (e > 0)
	{
		buf[e] = '\0';
		auto& line = mLines[coord.mLine];
		auto cindex = GetCharacterIndex(coord);

		// if (cindex < (int)line.size())
		// {
		// 	auto d = UTF8CharLength(line[cindex].mChar);

		// 	// u.mRemovedStart = mState.mCursorPosition;
		// 	// u.mRemovedEnd = Coordinates(coord.mLine, GetCharacterColumn(coord.mLine, cindex + d));
		// }

		for (auto p = buf; *p != '\0'; p++, ++cindex)
			line.insert(line.begin() + cindex, Glyph(*p, PaletteIndex::Default));
		// u.mAdded = buf;

		mState.mCursorPosition.mColumn++;
	}

	Colorize(mState.mCursorPosition.mLine - 1, 3);
	EnsureCursorVisible();
}

// // Function to insert a character at a specific position in a line
// void InsertCharacter(char newChar)
// {

// 	int ciCurrent = GetCharacterIndex(mState.mCursorPosition);
// 	GL_INFO("INSERT IDX:{}", ciCurrent);
// 	int currentLineIndex = mState.mCursorPosition.mLine;

// 	if (mState.mSelectionStart > mState.mSelectionEnd)
// 		std::swap(mState.mSelectionStart, mState.mSelectionEnd);

// 	UndoRecord uRecord;
// 	uRecord.mBeforeState = mState;
// 	uRecord.mAddedText = newChar;
// 	uRecord.mAddedStart = mState.mCursorPosition;

// 	if (mCursors.empty() && currentLineIndex >= 0 && currentLineIndex < mLines.size() && mState.mCursorPosition.mColumn >= 0 &&
// 	    ciCurrent <= mLines[currentLineIndex].size()) {

// 		bool isBracket = newChar == '(' || newChar == '{' || newChar == '[';
// 		bool isQuote = newChar == '\"' || newChar == '\'';
// 		// Fix for tab spaces
// 		if (isQuote || isBracket) {

// 			// For word and multiline selection
// 			if (mSelectionMode == SelectionMode::Word) {
// 				int ciStart = GetCharacterIndex(mState.mSelectionStart);
// 				int ciEnd = GetCharacterIndex(mState.mSelectionEnd) + 1;

// 				mLines[mState.mSelectionStart.mLine].insert(ciStart, 1, newChar);
// 				mState.mSelectionStart.mColumn++;

// 				if (isBracket)
// 					newChar = getClosingBracketFor(newChar);

// 				size_t max = mLines[mState.mSelectionEnd.mLine].size();
// 				if (ciEnd > max)
// 					ciEnd = max;

// 				// For MultiLine Selection Fix this
// 				bool isMultiLineSelection = mState.mSelectionStart.mLine != mState.mSelectionEnd.mLine;
// 				if (isMultiLineSelection) {
// 					if (ciEnd != max)
// 						ciEnd--;
// 					mState.mCursorPosition.mColumn--;
// 				}

// 				mLines[mState.mSelectionEnd.mLine].insert(ciEnd, 1, newChar);
// 				if (!isMultiLineSelection)
// 					mState.mSelectionEnd.mColumn++;
// 			} else { // For Noselection
// 				mLines[currentLineIndex].insert(ciCurrent, 1, newChar);
// 				if (isBracket)
// 					newChar = getClosingBracketFor(newChar);
// 				mLines[currentLineIndex].insert(ciCurrent + 1, 1, newChar);
// 				uRecord.mAddedText += newChar;
// 			}

// 		} else {

// 			if ((newChar == ')' || newChar == ']' || newChar == '}') && mLines[currentLineIndex][ciCurrent] == newChar) {
// 				// Avoiding ')' reentry after '(' pressed aka "()"
// 			} else {
// 				if (mSelectionMode == SelectionMode::Word) {
// 					Backspace();
// 					mSelectionMode = SelectionMode::Normal;
// 					ciCurrent = GetCharacterIndex(mState.mCursorPosition);
// 				}
// 				mLines[currentLineIndex].insert(ciCurrent, 1, newChar);
// 			}
// 		}

// 		mState.mCursorPosition.mColumn++;

// 		uRecord.mAfterState = mState;
// 		uRecord.mAddedEnd = mState.mCursorPosition;
// 		// As we need to remove 2 characeters for quotes and brackets
// 		if (isQuote || isBracket)
// 			uRecord.mAddedEnd.mColumn++;
// 		mUndoManager.AddUndo(uRecord);
// 		return;
// 	} else if (mCursors.size() > 0) {


// 		if (mSelectionMode == SelectionMode::Word)
// 			Backspace();
// 		if (mSelectionMode != SelectionMode::Normal)
// 			mSelectionMode = SelectionMode::Normal;


// 		int cidx = 0;
// 		GL_WARN("INSERT MULTIPLE CURSORS");


// 		for (auto& cursor : mCursors) {

// 			if (cursor.mSelectionStart > cursor.mSelectionEnd)
// 				std::swap(cursor.mSelectionStart, cursor.mSelectionEnd);

// 			int idx = GetCharacterIndex(cursor.mCursorPosition);
// 			if (cursor.mCursorPosition.mLine >= 0 && cursor.mCursorPosition.mLine < mLines.size() && cursor.mCursorPosition.mColumn >= 0 &&
// 			    idx <= mLines[cursor.mCursorPosition.mLine].size()) {

// 				int idx = GetCharacterIndex(cursor.mCursorPosition);
// 				int count = 0;
// 				int currentLine = cursor.mCursorPosition.mLine;
// 				GL_INFO("CIDX:{} IDX:{}", cidx, idx);
// 				if (newChar == '\"' || newChar == '\'' || newChar == '(' || newChar == '{' || newChar == '[') {

// 					char backup = newChar;


// 					if (mSelectionMode == SelectionMode::Word) {
// 						mLines[cursor.mSelectionStart.mLine].insert(GetCharacterIndex(cursor.mSelectionStart), 1, newChar);
// 						cursor.mSelectionStart.mColumn++;
// 					} else
// 						mLines[currentLine].insert(idx, 1, newChar);
// 					switch (newChar) {
// 						case '(':
// 							newChar = ')';
// 							break;
// 						case '[':
// 							newChar = ']';
// 							break;
// 						case '{':
// 							newChar = '}';
// 							break;
// 					}
// 					if (mSelectionMode == SelectionMode::Word) {
// 						mLines[cursor.mSelectionEnd.mLine].insert(GetCharacterIndex(cursor.mSelectionEnd) + 1, 1, newChar);
// 						cursor.mSelectionEnd.mColumn++;
// 					} else
// 						mLines[currentLine].insert(idx + 1, 1, newChar);

// 					count = 2;
// 					newChar = backup;
// 				} else {

// 					if ((newChar == ')' || newChar == ']' || newChar == '}') && mLines[cursor.mCursorPosition.mLine][idx] == newChar) {
// 						// Avoiding ')' reentry after '(' pressed aka "()"
// 						count = 0;
// 					} else {
// 						// if (mSelectionMode == SelectionMode::Word){
// 						// 	Backspace();
// 						// 	mSelectionMode=SelectionMode::Normal;
// 						// 	idx = GetCharacterIndex(mState.mCursorPosition);
// 						// }
// 						mLines[cursor.mCursorPosition.mLine].insert(idx, 1, newChar);
// 						count = 1;
// 					}
// 				}

// 				// Incrementing the mColumn for next cursor on the same line
// 				for (int i = cidx + 1; count > 0 && i < mCursors.size(); i++) {
// 					if (mCursors[i].mCursorPosition.mLine == cursor.mCursorPosition.mLine) {
// 						mCursors[i].mCursorPosition.mColumn += count;
// 						if (mSelectionMode == SelectionMode::Word) {
// 							mCursors[i].mSelectionStart.mColumn += count;
// 							mCursors[i].mSelectionEnd.mColumn += count;
// 						}
// 					}
// 				}

// 				cursor.mCursorPosition.mColumn++;
// 			}
// 			cidx++;
// 		}

// 		mState.mCursorPosition = mCursors[0].mCursorPosition;
// 	}
// 	// uRecord.mAfterState=mState;
// 	// uRecord.mAddedEnd=mState.mCursorPosition;
// 	// mUndoManager.AddUndo(uRecord);
// }

// void Editor::InsertLineBreak(EditorState& cursorState, int cursorIdx)
// {
// 	UndoRecord uRecord;
// 	uRecord.mBeforeState = mState;
// 	uRecord.mAddedText = '\n';
// 	uRecord.mAddedStart = mState.mCursorPosition;

// 	int idx = GetCharacterIndex(cursorState.mCursorPosition);
// 	reCalculateBounds = true;

// 	int lineIndex = cursorState.mCursorPosition.mLine;

// 	// CR Between
// 	if (idx != mLines[lineIndex].size()) {
// 		std::string substr = mLines[lineIndex].substr(idx);
// 		mLines[lineIndex].erase(idx);

// 		lineIndex++;
// 		mLines.insert(mLines.begin() + lineIndex, substr);
// 	} else {
// 		lineIndex++;
// 		mLines.insert(mLines.begin() + lineIndex, std::string(""));
// 	}


// 	int increments = 1;
// 	// Cursors - On same line
// 	if (mCursors.size() > 1) {
// 		for (int j = cursorIdx + 1; j < mCursors.size() && mCursors[j].mCursorPosition.mLine == cursorState.mCursorPosition.mLine; j++) {
// 			GL_INFO("CR SAMELINE");
// 			mCursors[j].mCursorPosition.mLine++;
// 			mCursors[j].mCursorPosition.mColumn -= cursorState.mCursorPosition.mColumn;
// 			increments++;
// 		}
// 	}

// 	cursorState.mCursorPosition.mLine++;

// 	// Cursors - Following lines
// 	if (mCursors.size() > 1) {
// 		for (int j = cursorIdx + increments; j < mCursors.size() && mCursors[j].mCursorPosition.mLine >= cursorState.mCursorPosition.mLine;
// 		     j++) {
// 			GL_INFO("CR NEXT LINES");
// 			mCursors[j].mCursorPosition.mLine++;
// 		}
// 	}


// 	cursorState.mCursorPosition.mLine--;

// 	int prev_line = lineIndex - 1;
// 	uint8_t tabCounts = GetTabCountsUptoCursor(cursorState.mCursorPosition);

// 	if (mLines[prev_line].size() > 0) {
// 		std::string begin = "";
// 		while (begin.size() < tabCounts) begin += '\t';
// 		// uRecord.mAddedText+=begin;
// 		// begin.clear();
// 		bool isOpenParen = false;
// 		if (mLines[prev_line].back() == '{') {
// 			isOpenParen = true;
// 			begin += '\t';
// 			tabCounts++;
// 		}

// 		if (isOpenParen && mLines[lineIndex].size() > 0 && mLines[lineIndex].back() == '}') {
// 			uRecord.mAddedText += begin;
// 			// uRecord.mAddedText+='\n';
// 			mLines.insert(mLines.begin() + lineIndex, begin);
// 			mLines[lineIndex + 1].insert(0, begin.substr(0, begin.size() - 1));
// 		} else {
// 			mLines[lineIndex].insert(0, begin.c_str());
// 		}
// 	}
// 	cursorState.mCursorPosition.mLine++;
// 	cursorState.mCursorPosition.mColumn = tabCounts * mTabSize;

// 	if (cursorIdx == 0)
// 		mState = cursorState;
// 	uRecord.mAfterState = mState;
// 	uRecord.mAddedEnd = mState.mCursorPosition;
// 	mUndoManager.AddUndo(uRecord);
// }


Editor::Line& Editor::InsertLine(int aIndex)
{
	auto& result = *mLines.insert(mLines.begin() + aIndex, Line());

	ErrorMarkers etmp;
	for (auto& i : mErrorMarkers)
		etmp.insert(ErrorMarkers::value_type(i.first >= aIndex ? i.first + 1 : i.first, i.second));
	mErrorMarkers = std::move(etmp);

	Breakpoints btmp;
	for (auto i : mBreakpoints)
		btmp.insert(i >= aIndex ? i + 1 : i);
	mBreakpoints = std::move(btmp);

	return result;
}


void Editor::Backspace()
{
	if (mLines.empty())
		return;


	if (HasSelection())
	{
		DeleteSelection();
	}
	else
	{
		auto pos = GetActualCursorCoordinates();
		SetCursorPosition(pos);

		if (mState.mCursorPosition.mColumn == 0)
		{
			if (mState.mCursorPosition.mLine == 0)
				return;


			auto& line = mLines[mState.mCursorPosition.mLine];
			auto& prevLine = mLines[mState.mCursorPosition.mLine - 1];
			auto prevSize = GetLineMaxColumn(mState.mCursorPosition.mLine - 1);
			prevLine.insert(prevLine.end(), line.begin(), line.end());

			ErrorMarkers etmp;
			for (auto& i : mErrorMarkers)
				etmp.insert(ErrorMarkers::value_type(i.first - 1 == mState.mCursorPosition.mLine ? i.first - 1 : i.first, i.second));
			mErrorMarkers = std::move(etmp);

			RemoveLine(mState.mCursorPosition.mLine);
			--mState.mCursorPosition.mLine;
			mState.mCursorPosition.mColumn = prevSize;
		}
		else
		{
			auto& line = mLines[mState.mCursorPosition.mLine];
			auto cindex = GetCharacterIndex(pos) - 1;
			if(line[cindex].mChar=='\t')
			{
				auto start=line.begin()+cindex;
				mLines[mState.mCursorPosition.mLine].erase(start,start+1);
				// mCurrLineLength = GetLineMaxColumn(lineIndex);
				mState.mCursorPosition.mColumn -= mTabSize;
				Colorize(mState.mCursorPosition.mLine, 1);
				return;
			}
			auto cend = cindex + 1;
			while (cindex > 0 && IsUTFSequence(line[cindex].mChar))
				--cindex;

			//if (cindex > 0 && UTF8CharLength(line[cindex].mChar) > 1)
			//	--cindex;

			--mState.mCursorPosition.mColumn;

			while (cindex < line.size() && cend-- > cindex)
			{
				line.erase(line.begin() + cindex);
			}
		}

		mTextChanged = true;

		EnsureCursorVisible();
		Colorize(mState.mCursorPosition.mLine, 1);
	}
}

void Editor::SwapLines(bool up)
{
	if (!mCursors.empty())
		return;
	int value = up ? -1 : 1;

	if (!HasSelection()) {

		if (mState.mCursorPosition.mLine == 0 && up)
			return;
		if (mState.mCursorPosition.mLine == mLines.size() - 1 && !up)
			return;

		std::swap(mLines[mState.mCursorPosition.mLine], mLines[mState.mCursorPosition.mLine + value]);

	} else {

		if (mState.mSelectionStart > mState.mSelectionEnd)
			std::swap(mState.mSelectionStart, mState.mSelectionEnd);

		int startLine = mState.mSelectionStart.mLine;
		int endLine = mState.mSelectionEnd.mLine;


		if (startLine == 0 && up)
			return;
		if (endLine == mLines.size() - 1 && !up)
			return;

		if (up) {
			Line aboveLine = mLines[startLine - 1];
			mLines.erase(mLines.begin() + startLine - 1);
			mLines.insert(mLines.begin() + endLine, aboveLine);
		} else {
			Line belowLine = mLines[endLine + 1];
			mLines.erase(mLines.begin() + endLine + 1);
			mLines.insert(mLines.begin() + startLine, belowLine);
		}
		ColorizeRange(startLine-1,endLine-startLine+1);
	}

	mState.mSelectionStart.mLine += value;
	mState.mSelectionEnd.mLine += value;
	mState.mCursorPosition.mLine += value;
}


void Editor::InsertTab(bool isShiftPressed)
{
	// if (mSearchState.isValid())
	// 	mSearchState.reset();

	// UndoRecord uRecord;
	// uRecord.mBeforeState = mState;

	if (!HasSelection()) {
		// UndoRecord
		// uRecord.mAddedStart = mState.mCursorPosition;
		// uRecord.mAddedText = '\t';

		if (!mCursors.empty()) {
			for (int i = 0; i < mCursors.size(); i++) {
				int lineIdx = mCursors[i].mCursorPosition.mLine;
				int idx = GetCharacterIndex(mCursors[i].mCursorPosition);
				GL_INFO("IDX:", idx);

				mLines[lineIdx].insert(mLines[lineIdx].begin() + idx,Glyph('\t',PaletteIndex::Background));

				mCursors[i].mCursorPosition.mColumn += mTabSize;

				for (int j = i + 1; j < mCursors.size(); j++) {
					if (mCursors[j].mCursorPosition.mLine == mCursors[i].mCursorPosition.mLine)
						mCursors[j].mCursorPosition.mColumn += mTabSize;
				}
			}
		} else {
			int idx = GetCharacterIndex(mState.mCursorPosition);
			GL_INFO("IDX:", idx);

			mLines[mState.mCursorPosition.mLine].insert(mLines[mState.mCursorPosition.mLine].begin() + idx, Glyph('\t',PaletteIndex::Background));
			mState.mCursorPosition.mColumn += mTabSize;
		}

		// UndoRecord
		// uRecord.mAddedEnd = mState.mCursorPosition;
		// uRecord.mAfterState = mState;
		// mUndoManager.AddUndo(uRecord);

		return;
	}

	if (mSelectionMode == SelectionMode::Line) {

		int value = isShiftPressed ? -1 : 1;

		if (isShiftPressed) {
			if (mLines[mState.mCursorPosition.mLine][0].mChar != '\t')
				return;

			// UndoRecord
			// uRecord.mRemovedText = '\t';
			// uRecord.mRemovedStart = Coordinates(mState.mCursorPosition.mLine, 0);
			// uRecord.mRemovedEnd = Coordinates(mState.mCursorPosition.mLine, 1);

			if (mState.mSelectionStart.mColumn == 0)
				mState.mSelectionStart.mColumn += mTabSize;

			auto& line=mLines[mState.mCursorPosition.mLine];
			line.erase(line.begin(), line.begin()+1);
		} else {
			// UndoRecord
			// uRecord.mAddedText = '\t';
			// uRecord.mAddedStart = Coordinates(mState.mCursorPosition.mLine, 0);
			// uRecord.mAddedEnd = Coordinates(mState.mCursorPosition.mLine, 1);

			auto& line=mLines[mState.mCursorPosition.mLine];
			line.insert(line.begin(), Glyph('\t',PaletteIndex::Background));
		}


		mState.mCursorPosition.mColumn += mTabSize * value;
		mState.mSelectionStart.mColumn += mTabSize * value;
		mState.mSelectionEnd.mColumn += mTabSize * value;

		// UndoRecord
		// uRecord.mAfterState = mState;
		// mUndoManager.AddUndo(uRecord);
		return;
	}


	// if (mSelectionMode == SelectionMode::Word && mState.mSelectionStart.mLine != mState.mSelectionEnd.mLine) {
	// 	GL_INFO("INDENTING");
	// 	int startLine = mState.mSelectionStart.mLine;
	// 	int endLine = mState.mSelectionEnd.mLine;

	// 	if (startLine > endLine)
	// 		std::swap(startLine, endLine);
	// 	int indentValue = isShiftPressed ? -1 : 1;


	// 	bool isFirst = true;

	// 	while (startLine <= endLine) {

	// 		if (isShiftPressed) {
	// 			if (mLines[startLine][0] == '\t')
	// 				mLines[startLine].erase(0, 1);

	// 			uRecord.mRemovedText = '\t';
	// 			uRecord.mRemovedStart = Coordinates(startLine, 0);
	// 			uRecord.mRemovedEnd = Coordinates(startLine, 1);
	// 			uRecord.mAfterState = mState;
	// 			if (isFirst) {
	// 				uRecord.isBatchStart = true;
	// 				isFirst = false;
	// 			} else
	// 				uRecord.isBatchStart = false;

	// 			if (startLine == endLine)
	// 				uRecord.isBatchEnd = true;
	// 			mUndoManager.AddUndo(uRecord);

	// 		} else {
	// 			mLines[startLine].insert(mLines[startLine].begin(), 1, '\t');

	// 			uRecord.mAddedText = '\t';
	// 			uRecord.mAddedStart = Coordinates(startLine, 0);
	// 			uRecord.mAddedEnd = Coordinates(startLine, 1);
	// 			uRecord.mAfterState = mState;
	// 			if (isFirst) {
	// 				uRecord.isBatchStart = true;
	// 				isFirst = false;
	// 			} else
	// 				uRecord.isBatchStart = false;

	// 			if (startLine == endLine)
	// 				uRecord.isBatchEnd = true;
	// 			mUndoManager.AddUndo(uRecord);
	// 		}

	// 		startLine++;
	// 	}
	// 	mState.mSelectionStart.mColumn += mTabSize * indentValue;
	// 	mState.mSelectionEnd.mColumn += mTabSize * indentValue;
	// 	mState.mCursorPosition.mColumn += mTabSize * indentValue;
	// }
}

