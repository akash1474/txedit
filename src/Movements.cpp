#include "DataTypes.h"
#include "Timer.h"
#include "pch.h"
#include "Coordinates.h"
#include "Log.h"
#include "UndoManager.h"
#include "imgui.h"
#include "TextEditor.h"
#include "tree_sitter/api.h"
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <unordered_map>
#include <winnt.h>

// void Editor::SnapCursorToNearestTab(EditorState& aState){
// 	int tabCounts=GetTabCountsUptoCursor(aState.mCursorPosition);
// 	aState.mCursorPosition.mColumn=tabCounts*mTabSize;
// }

void Editor::MoveUp(bool ctrl, bool shift)
{
	if (mCursors.empty())
		mCursors.push_back(mState);

	for (auto& cursorState : mCursors) {
		if (mSearchState.isValid())
			mSearchState.reset();

		if (!shift && HasSelection()) {
			DisableSelection();
			break;
		}

		if (cursorState.mCursorPosition.mLine == 0)
			break;

		cursorState.mCursorPosition.mLine--;

		size_t lineMaxColumn = GetLineMaxColumn(cursorState.mCursorPosition.mLine);
		if (cursorState.mCursorPosition.mColumn > lineMaxColumn)
			cursorState.mCursorPosition.mColumn = lineMaxColumn;
	}
	mState = mCursors[0];
	if (mCursors.size() == 1)
		mCursors.clear();

	EnsureCursorVisible();
}

void Editor::MoveDown(bool ctrl, bool shift)
{
	if (mCursors.empty())
		mCursors.push_back(mState);

	for (auto& cursorState : mCursors) {
		if (mSearchState.isValid())
			mSearchState.reset();

		if (!shift && HasSelection()) {
			DisableSelection();
			break;
		}

		if (cursorState.mCursorPosition.mLine == (int)mLines.size() - 1)
			break;

		cursorState.mCursorPosition.mLine++;

		size_t lineLength = GetLineMaxColumn(cursorState.mCursorPosition.mLine);
		if (cursorState.mCursorPosition.mColumn > lineLength)
			cursorState.mCursorPosition.mColumn = lineLength;

		if(shift && HasSelection()){
			cursorState.mSelectionEnd=cursorState.mCursorPosition;
		}
		// SnapCursorToNearestTab(cursorState);
	}
	mState = mCursors[0];
	if (mCursors.size() == 1)
		mCursors.clear();

	EnsureCursorVisible();
}

