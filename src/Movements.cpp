#include "Coordinates.h"
#include "Log.h"
#include "UndoManager.h"
#include "imgui.h"
#include "pch.h"
#include "TextEditor.h"
#include <algorithm>
#include <unordered_map>
#include <winnt.h>

void Editor::MoveUp(bool ctrl, bool shift)
{
	if (mCursors.empty())
		mCursors.push_back(mState);

	for (auto& cursorState : mCursors) {
		if (mSearchState.isValid())
			mSearchState.reset();
		if (mLinePosition.y < (mTitleBarHeight * 2))
			ImGui::SetScrollY(ImGui::GetScrollY() - mLineHeight);

		if (!shift && mSelectionMode != SelectionMode::Normal) {
			mSelectionMode = SelectionMode::Normal;
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
}

void Editor::MoveDown(bool ctrl, bool shift)
{
	if (mCursors.empty())
		mCursors.push_back(mState);

	for (auto& cursorState : mCursors) {
		if (mSearchState.isValid())
			mSearchState.reset();
		if (mLinePosition.y > mEditorWindow->Size.y - (mTitleBarHeight * 2))
			ImGui::SetScrollY(ImGui::GetScrollY() + mLineHeight);

		if (!shift && mSelectionMode != SelectionMode::Normal) {
			mSelectionMode = SelectionMode::Normal;
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
}

void Editor::MoveLeft(bool ctrl, bool shift)
{
	if (mCursors.empty())
		mCursors.push_back(mState);
	SelectionMode mode = mSelectionMode;
	for (auto& cursorState : mCursors) {
		if (mLinePosition.y < (mTitleBarHeight * 2))
			ImGui::SetScrollY(ImGui::GetScrollY() - mLineHeight);

		if (mSearchState.isValid())
			mSearchState.reset();


		if (!shift && mSelectionMode != SelectionMode::Normal) {
			mode = SelectionMode::Normal;
			if (cursorState.mSelectionStart < cursorState.mSelectionEnd)
				cursorState.mCursorPosition = cursorState.mSelectionStart;
			continue;
		}

		if (shift && mSelectionMode == SelectionMode::Normal) {
			// mSelectionMode=SelectionMode::Word;
			mode = SelectionMode::Word;

			cursorState.mSelectionEnd = cursorState.mSelectionStart = cursorState.mCursorPosition;
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


		// Doesn't consider tab's in between
		if (ctrl) {
			int idx = GetCharacterIndex(cursorState.mCursorPosition);

			// while((idx-1) > 0 && !isalnum(mLines[mCurrentLineIndex][idx-1])){
			// 	idx--;
			// 	cursorState.mCursorPosition.mColumn--;
			// 	if(idx==0) continue;
			// }

			if (idx > 0 && isalnum(mLines[cursorState.mCursorPosition.mLine][idx - 1])) {
				GL_INFO("WORD JUMP LEFT");
				uint8_t count = 0;

				while (idx > 0 && isalnum(mLines[cursorState.mCursorPosition.mLine][idx - 1])) {
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
			if (idx == 0 && mLines[cursorState.mCursorPosition.mLine][0] == '\t') {
				cursorState.mCursorPosition.mColumn = 0;
				continue;
			}

			if (idx > 0 && mLines[cursorState.mCursorPosition.mLine][idx - 1] == '\t')
				cursorState.mCursorPosition.mColumn -= mTabWidth;
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

	if (mCursors.empty())
		mCursors.push_back(mState);

	SelectionMode mode = mSelectionMode;

	for (auto& cursorState : mCursors) {

		if (mLinePosition.y > mEditorWindow->Size.y - (mTitleBarHeight * 2))
			ImGui::SetScrollY(ImGui::GetScrollY() + mLineHeight);

		if (mSearchState.isValid())
			mSearchState.reset();

		// Disable selection if only right key pressed without
		if (!shift && mSelectionMode != SelectionMode::Normal) {
			mode = SelectionMode::Normal;
			if (cursorState.mSelectionStart > cursorState.mSelectionEnd)
				cursorState.mCursorPosition = cursorState.mSelectionStart;
			continue;
		}

		if (shift && mSelectionMode == SelectionMode::Normal) {
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

			if (isalnum(mLines[cursorState.mCursorPosition.mLine][idx])) {
				GL_INFO("WORD JUMP RIGHT");
				uint8_t count = 0;

				while (isalnum(mLines[cursorState.mCursorPosition.mLine][idx])) {
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

			if (cursorState.mCursorPosition.mColumn == 0 && mLines[cursorState.mCursorPosition.mLine][0] == '\t') {
				cursorState.mCursorPosition.mColumn += mTabWidth;
			} else if (mLines[cursorState.mCursorPosition.mLine][GetCharacterIndex(cursorState.mCursorPosition)] == '\t') {
				cursorState.mCursorPosition.mColumn += mTabWidth;
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
	if (mCursors.empty())
		mCursors.push_back(mState);

	for (size_t i = 0; i < mCursors.size(); i++) {

		EditorState& cursorState = mCursors[i];

		int idx = GetCharacterIndex(cursorState.mCursorPosition);
		int jumpSize = 0;
		int currentLine = cursorState.mCursorPosition.mLine;

		if (idx < mLines[currentLine].size()) {

			if (mLines[currentLine][idx] == '\t')
				jumpSize = mTabWidth;
			else
				jumpSize = 1;

			mLines[currentLine].erase(idx, 1);
		} else {
			// Merge next line with current
			std::string substr = mLines[currentLine + 1].substr(0);
			mLines.erase(mLines.begin() + currentLine + 1);
			mLines[currentLine] += substr;
		}

		for (size_t j = i + 1; j < mCursors.size(); j++) {
			if (currentLine == mCursors[j].mCursorPosition.mLine) {
				mCursors[j].mCursorPosition.mColumn -= jumpSize;
			}
		}
	}

	// Removing cursors with same coordinates
	mCursors.erase(std::unique(mCursors.begin(), mCursors.end(),
	                           [&](const auto& left, const auto& right) { return left.mCursorPosition == right.mCursorPosition; }),
	               mCursors.end());

	mState = mCursors[0];
	if (mCursors.size() == 1)
		mCursors.clear();
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

// Function to insert a character at a specific position in a line
void Editor::InsertCharacter(char newChar)
{

	int ciCurrent = GetCharacterIndex(mState.mCursorPosition);
	GL_INFO("INSERT IDX:{}", ciCurrent);
	int currentLineIndex = mState.mCursorPosition.mLine;

	if (mState.mSelectionStart > mState.mSelectionEnd)
		std::swap(mState.mSelectionStart, mState.mSelectionEnd);

	UndoRecord uRecord;
	uRecord.mBeforeState = mState;
	uRecord.mAddedText = newChar;
	uRecord.mAddedStart = mState.mCursorPosition;

	if (mCursors.empty() && currentLineIndex >= 0 && currentLineIndex < mLines.size() && mState.mCursorPosition.mColumn >= 0 &&
	    ciCurrent <= mLines[currentLineIndex].size()) {

		bool isBracket = newChar == '(' || newChar == '{' || newChar == '[';
		bool isQuote = newChar == '\"' || newChar == '\'';
		// Fix for tab spaces
		if (isQuote || isBracket) {

			// For word and multiline selection
			if (mSelectionMode == SelectionMode::Word) {
				int ciStart = GetCharacterIndex(mState.mSelectionStart);
				int ciEnd = GetCharacterIndex(mState.mSelectionEnd) + 1;

				mLines[mState.mSelectionStart.mLine].insert(ciStart, 1, newChar);
				mState.mSelectionStart.mColumn++;

				if (isBracket)
					newChar = getClosingBracketFor(newChar);

				size_t max = mLines[mState.mSelectionEnd.mLine].size();
				if (ciEnd > max)
					ciEnd = max;

				// For MultiLine Selection Fix this
				bool isMultiLineSelection = mState.mSelectionStart.mLine != mState.mSelectionEnd.mLine;
				if (isMultiLineSelection) {
					if (ciEnd != max)
						ciEnd--;
					mState.mCursorPosition.mColumn--;
				}

				mLines[mState.mSelectionEnd.mLine].insert(ciEnd, 1, newChar);
				if (!isMultiLineSelection)
					mState.mSelectionEnd.mColumn++;
			} else { // For Noselection
				mLines[currentLineIndex].insert(ciCurrent, 1, newChar);
				if (isBracket)
					newChar = getClosingBracketFor(newChar);
				mLines[currentLineIndex].insert(ciCurrent + 1, 1, newChar);
				uRecord.mAddedText += newChar;
			}

		} else {

			if ((newChar == ')' || newChar == ']' || newChar == '}') && mLines[currentLineIndex][ciCurrent] == newChar) {
				// Avoiding ')' reentry after '(' pressed aka "()"
			} else {
				if (mSelectionMode == SelectionMode::Word) {
					Backspace();
					mSelectionMode = SelectionMode::Normal;
					ciCurrent = GetCharacterIndex(mState.mCursorPosition);
				}
				mLines[currentLineIndex].insert(ciCurrent, 1, newChar);
			}
		}

		mState.mCursorPosition.mColumn++;

		uRecord.mAfterState = mState;
		uRecord.mAddedEnd = mState.mCursorPosition;
		// As we need to remove 2 characeters for quotes and brackets
		if (isQuote || isBracket)
			uRecord.mAddedEnd.mColumn++;
		mUndoManager.AddUndo(uRecord);
		return;
	} else if (mCursors.size() > 0) {


		if (mSelectionMode == SelectionMode::Word)
			Backspace();
		if (mSelectionMode != SelectionMode::Normal)
			mSelectionMode = SelectionMode::Normal;


		int cidx = 0;
		GL_WARN("INSERT MULTIPLE CURSORS");


		for (auto& cursor : mCursors) {

			if (cursor.mSelectionStart > cursor.mSelectionEnd)
				std::swap(cursor.mSelectionStart, cursor.mSelectionEnd);

			int idx = GetCharacterIndex(cursor.mCursorPosition);
			if (cursor.mCursorPosition.mLine >= 0 && cursor.mCursorPosition.mLine < mLines.size() && cursor.mCursorPosition.mColumn >= 0 &&
			    idx <= mLines[cursor.mCursorPosition.mLine].size()) {

				int idx = GetCharacterIndex(cursor.mCursorPosition);
				int count = 0;
				int currentLine = cursor.mCursorPosition.mLine;
				GL_INFO("CIDX:{} IDX:{}", cidx, idx);
				if (newChar == '\"' || newChar == '\'' || newChar == '(' || newChar == '{' || newChar == '[') {

					char backup = newChar;


					if (mSelectionMode == SelectionMode::Word) {
						mLines[cursor.mSelectionStart.mLine].insert(GetCharacterIndex(cursor.mSelectionStart), 1, newChar);
						cursor.mSelectionStart.mColumn++;
					} else
						mLines[currentLine].insert(idx, 1, newChar);
					switch (newChar) {
						case '(':
							newChar = ')';
							break;
						case '[':
							newChar = ']';
							break;
						case '{':
							newChar = '}';
							break;
					}
					if (mSelectionMode == SelectionMode::Word) {
						mLines[cursor.mSelectionEnd.mLine].insert(GetCharacterIndex(cursor.mSelectionEnd) + 1, 1, newChar);
						cursor.mSelectionEnd.mColumn++;
					} else
						mLines[currentLine].insert(idx + 1, 1, newChar);

					count = 2;
					newChar = backup;
				} else {

					if ((newChar == ')' || newChar == ']' || newChar == '}') && mLines[cursor.mCursorPosition.mLine][idx] == newChar) {
						// Avoiding ')' reentry after '(' pressed aka "()"
						count = 0;
					} else {
						// if (mSelectionMode == SelectionMode::Word){
						// 	Backspace();
						// 	mSelectionMode=SelectionMode::Normal;
						// 	idx = GetCharacterIndex(mState.mCursorPosition);
						// }
						mLines[cursor.mCursorPosition.mLine].insert(idx, 1, newChar);
						count = 1;
					}
				}

				// Incrementing the mColumn for next cursor on the same line
				for (int i = cidx + 1; count > 0 && i < mCursors.size(); i++) {
					if (mCursors[i].mCursorPosition.mLine == cursor.mCursorPosition.mLine) {
						mCursors[i].mCursorPosition.mColumn += count;
						if (mSelectionMode == SelectionMode::Word) {
							mCursors[i].mSelectionStart.mColumn += count;
							mCursors[i].mSelectionEnd.mColumn += count;
						}
					}
				}

				cursor.mCursorPosition.mColumn++;
			}
			cidx++;
		}

		mState.mCursorPosition = mCursors[0].mCursorPosition;
	}
	// uRecord.mAfterState=mState;
	// uRecord.mAddedEnd=mState.mCursorPosition;
	// mUndoManager.AddUndo(uRecord);
}

void Editor::InsertLineBreak(EditorState& cursorState, int cursorIdx)
{
	UndoRecord uRecord;
	uRecord.mBeforeState = mState;
	uRecord.mAddedText = '\n';
	uRecord.mAddedStart = mState.mCursorPosition;

	int idx = GetCharacterIndex(cursorState.mCursorPosition);
	reCalculateBounds = true;

	int lineIndex = cursorState.mCursorPosition.mLine;

	// CR Between
	if (idx != mLines[lineIndex].size()) {
		std::string substr = mLines[lineIndex].substr(idx);
		mLines[lineIndex].erase(idx);

		lineIndex++;
		mLines.insert(mLines.begin() + lineIndex, substr);
	} else {
		lineIndex++;
		mLines.insert(mLines.begin() + lineIndex, std::string(""));
	}


	int increments = 1;
	// Cursors - On same line
	if (mCursors.size() > 1) {
		for (int j = cursorIdx + 1; j < mCursors.size() && mCursors[j].mCursorPosition.mLine == cursorState.mCursorPosition.mLine; j++) {
			GL_INFO("CR SAMELINE");
			mCursors[j].mCursorPosition.mLine++;
			mCursors[j].mCursorPosition.mColumn -= cursorState.mCursorPosition.mColumn;
			increments++;
		}
	}

	cursorState.mCursorPosition.mLine++;

	// Cursors - Following lines
	if (mCursors.size() > 1) {
		for (int j = cursorIdx + increments; j < mCursors.size() && mCursors[j].mCursorPosition.mLine >= cursorState.mCursorPosition.mLine;
		     j++) {
			GL_INFO("CR NEXT LINES");
			mCursors[j].mCursorPosition.mLine++;
		}
	}


	cursorState.mCursorPosition.mLine--;

	int prev_line = lineIndex - 1;
	uint8_t tabCounts = GetTabCountsUptoCursor(cursorState.mCursorPosition);

	if (mLines[prev_line].size() > 0) {
		std::string begin = "";
		while (begin.size() < tabCounts) begin += '\t';
		// uRecord.mAddedText+=begin;
		// begin.clear();
		bool isOpenParen = false;
		if (mLines[prev_line].back() == '{') {
			isOpenParen = true;
			begin += '\t';
			tabCounts++;
		}

		if (isOpenParen && mLines[lineIndex].size() > 0 && mLines[lineIndex].back() == '}') {
			uRecord.mAddedText += begin;
			// uRecord.mAddedText+='\n';
			mLines.insert(mLines.begin() + lineIndex, begin);
			mLines[lineIndex + 1].insert(0, begin.substr(0, begin.size() - 1));
		} else {
			mLines[lineIndex].insert(0, begin.c_str());
		}
	}
	cursorState.mCursorPosition.mLine++;
	cursorState.mCursorPosition.mColumn = tabCounts * mTabWidth;

	if (cursorIdx == 0)
		mState = cursorState;
	uRecord.mAfterState = mState;
	uRecord.mAddedEnd = mState.mCursorPosition;
	mUndoManager.AddUndo(uRecord);
}


void Editor::InsertLine()
{
	if (mSearchState.isValid())
		mSearchState.reset();
	if (mSelectionMode != SelectionMode::Normal)
		Backspace();
	if (mCursors.empty())
		mCursors.push_back(mState);


	for (int i = 0; i < mCursors.size(); i++) InsertLineBreak(mCursors[i], i);


	mState = mCursors[0];

	int start = std::min(int(mMinLineVisible), (int)mLines.size());
	int lineCount = (mEditorWindow->Size.y) / mLineHeight;

	// Scroll if cursor goes off screen while pressing enter
	if (mMinLineVisible + lineCount - 2 <= mState.mCursorPosition.mLine)
		ImGui::SetScrollY(ImGui::GetScrollY() + mLineHeight);

	if (mCursors.size() == 1)
		mCursors.clear();
}

// TODO: Implement the character deletion UndoRecord Storage
void Editor::DeleteCharacter(EditorState& cursor, int cidx)
{
	int lineIndex = cursor.mCursorPosition.mLine;
	UndoRecord uRecord;
	uRecord.mBeforeState = mState;
	if (lineIndex >= 0 && lineIndex < mLines.size() && cursor.mCursorPosition.mColumn > 0 &&
	    cursor.mCursorPosition.mColumn <= GetLineMaxColumn(lineIndex)) {

		int idx = GetCharacterIndex(cursor.mCursorPosition);
		GL_INFO("BACKSPACE IDX:{}", idx);


		char x = mLines[lineIndex][idx];

		if ((x == ')' || x == ']' || x == '}' || x == '\"' || x == '\'')) {
			bool haveMatchingBrackets = false;
			char y = mLines[lineIndex][idx - 1];

			if (x == ')' && y == '(')
				haveMatchingBrackets = true;
			else if (x == ']' && y == '[')
				haveMatchingBrackets = true;
			else if (x == '}' && y == '{')
				haveMatchingBrackets = true;
			else if (x == '\"' && y == '\"')
				haveMatchingBrackets = true;
			else if (x == '\'' && y == '\'')
				haveMatchingBrackets = true;


			if (haveMatchingBrackets) {
				// UndoRecord
				// Deleting the 2 chars starting from idx-1
				mLines[lineIndex].erase(idx - 1, 2);
				cursor.mCursorPosition.mColumn--;

				uRecord.mRemovedStart = cursor.mCursorPosition;

				uRecord.mRemovedText = y;  // opening
				uRecord.mRemovedText += x; // closing

				uRecord.mRemovedEnd = cursor.mCursorPosition;
				uRecord.mRemovedEnd.mColumn += 2;

				uRecord.mAfterState = mState;
				mUndoManager.AddUndo(uRecord);

				// Reducing the mCol for each cursor by 2 that are located after the current cursor
				for (int i = cidx + 1; i < mCursors.size(); i++) {
					if (mCursors[i].mCursorPosition.mLine == cursor.mCursorPosition.mLine) {
						mCursors[i].mCursorPosition.mColumn -= 2;
						if (mSelectionMode == SelectionMode::Word) {
							mCursors[i].mSelectionStart.mColumn -= 2;
							mCursors[i].mSelectionEnd.mColumn -= 2;
						}
					}
				}
				return;
			}
		}


		// Character Deletion : TabSpacing
		if (mLines[lineIndex][idx - 1] == '\t') {
			GL_INFO("TAB REMOVE");

			uRecord.mRemovedEnd = cursor.mCursorPosition;
			uRecord.mRemovedText = '\t';

			mLines[lineIndex].erase(idx - 1, 1);
			// mCurrLineLength = GetLineMaxColumn(lineIndex);
			cursor.mCursorPosition.mColumn -= mTabWidth;

			uRecord.mRemovedStart = cursor.mCursorPosition;
			uRecord.mAfterState = mState;
			mUndoManager.AddUndo(uRecord);

			if (mCursors.empty())
				return;
			for (int i = cidx + 1; i < mCursors.size(); i++) {
				if (mCursors[i].mCursorPosition.mLine == cursor.mCursorPosition.mLine) {
					mCursors[i].mCursorPosition.mColumn -= mTabWidth;
					if (mSelectionMode == SelectionMode::Word) {
						mCursors[i].mSelectionStart.mColumn -= mTabWidth;
						mCursors[i].mSelectionEnd.mColumn -= mTabWidth;
					}
				}
			}
		} else { // Any Character Deletion
			int prevIdx = idx - 1;
			char prevChar = mLines[lineIndex][prevIdx];
			GL_TRACE("CHAR REMOVE [chr:{} idx:{}]", prevChar, prevIdx);

			uRecord.mRemovedEnd = cursor.mCursorPosition;
			uRecord.mRemovedText = prevChar;

			mLines[lineIndex].erase(prevIdx, 1);
			cursor.mCursorPosition.mColumn--;

			uRecord.mRemovedStart = cursor.mCursorPosition;
			uRecord.mAfterState = mState;
			mUndoManager.AddUndo(uRecord);

			if (mCursors.empty())
				return;
			for (int i = cidx + 1; i < mCursors.size() && mCursors[i].mCursorPosition.mLine == cursor.mCursorPosition.mLine; i++) {
				mCursors[i].mCursorPosition.mColumn--;
				if (mSelectionMode == SelectionMode::Word) {
					mCursors[i].mSelectionStart.mColumn--;
					mCursors[i].mSelectionEnd.mColumn--;
				}
			}
		}


	} else if (lineIndex > 0 && cursor.mCursorPosition.mColumn == 0) {
		reCalculateBounds = true;


		// Empty Line
		if (mLines[lineIndex].size() == 0) {

			uRecord.mRemovedEnd = cursor.mCursorPosition;
			uRecord.mRemovedText = '\n';

			mLines.erase(mLines.begin() + lineIndex);
			lineIndex--;
			cursor.mCursorPosition.mLine--;

			if (!mCursors.empty() && mSelectionMode == SelectionMode::Word) {
				cursor.mSelectionStart.mLine--;
				cursor.mSelectionEnd.mLine--;
			}

			cursor.mCursorPosition.mColumn = GetLineMaxColumn(lineIndex);
			uRecord.mRemovedStart = cursor.mCursorPosition;
			uRecord.mAfterState = mState;

			mUndoManager.AddUndo(uRecord);
		} else {
			// Merging the current line with the previous Line
			size_t previousLineLen = GetLineMaxColumn(lineIndex - 1);
			uRecord.mRemovedEnd = cursor.mCursorPosition;
			uRecord.mRemovedText = '\n';

			mLines[lineIndex - 1] += mLines[lineIndex];
			mLines.erase(mLines.begin() + lineIndex);

			lineIndex--;
			cursor.mCursorPosition.mLine--;

			cursor.mCursorPosition.mColumn = previousLineLen;

			// UndoRecord
			uRecord.mRemovedStart = cursor.mCursorPosition;
			uRecord.mAfterState = mState;
			mUndoManager.AddUndo(uRecord);

			for (int i = cidx + 1; i < mCursors.size() && cursor.mCursorPosition.mLine == (mCursors[i].mCursorPosition.mLine - 1); i++) {
				mCursors[i].mCursorPosition.mLine--;
				mCursors[i].mCursorPosition.mColumn += previousLineLen;

				if (mSelectionMode == SelectionMode::Word) {
					mCursors[i].mSelectionStart.mLine--;
					mCursors[i].mSelectionStart.mColumn += previousLineLen;
					mCursors[i].mSelectionEnd.mLine--;
					mCursors[i].mSelectionEnd.mColumn += previousLineLen;
				}

				cidx++;
			}
		}
		if (mCursors.empty())
			return;
		for (int i = cidx + 1; i < mCursors.size() && mCursors[i].mCursorPosition.mLine > (cursor.mCursorPosition.mLine - 1); i++) {
			mCursors[i].mCursorPosition.mLine--;
			if (mSelectionMode == SelectionMode::Word) {
				mCursors[i].mSelectionStart.mLine--;
				mCursors[i].mSelectionEnd.mLine--;
			}
		}
	}
}


void Editor::Backspace()
{
	if (mSearchState.isValid())
		mSearchState.reset();

	UndoRecord uRecord;
	uRecord.mBeforeState = mState;

	// Deletion of selection
	if (mSelectionMode == SelectionMode::Word || mSelectionMode == SelectionMode::Line) {

		// Multiple Cursors
		if (mCursors.size() > 0) {
			GL_WARN("DELETING MULTIPLE CURSORS");
			for (size_t i = 0; i < mCursors.size(); i++) {
				if (mCursors[i].mSelectionStart > mCursors[i].mSelectionEnd)
					std::swap(mCursors[i].mSelectionStart, mCursors[i].mSelectionEnd);

				uint8_t end = GetCharacterIndex(mCursors[i].mSelectionEnd);
				uint8_t start = GetCharacterIndex(mCursors[i].mSelectionStart);

				mCursors[i].mCursorPosition = mCursors[i].mSelectionStart;

				if (mCursors[i].mSelectionStart.mLine == mCursors[i].mSelectionEnd.mLine) {
					uint8_t word_len = end - start;
					GL_INFO("LINE:{} START:{} END:{} LENGTH:{}", mCursors[i].mCursorPosition.mLine, start, end, word_len);
					mLines[mCursors[i].mCursorPosition.mLine].erase(start, word_len);

					for (size_t j = i + 1; j < mCursors.size(); j++) {
						if (mCursors[j].mCursorPosition.mLine == mCursors[i].mCursorPosition.mLine) {
							mCursors[j].mSelectionStart.mColumn -= word_len;
							mCursors[j].mSelectionEnd.mColumn -= word_len;
							mCursors[j].mCursorPosition.mColumn = mCursors[j].mSelectionStart.mColumn;
						}
					}
				}
			}
			mState.mCursorPosition = mCursors[0].mCursorPosition;
			return;
		}

		// Not executed if Multiple Cursors
		if (mState.mSelectionStart > mState.mSelectionEnd)
			std::swap(mState.mSelectionStart, mState.mSelectionEnd);

		mSelectionMode = SelectionMode::Normal;
		uRecord.mRemovedText = GetSelectedText();
		uRecord.mRemovedStart = mState.mSelectionStart;
		uRecord.mRemovedEnd = mState.mSelectionEnd;

		GL_INFO("Text:{}", uRecord.mRemovedText.c_str());
		// Update the cursor position and delete the lines from mSelectionStart to mSelectionEnd
		mState.mCursorPosition = mState.mSelectionStart;
		DeleteSelection();
		mState.mCursorPosition = mState.mSelectionEnd = mState.mSelectionStart;
		uRecord.mAfterState = mState;
		mUndoManager.AddUndo(uRecord);
		return;
	}

	// Character deletion Inside line
	if (mCursors.empty()) {

		DeleteCharacter(mState);
	} else {
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
}

void Editor::SwapLines(bool up)
{
	if (!mCursors.empty())
		return;
	int value = up ? -1 : 1;

	if (mSelectionMode == SelectionMode::Normal) {

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
			std::string aboveLine = mLines[startLine - 1];
			mLines.erase(mLines.begin() + startLine - 1);
			mLines.insert(mLines.begin() + endLine, aboveLine);
		} else {
			std::string belowLine = mLines[endLine + 1];
			mLines.erase(mLines.begin() + endLine + 1);
			mLines.insert(mLines.begin() + startLine, belowLine);
		}
	}

	mState.mSelectionStart.mLine += value;
	mState.mSelectionEnd.mLine += value;
	mState.mCursorPosition.mLine += value;
}


void Editor::InsertTab(bool isShiftPressed)
{
	if (mSearchState.isValid())
		mSearchState.reset();

	UndoRecord uRecord;
	uRecord.mBeforeState = mState;

	if (mSelectionMode == SelectionMode::Normal) {
		// UndoRecord
		uRecord.mAddedStart = mState.mCursorPosition;
		uRecord.mAddedText = '\t';

		if (!mCursors.empty()) {
			for (int i = 0; i < mCursors.size(); i++) {
				int lineIdx = mCursors[i].mCursorPosition.mLine;
				int idx = GetCharacterIndex(mCursors[i].mCursorPosition);
				GL_INFO("IDX:", idx);

				mLines[lineIdx].insert(mLines[lineIdx].begin() + idx, 1, '\t');

				mCursors[i].mCursorPosition.mColumn += mTabWidth;

				for (int j = i + 1; j < mCursors.size(); j++) {
					if (mCursors[j].mCursorPosition.mLine == mCursors[i].mCursorPosition.mLine)
						mCursors[j].mCursorPosition.mColumn += mTabWidth;
				}
			}
		} else {
			int idx = GetCharacterIndex(mState.mCursorPosition);
			GL_INFO("IDX:", idx);

			mLines[mState.mCursorPosition.mLine].insert(mLines[mState.mCursorPosition.mLine].begin() + idx, 1, '\t');
			mState.mCursorPosition.mColumn += mTabWidth;
		}

		// UndoRecord
		uRecord.mAddedEnd = mState.mCursorPosition;
		uRecord.mAfterState = mState;
		mUndoManager.AddUndo(uRecord);

		return;
	}

	if (mSelectionMode == SelectionMode::Line) {

		int value = isShiftPressed ? -1 : 1;

		if (isShiftPressed) {
			if (mLines[mState.mCursorPosition.mLine][0] != '\t')
				return;

			// UndoRecord
			uRecord.mRemovedText = '\t';
			uRecord.mRemovedStart = Coordinates(mState.mCursorPosition.mLine, 0);
			uRecord.mRemovedEnd = Coordinates(mState.mCursorPosition.mLine, 1);

			if (mState.mSelectionStart.mColumn == 0)
				mState.mSelectionStart.mColumn += mTabWidth;
			mLines[mState.mCursorPosition.mLine].erase(0, 1);
		} else {
			// UndoRecord
			uRecord.mAddedText = '\t';
			uRecord.mAddedStart = Coordinates(mState.mCursorPosition.mLine, 0);
			uRecord.mAddedEnd = Coordinates(mState.mCursorPosition.mLine, 1);

			mLines[mState.mCursorPosition.mLine].insert(mLines[mState.mCursorPosition.mLine].begin(), 1, '\t');
		}


		mState.mCursorPosition.mColumn += mTabWidth * value;
		mState.mSelectionStart.mColumn += mTabWidth * value;
		mState.mSelectionEnd.mColumn += mTabWidth * value;

		// UndoRecord
		uRecord.mAfterState = mState;
		mUndoManager.AddUndo(uRecord);
		return;
	}


	if (mSelectionMode == SelectionMode::Word && mState.mSelectionStart.mLine != mState.mSelectionEnd.mLine) {
		GL_INFO("INDENTING");
		int startLine = mState.mSelectionStart.mLine;
		int endLine = mState.mSelectionEnd.mLine;

		if (startLine > endLine)
			std::swap(startLine, endLine);
		int indentValue = isShiftPressed ? -1 : 1;


		bool isFirst = true;

		while (startLine <= endLine) {

			if (isShiftPressed) {
				if (mLines[startLine][0] == '\t')
					mLines[startLine].erase(0, 1);

				uRecord.mRemovedText = '\t';
				uRecord.mRemovedStart = Coordinates(startLine, 0);
				uRecord.mRemovedEnd = Coordinates(startLine, 1);
				uRecord.mAfterState = mState;
				if (isFirst) {
					uRecord.isBatchStart = true;
					isFirst = false;
				} else
					uRecord.isBatchStart = false;

				if (startLine == endLine)
					uRecord.isBatchEnd = true;
				mUndoManager.AddUndo(uRecord);

			} else {
				mLines[startLine].insert(mLines[startLine].begin(), 1, '\t');

				uRecord.mAddedText = '\t';
				uRecord.mAddedStart = Coordinates(startLine, 0);
				uRecord.mAddedEnd = Coordinates(startLine, 1);
				uRecord.mAfterState = mState;
				if (isFirst) {
					uRecord.isBatchStart = true;
					isFirst = false;
				} else
					uRecord.isBatchStart = false;

				if (startLine == endLine)
					uRecord.isBatchEnd = true;
				mUndoManager.AddUndo(uRecord);
			}

			startLine++;
		}
		mState.mSelectionStart.mColumn += mTabWidth * indentValue;
		mState.mSelectionEnd.mColumn += mTabWidth * indentValue;
		mState.mCursorPosition.mColumn += mTabWidth * indentValue;
	}
}

void Editor::DeleteRange(const Coordinates& aStart, const Coordinates& aEnd)
{
	assert(aEnd >= aStart);

	if (aEnd == aStart)
		return;

	uint8_t start = GetCharacterIndex(aStart);
	uint8_t end = GetCharacterIndex(aEnd);
	GL_INFO("DeleteRange:({}.{})-({}.{})  IDX:({},{})", aStart.mLine, aStart.mColumn, aEnd.mLine, aEnd.mColumn, start, end);


	// Words on Single Line
	if (aStart.mLine == aEnd.mLine) {
		uint8_t word_len = end - start;
		mLines[aStart.mLine].erase(start, word_len);
		return;
	}

	// Two Lines or More Lines
	start = 0;
	end = GetCharacterIndex(aEnd);
	uint8_t word_len = end - start;
	std::string remainingStr = mLines[aEnd.mLine].substr(end, mLines[aEnd.mLine].size() - end);
	mLines.erase(mLines.begin() + aEnd.mLine);

	// start
	start = GetCharacterIndex(aStart);
	end = mLines[aStart.mLine].size();
	word_len = end - start;
	mLines[aStart.mLine].erase(start, word_len);
	mLines[aStart.mLine].append(remainingStr);

	if ((aEnd.mLine - aStart.mLine) > 1)
		mLines.erase(mLines.begin() + aStart.mLine + 1, mLines.begin() + aEnd.mLine);


	if (aStart.mLine < (int)floor(mMinLineVisible))
		ScrollToLineNumber(aStart.mLine + 1, false);

	reCalculateBounds = true;
}