void Editor::MoveLeft(bool ctrl, bool shift)
{
	if (mCursors.empty())
		mCursors.push_back(mState);


	SelectionMode mode = mSelectionMode;
	for (auto& cursorState : mCursors) {
		// if (mLinePosition.y < 0)
		// 	ImGui::SetScrollY(ImGui::GetScrollY() - mLineHeight);

		if (mSearchState.isValid())
			mSearchState.reset();


		if (!shift && HasSelection()) {
			mode = SelectionMode::Normal;
			if (cursorState.mSelectionStart < cursorState.mSelectionEnd)
				cursorState.mCursorPosition = cursorState.mSelectionStart;

			cursorState.mSelectionStart=cursorState.mSelectionEnd=cursorState.mCursorPosition;
			continue;
		}

		if (shift && !HasSelection()) {
			mode = SelectionMode::Normal;
			GL_INFO("No Selection");

			cursorState.mSelectionStart = cursorState.mSelectionEnd = cursorState.mCursorPosition;
			cursorState.mSelectionEnd.mColumn = std::max(0, --cursorState.mCursorPosition.mColumn);

			// Selection Started From Line Begin the -ve mColumn
			if (cursorState.mCursorPosition.mColumn < 0) {
				cursorState.mCursorPosition.mLine--;

				cursorState.mCursorPosition.mColumn = GetLineMaxColumn(cursorState.mCursorPosition.mLine);
				cursorState.mSelectionEnd = cursorState.mCursorPosition;
			}

			if (cursorState.mSelectionStart > cursorState.mSelectionEnd)
				cursorState.mCursorDirectionChanged = true;

			continue;
		}

		auto& line=mLines[cursorState.mCursorPosition.mLine];
		int idx = GetCharacterIndex(cursorState.mCursorPosition);
		// Doesn't consider tab's in between
		if (ctrl && idx >0 && isalnum(line[idx - 1].mChar) && !IsUTFSequence(line[idx - 1].mChar)) {
			GL_INFO("WORD JUMP LEFT");
			uint8_t count = 0;

			while (idx > 0 && isalnum(line[idx - 1].mChar)) {
				idx--;
				count++;
			}

			cursorState.mCursorPosition.mColumn -= count;
			if (HasSelection())
				cursorState.mSelectionEnd = cursorState.mCursorPosition;

			continue;
		}

		// Cursor at line start
		if (cursorState.mCursorPosition.mColumn == 0 && cursorState.mCursorPosition.mLine > 0) {
			cursorState.mCursorPosition.mLine--;
			cursorState.mCursorPosition.mColumn = GetLineMaxColumn(cursorState.mCursorPosition.mLine);

		} else if (cursorState.mCursorPosition.mColumn > 0) {

			int idx = GetCharacterIndex(cursorState.mCursorPosition);
			if (idx == 0 && line[0].mChar == '\t') {
				cursorState.mCursorPosition.mColumn = 0;
				continue;
			}

			if (idx > 0 && line[idx - 1].mChar == '\t')
				cursorState.mCursorPosition.mColumn -= mTabSize;
			else
				cursorState.mCursorPosition.mColumn--;
		}

		if (shift && HasSelection())
			cursorState.mSelectionEnd = cursorState.mCursorPosition;

		if (cursorState.mSelectionStart > cursorState.mSelectionEnd)
			cursorState.mCursorDirectionChanged = true;
	}

	mSelectionMode = mode;
	mState = mCursors[0];
	EnsureCursorVisible();
	if (mCursors.size() == 1)
		mCursors.clear();
}


void Editor::MoveRight(bool ctrl, bool shift)
{

	if (mCursors.empty())
		mCursors.push_back(mState);

	SelectionMode mode = mSelectionMode;

	for (auto& cursorState : mCursors) {

		Line& line=mLines[cursorState.mCursorPosition.mLine];

		if (mLinePosition.y > mEditorWindow->Size.y)
			ImGui::SetScrollY(ImGui::GetScrollY() + mLineHeight);

		if (mSearchState.isValid())
			mSearchState.reset();

		// Disable selection if only right key pressed without
		if (!shift && HasSelection()) {
			mode = SelectionMode::Normal;
			if (cursorState.mSelectionStart > cursorState.mSelectionEnd)
				cursorState.mCursorPosition = cursorState.mSelectionStart;

			cursorState.mSelectionStart=cursorState.mSelectionEnd=cursorState.mCursorPosition;
			continue;
		}

		if (shift && !HasSelection()) {
			mode = SelectionMode::Normal;
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

		int idx=GetCharacterIndex(cursorState.mCursorPosition);
		//Only execute when not at LineEnd
		if (ctrl && idx<line.size() && isalnum(line[idx].mChar) && !IsUTFSequence(line[idx].mChar)) {
			GL_INFO("WORD JUMP RIGHT");
			uint8_t count = 0;

			while (isalnum(line[idx].mChar) && !IsUTFSequence(line[idx].mChar)) {
				idx++;
				count++;
			}

			cursorState.mCursorPosition.mColumn += count;
			if (HasSelection())
				cursorState.mSelectionEnd = cursorState.mCursorPosition;

			mState = mCursors[0];
			continue;
		}


		size_t lineLength = GetLineMaxColumn(cursorState.mCursorPosition.mLine);
		if (cursorState.mCursorPosition.mColumn == lineLength) {

			cursorState.mCursorPosition.mColumn = 0;
			cursorState.mCursorPosition.mLine++;

		} else if (cursorState.mCursorPosition.mColumn < lineLength) {

			if (cursorState.mCursorPosition.mColumn == 0 && line[0].mChar == '\t') {
				cursorState.mCursorPosition.mColumn += mTabSize;
			} else if (line[GetCharacterIndex(cursorState.mCursorPosition)].mChar == '\t') {
				cursorState.mCursorPosition.mColumn += mTabSize;
			} else {
				cursorState.mCursorPosition.mColumn++;
			}
		}

		if (shift && HasSelection()) {
			cursorState.mSelectionEnd = cursorState.mCursorPosition;
		}
	}
	mSelectionMode = mode;

	mState = mCursors[0];
	EnsureCursorVisible();

	if (mCursors.size() == 1)
		mCursors.clear();
}

void Editor::Delete()
{
	assert(!mReadOnly);

	if (mLines.empty())
		return;

	// UndoRecord u;
	// u.mBefore = mState;

	if (HasSelection())
	{
		// u.mRemoved = GetSelectedText();
		// u.mRemovedStart = mState.mSelectionStart;
		// u.mRemovedEnd = mState.mSelectionEnd;

		DeleteSelection();
	}
	else
	{
		DeleteCharacter(mState,0);
	}

	// u.mAfter = mState;
	// AddUndo(u);
	EnsureCursorVisible();
}

char getClosingBracketFor(char x)
{
	switch (x) {
		case '(':
			x = ')';
			break;
		case '[':
			x = ']';
			break;
		case '{':
			x = '}';
			break;
	}
	return x;
}

uint32_t Editor::GetBufferOffset(const Coordinates& aCoords){
	uint32_t lineOffset=mLineOffset[aCoords.mLine];
	int col=aCoords.mColumn;
	for(const Glyph& glyph:mLines[aCoords.mLine]){
		lineOffset++;
		if(glyph.mChar=='\t') col-=mTabSize;
		else col--;
		if(col<=0) break;
	}

	return lineOffset;
}

static const char* ReadCallback(void* payload, uint32_t byte_offset, TSPoint position, uint32_t* bytes_read) {
    std::string* text = static_cast<std::string*>(payload);
    if (!text || byte_offset >= text->size()) {
        *bytes_read = 0;
        return nullptr;
    }
    *bytes_read = text->size() - byte_offset;
    return text->c_str() + byte_offset;
}

//Take around 2ms at max
std::string Editor::GetFullText() {
	OpenGL::ScopedTimer timer("GetFullText");
    std::string bufferContent;

    size_t totalSize = 0;
    for (const auto& line : mLines) {
        totalSize += line.size();  // +1 for newline
    }
    
    bufferContent.clear();
    bufferContent.reserve(totalSize);
    
    for (size_t i = 0; i < mLines.size(); i++) {
        for (const auto& glyph : mLines[i]) {
            bufferContent += (char)glyph.mChar;
        }
        bufferContent += '\n';
    }
    return bufferContent;
}

void Editor::DebugDisplayNearByText(){

	std::string buffer=GetNearbyLinesString(mState.mCursorPosition.mLine,1);

    TSParser* parser= ts_parser_new();
    ts_parser_set_language(parser,tree_sitter_cpp());
    TSTree* tree = ts_parser_parse_string(parser, nullptr, buffer.c_str(), buffer.size());
	TSNode node=ts_tree_root_node(tree);

	std::string output;
	PrintTree(node,buffer,output);

	ImGui::Begin("Debug NearBy Text");
	ImGui::TextUnformatted(buffer.c_str());
	ImGui::Separator();
	ImGui::TextUnformatted(output.c_str());
	ImGui::End();
}


uint32_t Editor::GetLineLengthInBytes(int aLineIdx){
	return mLines[aLineIdx].size();
}


void Editor::InsertCharacter(char chr){
	uint8_t aChar=chr;
	Coordinates& coord=mState.mCursorPosition;

	char buff[7];
	int e = ImTextCharToUtf8(buff,7, aChar);
	if (e > 0)
	{
		buff[e] = '\0';
		auto& line = mLines[coord.mLine];
		auto cindex = GetCharacterIndex(coord);

		// if (cindex < (int)line.size())
		// {
		// 	auto d = UTF8CharLength(line[cindex].mChar);

		// 	// u.mRemovedStart = mState.mCursorPosition;
		// 	// u.mRemovedEnd = Coordinates(coord.mLine, GetCharacterColumn(coord.mLine, cindex + d));
		// }

		for (auto p = buff; *p != '\0'; p++, ++cindex)
			line.insert(line.begin() + cindex, Glyph(*p, ColorSchemeIdx::Default));
		// u.mAdded = buff;

		mState.mCursorPosition.mColumn++;
		mState.mSelectionStart=mState.mSelectionEnd=mState.mCursorPosition;
	}

	UpdateSyntaxHighlighting(mState.mCursorPosition.mLine,10);
	EnsureCursorVisible();
	DebouncedReparse();
}

// Function to insert a character at a specific position in a line
// void Editor::InsertCharacter(char newChar)
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

void Editor::MoveTop(bool aShift)
{
	auto oldPos = mState.mCursorPosition;
	SetCursorPosition(Coordinates(0, 0));

	if (mState.mCursorPosition != oldPos)
	{
		if (aShift)
		{
			mState.mSelectionStart=mState.mCursorPosition;
			mState.mSelectionEnd=oldPos;
		}
		else
			mState.mSelectionStart = mState.mSelectionEnd = mState.mCursorPosition;
	}
	EnsureCursorVisible();
	// ScrollToLineNumber(mState.mCursorPosition.mLine,false);
}

void Editor::MoveBottom(bool aShift)
{
	auto oldPos = mState.mCursorPosition;
	SetCursorPosition(Coordinates((int)mLines.size() - 1, 0));

	if (mState.mCursorPosition != oldPos)
	{
		if (aShift)
		{
			mState.mSelectionEnd=mState.mCursorPosition;
			mState.mSelectionStart=oldPos;
		}
		else
			mState.mSelectionStart = mState.mSelectionEnd = mState.mCursorPosition;
	}
	EnsureCursorVisible();
	// ScrollToLineNumber(newPos.mLine,false);
}

void Editor::MoveHome(bool aShift)
{
	auto oldPos = mState.mCursorPosition;
	SetCursorPosition(Coordinates(mState.mCursorPosition.mLine, 0));

	if (mState.mCursorPosition != oldPos)
	{
		if (aShift)
		{
			mState.mSelectionStart=mState.mCursorPosition;
			mState.mSelectionEnd=oldPos;
		}
		else
			mState.mSelectionStart = mState.mSelectionEnd = mState.mCursorPosition;
	}
	EnsureCursorVisible();
}

void Editor::MoveEnd(bool aShift)
{
	auto oldPos = mState.mCursorPosition;
	SetCursorPosition(Coordinates(mState.mCursorPosition.mLine, GetLineMaxColumn(oldPos.mLine)));

	if (mState.mCursorPosition != oldPos)
	{
		if (aShift)
		{
			mState.mSelectionEnd=mState.mCursorPosition;
			mState.mSelectionStart=oldPos;
		}
		else
			mState.mSelectionStart = mState.mSelectionEnd = mState.mCursorPosition;
	}
	EnsureCursorVisible();
}

void Editor::InsertLineBreak(){
	GL_INFO("InsertLineBreak");
	if(HasSelection()) Backspace();
	Coordinates& coord=mState.mCursorPosition;


	InsertLine(coord.mLine + 1);
	auto& line = mLines[coord.mLine];
	auto& newLine = mLines[coord.mLine + 1];

	const size_t whitespaceSize = newLine.size();
	auto cindex = GetCharacterIndex(coord);

	int tabCounts=GetTabCountsUptoCursor(coord);
	int idx=tabCounts;
	while(tabCounts-->0) newLine.insert(newLine.end(), Glyph('\t',ColorSchemeIdx::Default));

	newLine.insert(newLine.end(), line.begin() + cindex, line.end());
	line.erase(line.begin() + cindex, line.begin() + line.size());

	SetCursorPosition(Coordinates(coord.mLine + 1, GetCharacterColumn(coord.mLine + 1, idx)));
	EnsureCursorVisible();
}


// // TODO: Implement the character deletion UndoRecord Storage
void Editor::DeleteCharacter(EditorState& cursor, int cidx)
{
	auto pos = GetActualCursorCoordinates();
	SetCursorPosition(pos);

	bool isBackspace=cidx==-1;
	if(isBackspace)
	{
		if (mState.mCursorPosition.mColumn == 0)
		{
			if (mState.mCursorPosition.mLine == 0)
				return;

			auto& line = mLines[mState.mCursorPosition.mLine];
			auto& prevLine = mLines[mState.mCursorPosition.mLine - 1];
			auto prevSize = GetLineMaxColumn(mState.mCursorPosition.mLine - 1);
			prevLine.insert(prevLine.end(), line.begin(), line.end());

			// ErrorMarkers etmp;
			// for (auto& i : mErrorMarkers)
			// 	etmp.insert(ErrorMarkers::value_type(i.first - 1 == mState.mCursorPosition.mLine ? i.first - 1 : i.first, i.second));
			// mErrorMarkers = std::move(etmp);

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
				return;
			}
			int len=UTF8CharLength(line[cindex].mChar);
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
			// uint32_t end=GetBufferOffset(mState.mCursorPosition);
			// uint32_t start=end + len;
			// UpdateTree(start, end);
		}

		mTextChanged = true;

	
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

		UpdateSyntaxHighlighting(pos.mLine, 1);
		EnsureCursorVisible();
	}

}


void Editor::Backspace()
{
	OpenGL::ScopedTimer timer("Editor::Backspace");
	if (mSearchState.isValid())
		mSearchState.reset();

	// UndoRecord uRecord;
	// uRecord.mBeforeState = mState;

	// Deletion of selection
	if (HasSelection()) {
		DeleteSelection();
		EnsureCursorVisible();
		return;
	}

	// Character deletion Inside line
	if (mCursors.empty()) {
		DeleteCharacter(mState);
	} else {
		assert(false && "Backspace(): MultipleCursors Not Supported!");
		for (int i = 0; i < mCursors.size(); i++) DeleteCharacter(mCursors[i], i);

		// Removing cursors with same coordinates
		mCursors.erase(std::unique(mCursors.begin(), mCursors.end(),
		                           [&](const auto& left, const auto& right) { return left.mCursorPosition == right.mCursorPosition; }),
		               mCursors.end());

		// updating mState
		mState = mCursors[0];

		// clearing mCursors if size==1
		if (mCursors.size() == 1)
			mCursors.clear();
	}
	EnsureCursorVisible();
	UpdateSyntaxHighlighting(mState.mCursorPosition.mLine,0);
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
		UpdateSyntaxHighlighting(startLine-1,endLine-startLine+1);
	}

	mState.mSelectionStart.mLine += value;
	mState.mSelectionEnd.mLine += value;
	mState.mCursorPosition.mLine += value;
	EnsureCursorVisible();
}


void Editor::InsertTab(bool isShiftPressed)
{
	if (mSearchState.isValid())
		mSearchState.reset();

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

				mLines[lineIdx].insert(mLines[lineIdx].begin() + idx, 1, Glyph('\t',ColorSchemeIdx::Default));

				mCursors[i].mCursorPosition.mColumn += mTabSize;

				for (int j = i + 1; j < mCursors.size(); j++) {
					if (mCursors[j].mCursorPosition.mLine == mCursors[i].mCursorPosition.mLine)
						mCursors[j].mCursorPosition.mColumn += mTabSize;
				}
			}
		} else {
			auto& line=mLines[mState.mCursorPosition.mLine];
			int idx = GetCharacterIndex(mState.mCursorPosition);

			if(idx>0 && line[idx-1].mChar!='\t') return;

			line.insert(line.begin() + idx, 1, Glyph('\t',ColorSchemeIdx::Default));
			mState.mCursorPosition.mColumn += mTabSize;
		}

		// UndoRecord
		// uRecord.mAddedEnd = mState.mCursorPosition;
		// uRecord.mAfterState = mState;
		// mUndoManager.AddUndo(uRecord);

		return;
	}

	if (HasSelection()) {
		GL_INFO("INDENTING");
		int startLine = mState.mSelectionStart.mLine;
		int endLine = mState.mSelectionEnd.mLine;

		if (startLine > endLine)
			std::swap(startLine, endLine);
		int indentValue = isShiftPressed ? -1 : 1;


		bool isFirst = true;

		while (startLine <= endLine) {

			if (isShiftPressed) {
				if (mLines[startLine][0].mChar == '\t')
					mLines[startLine].erase(mLines[startLine].begin());
				else
					indentValue=0;

				// uRecord.mRemovedText = '\t';
				// uRecord.mRemovedStart = Coordinates(startLine, 0);
				// uRecord.mRemovedEnd = Coordinates(startLine, 1);
				// uRecord.mAfterState = mState;
				// if (isFirst) {
					// uRecord.isBatchStart = true;
					// isFirst = false;
				// } else
					// uRecord.isBatchStart = false;

				// if (startLine == endLine)
				// 	uRecord.isBatchEnd = true;
				// mUndoManager.AddUndo(uRecord);

			} else {
				mLines[startLine].insert(mLines[startLine].begin(), 1, Glyph('\t',ColorSchemeIdx::Default));

				// uRecord.mAddedText = '\t';
				// uRecord.mAddedStart = Coordinates(startLine, 0);
				// uRecord.mAddedEnd = Coordinates(startLine, 1);
				// uRecord.mAfterState = mState;
				// if (isFirst) {
				// 	uRecord.isBatchStart = true;
				// 	isFirst = false;
				// } else
				// 	uRecord.isBatchStart = false;

				// if (startLine == endLine)
				// 	uRecord.isBatchEnd = true;
				// mUndoManager.AddUndo(uRecord);
			}

			startLine++;
		}
		mState.mSelectionStart.mColumn += mTabSize * indentValue;
		mState.mSelectionEnd.mColumn += mTabSize * indentValue;
		mState.mCursorPosition.mColumn += mTabSize * indentValue;
	}
}

void Editor::DeleteRange(const Coordinates& aStart, const Coordinates& aEnd)
{
	assert(aEnd >= aStart);
	assert(!mReadOnly);

	GL_WARN("DeleteRange({}.{})-({}.{})", aStart.mLine, aStart.mColumn, aEnd.mLine, aEnd.mColumn);

	if (aEnd == aStart)
		return;

	auto start = GetCharacterIndex(aStart);
	auto end = GetCharacterIndex(aEnd);
	GL_INFO("Start:{},End:{}",start,end);

	if (aStart.mLine == aEnd.mLine) {
		auto& line = mLines[aStart.mLine];
		auto n = GetLineMaxColumn(aStart.mLine);
		if (aEnd.mColumn >= n)
			line.erase(line.begin() + start, line.end());
		else
			line.erase(line.begin() + start, line.begin() + end);
	} else {
		auto& firstLine = mLines[aStart.mLine];
		auto& lastLine = mLines[aEnd.mLine];

		firstLine.erase(firstLine.begin() + start, firstLine.end());
		lastLine.erase(lastLine.begin(), lastLine.begin() + end);

		if (aStart.mLine < aEnd.mLine)
			firstLine.insert(firstLine.end(), lastLine.begin(), lastLine.end());

		if (aStart.mLine < aEnd.mLine)
			RemoveLine(aStart.mLine + 1, aEnd.mLine + 1);
	}

	mTextChanged = true;
}
