#include "Log.h"
#include "imgui.h"
#include "pch.h"
#include "TextEditor.h"
#include <algorithm>
#include <unordered_map>
#include <winnt.h>

void Editor::MoveUp(bool ctrl, bool shift)
{
	if(mCursors.empty()) mCursors.push_back(mState);

	for(auto& cursorState:mCursors){
		if(mSearchState.isValid()) mSearchState.reset();
		if(mLinePosition.y<(mTitleBarHeight*2)) 
			ImGui::SetScrollY(ImGui::GetScrollY()-mLineHeight);

		if (!shift && mSelectionMode != SelectionMode::Normal) {
			mSelectionMode = SelectionMode::Normal;
			return;
		}

		if(cursorState.mCursorPosition.mLine==0) return;

		cursorState.mCursorPosition.mLine--;

		int lineLength = GetCurrentLineLength(cursorState.mCursorPosition.mLine);
		if (cursorState.mCursorPosition.mColumn > lineLength) cursorState.mCursorPosition.mColumn = lineLength;
	}
	mState=mCursors[0];
	if(mCursors.size()==1) mCursors.clear();
}

void Editor::MoveDown(bool ctrl, bool shift)
{
	if(mCursors.empty()) mCursors.push_back(mState);

	for(auto& cursorState:mCursors){
		if(mSearchState.isValid()) mSearchState.reset();
		if(mLinePosition.y>mEditorWindow->Size.y-(mTitleBarHeight*2))
			ImGui::SetScrollY(ImGui::GetScrollY()+mLineHeight);

		if (!shift && mSelectionMode != SelectionMode::Normal) {
			mSelectionMode = SelectionMode::Normal;
			return;
		}

		if(cursorState.mCursorPosition.mLine==(int)mLines.size()-1) return;

		cursorState.mCursorPosition.mLine++;

		int lineLength = GetCurrentLineLength(cursorState.mCursorPosition.mLine);
		if (cursorState.mCursorPosition.mColumn > lineLength) cursorState.mCursorPosition.mColumn = lineLength;
	}
	mState=mCursors[0];
	if(mCursors.size()==1) mCursors.clear();
}

void Editor::MoveLeft(bool ctrl, bool shift)
{
	if(mCursors.empty()) mCursors.push_back(mState);
	SelectionMode mode=mSelectionMode;
	for(auto& cursorState:mCursors){
		if(mLinePosition.y<(mTitleBarHeight*2)) 
			ImGui::SetScrollY(ImGui::GetScrollY()-mLineHeight);

		if(mSearchState.isValid()) mSearchState.reset();


		if (!shift && mSelectionMode != SelectionMode::Normal) {
			mode = SelectionMode::Normal;
			if(cursorState.mSelectionStart < cursorState.mSelectionEnd)
				cursorState.mCursorPosition = cursorState.mSelectionStart;
			continue;
		}

		if(shift && mSelectionMode == SelectionMode::Normal){
			// mSelectionMode=SelectionMode::Word;
			mode=SelectionMode::Word;

			cursorState.mSelectionEnd=cursorState.mSelectionStart=cursorState.mCursorPosition;
			cursorState.mSelectionEnd.mColumn=std::max(0,--cursorState.mCursorPosition.mColumn);

			//Selection Started From Line Begin the -ve mColumn
			if(cursorState.mCursorPosition.mColumn < 0 ){
				cursorState.mCursorPosition.mLine--;

				cursorState.mCursorPosition.mColumn=GetCurrentLineLength(cursorState.mCursorPosition.mLine);
				cursorState.mSelectionEnd=cursorState.mCursorPosition;
			}

			if (cursorState.mSelectionStart > cursorState.mSelectionEnd) cursorState.mCursorDirectionChanged=true;

			continue;
		}


		//Doesn't consider tab's in between
		if (ctrl) {
			int idx = GetCurrentLineIndex(cursorState.mCursorPosition);

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
				if(mSelectionMode==SelectionMode::Word) cursorState.mSelectionEnd=cursorState.mCursorPosition;

				continue;
			}
		}

		//Cursor at line start
		if (cursorState.mCursorPosition.mColumn == 0 && cursorState.mCursorPosition.mLine>0) {

			cursorState.mCursorPosition.mLine--;
			cursorState.mCursorPosition.mColumn = GetCurrentLineLength(cursorState.mCursorPosition.mLine);

		} else if(cursorState.mCursorPosition.mColumn > 0) {

			int idx=GetCurrentLineIndex(cursorState.mCursorPosition);
			if (idx == 0 && mLines[cursorState.mCursorPosition.mLine][0] == '\t') {
				cursorState.mCursorPosition.mColumn = 0;
				continue;
			}

			if (idx > 0 && mLines[cursorState.mCursorPosition.mLine][idx - 1] == '\t') 
				cursorState.mCursorPosition.mColumn -= mTabWidth;
			else 
				cursorState.mCursorPosition.mColumn--;
		}

		if(shift && mSelectionMode==SelectionMode::Word) cursorState.mSelectionEnd=cursorState.mCursorPosition;

	}
	mSelectionMode=mode;
	mState=mCursors[0];
	if(mCursors.size()==1) mCursors.clear();
}


void Editor::MoveRight(bool ctrl, bool shift)
{

	if(mCursors.empty()) mCursors.push_back(mState);

	SelectionMode mode=mSelectionMode;

	for (auto& cursorState:mCursors) {

        if(mLinePosition.y>mEditorWindow->Size.y-(mTitleBarHeight*2))
			ImGui::SetScrollY(ImGui::GetScrollY()+mLineHeight);

		if(mSearchState.isValid()) mSearchState.reset();


		if (!shift && mSelectionMode != SelectionMode::Normal) {
			mode = SelectionMode::Normal;
			if(cursorState.mSelectionStart > cursorState.mSelectionEnd)
				cursorState.mCursorPosition = cursorState.mSelectionStart;
			continue;
		}

		if(shift && mSelectionMode == SelectionMode::Normal){
			// mSelectionMode=SelectionMode::Word;
			mode=SelectionMode::Word;
			cursorState.mSelectionEnd=cursorState.mCursorPosition;

			cursorState.mSelectionStart=cursorState.mCursorPosition;
			cursorState.mSelectionEnd.mColumn=(++cursorState.mCursorPosition.mColumn);

			//Selection Started From Line End the Cursor mColumn > len
			if(cursorState.mCursorPosition.mColumn > GetCurrentLineLength(cursorState.mCursorPosition.mLine)){
				cursorState.mCursorPosition.mLine++;

				cursorState.mCursorPosition.mColumn=0;
				cursorState.mSelectionEnd=cursorState.mCursorPosition;
			}

			mState=mCursors[0];
			continue;
		}

		if (ctrl) {
			int idx = GetCurrentLineIndex(cursorState.mCursorPosition);
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
				if(mSelectionMode==SelectionMode::Word) cursorState.mSelectionEnd=cursorState.mCursorPosition;

				mState=mCursors[0];
				continue;
			}
		}


		int lineLength=GetCurrentLineLength(cursorState.mCursorPosition.mLine);
		if (cursorState.mCursorPosition.mColumn == lineLength) {

			cursorState.mCursorPosition.mColumn = 0;
			cursorState.mCursorPosition.mLine++;

		} else if (cursorState.mCursorPosition.mColumn < lineLength) {

			if (cursorState.mCursorPosition.mColumn == 0 && mLines[cursorState.mCursorPosition.mLine][0] == '\t') {
				cursorState.mCursorPosition.mColumn += mTabWidth;
			} else if (mLines[cursorState.mCursorPosition.mLine][GetCurrentLineIndex(cursorState.mCursorPosition)] == '\t') {
				cursorState.mCursorPosition.mColumn += mTabWidth;
			} else {
				cursorState.mCursorPosition.mColumn++;
			}

		}

		if(shift && mSelectionMode==SelectionMode::Word){
			cursorState.mSelectionEnd=cursorState.mCursorPosition;
		}

    }
    mSelectionMode=mode;

	mState=mCursors[0];
	if(mCursors.size()==1) mCursors.clear();

}

void Editor::Delete(){
	if(mCursors.empty()) mCursors.push_back(mState);

	for(size_t i=0;i<mCursors.size();i++){

		EditorState& cursorState=mCursors[i];

		int idx=GetCurrentLineIndex(cursorState.mCursorPosition);
		int jumpSize=0;
		int currentLine=cursorState.mCursorPosition.mLine;

		if(idx < mLines[currentLine].size()){

			if(mLines[currentLine][idx]=='\t') jumpSize=mTabWidth;
			else jumpSize=1;

			mLines[currentLine].erase(idx,1);
		}else{
			//Merge next line with current
			std::string substr=mLines[currentLine+1].substr(0);
			mLines.erase(mLines.begin()+currentLine+1);
			mLines[currentLine]+=substr;
		}

		for(size_t j=i+1;j<mCursors.size();j++){
			if(currentLine==mCursors[j].mCursorPosition.mLine){
				mCursors[j].mCursorPosition.mColumn-=jumpSize;
			}
		}
	}

	//Removing cursors with same coordinates
	mCursors.erase(std::unique(mCursors.begin(), mCursors.end(),[&](const auto& left,const auto& right){
		return left.mCursorPosition==right.mCursorPosition;
	}), mCursors.end());

	mState=mCursors[0];
	if(mCursors.size()==1) mCursors.clear();
}

// Function to insert a character at a specific position in a line
void Editor::InsertCharacter(char newChar)
{

	int idx = GetCurrentLineIndex(mState.mCursorPosition);
	GL_INFO("INSERT IDX:{}",idx);
	int currentLineIndex=mState.mCursorPosition.mLine;

	if(mState.mSelectionStart > mState.mSelectionEnd)
		std::swap(mState.mSelectionStart,mState.mSelectionEnd);

	if ( mCursors.empty() && currentLineIndex >= 0 && currentLineIndex < mLines.size() && mState.mCursorPosition.mColumn >= 0 &&
	    idx <= mLines[currentLineIndex].size()) {

		//Fix for tab spaces
		if (newChar == '\"' || newChar == '\'' || newChar == '(' || newChar == '{' || newChar == '[') {

				if(mSelectionMode==SelectionMode::Word){
					mLines[mState.mSelectionStart.mLine].insert(GetCurrentLineIndex(mState.mSelectionStart),1,newChar);
					mState.mSelectionStart.mColumn++;
				}else mLines[currentLineIndex].insert(idx, 1, newChar);
				switch (newChar) {
					case '(': newChar = ')'; break;
					case '[': newChar = ']'; break;
					case '{': newChar = '}'; break;
				}
				if(mSelectionMode==SelectionMode::Word){
					GL_INFO("eidx:{}",GetCurrentLineIndex(mState.mSelectionEnd));
					size_t end_idx=GetCurrentLineIndex(mState.mSelectionEnd)+1;
					size_t max=mLines[mState.mSelectionEnd.mLine].size();
					if(end_idx>=max) end_idx=max;

					//For MultiLine Selection Fix this
					if(mState.mSelectionStart.mLine!=mState.mSelectionEnd.mLine){
						// end_idx--;
						mState.mSelectionEnd.mColumn--;
						mState.mCursorPosition.mColumn--;
					}

					mLines[mState.mSelectionEnd.mLine].insert(end_idx,1,newChar);
					mState.mSelectionEnd.mColumn++;
				}else mLines[currentLineIndex].insert(idx + 1, 1, newChar);

		} else {

			if ((newChar == ')' || newChar == ']' || newChar == '}') && mLines[currentLineIndex][idx] == newChar) {
				// Avoiding ')' reentry after '(' pressed aka "()"
			} else {
				mLines[currentLineIndex].insert(idx, 1, newChar);
			}

		}

		mState.mCursorPosition.mColumn++;
	}else if(mCursors.size()>0){


		// if(mSelectionMode == SelectionMode::Word) Backspace();
		// if(mSelectionMode!=SelectionMode::Normal) 
		// 	mSelectionMode=SelectionMode::Normal;


		int cidx=0;
		GL_WARN("INSERT MULTIPLE CURSORS");


		for(auto& cursor:mCursors){

			if(cursor.mSelectionStart > cursor.mSelectionEnd)
				std::swap(cursor.mSelectionStart,cursor.mSelectionEnd);

			int idx = GetCurrentLineIndex(cursor.mCursorPosition);
			if (cursor.mCursorPosition.mLine >= 0 && cursor.mCursorPosition.mLine < mLines.size() && cursor.mCursorPosition.mColumn >= 0 &&
			    idx <= mLines[cursor.mCursorPosition.mLine].size()) {

				int idx = GetCurrentLineIndex(cursor.mCursorPosition);
				int count=0;
				int currentLine=cursor.mCursorPosition.mLine;
				GL_INFO("CIDX:{} IDX:{}",cidx,idx);
				if (newChar == '\"' || newChar == '\'' || newChar == '(' || newChar == '{' || newChar == '[') {

					char backup=newChar;


					if(mSelectionMode==SelectionMode::Word){
						mLines[cursor.mSelectionStart.mLine].insert(GetCurrentLineIndex(cursor.mSelectionStart),1,newChar);
						cursor.mSelectionStart.mColumn++;
					}else mLines[currentLine].insert(idx, 1, newChar);
					switch (newChar) {
						case '(': newChar = ')'; break;
						case '[': newChar = ']'; break;
						case '{': newChar = '}'; break;
					}
					if(mSelectionMode==SelectionMode::Word){
						mLines[cursor.mSelectionEnd.mLine].insert(GetCurrentLineIndex(cursor.mSelectionEnd)+1,1,newChar);
						cursor.mSelectionEnd.mColumn++;
					}else mLines[currentLine].insert(idx + 1, 1, newChar);

						count=2;
						newChar=backup;
				} else {

					if ((newChar == ')' || newChar == ']' || newChar == '}') && mLines[cursor.mCursorPosition.mLine][idx] == newChar) {
						// Avoiding ')' reentry after '(' pressed aka "()"
						count=0;
					} else {
						mLines[cursor.mCursorPosition.mLine].insert(idx, 1, newChar);
						count=1;
					}

				}
				
				//Incrementing the mColumn for next cursor on the same line
				for(int i=cidx+1;count > 0 && i<mCursors.size();i++){
					if(mCursors[i].mCursorPosition.mLine==cursor.mCursorPosition.mLine){
						mCursors[i].mCursorPosition.mColumn+=count;
						if(mSelectionMode==SelectionMode::Word){
							mCursors[i].mSelectionStart.mColumn+=count;
							mCursors[i].mSelectionEnd.mColumn+=count;
						}
					}
				}

				cursor.mCursorPosition.mColumn++;
			}
			cidx++;
		}

		mState.mCursorPosition=mCursors[0].mCursorPosition;
	}
}


void Editor::InsertLine()
{
	if(mSearchState.isValid()) mSearchState.reset();

	if(mSelectionMode!=SelectionMode::Normal)
		Backspace();

	if(mCursors.empty()) mCursors.push_back(mState);

	for(int i=0;i<mCursors.size();i++){
		int idx = GetCurrentLineIndex(mCursors[i].mCursorPosition);
		reCalculateBounds = true;

		int lineIndex=mCursors[i].mCursorPosition.mLine;

		if (idx != mLines[lineIndex].size()) {
			GL_INFO("CR BETWEEN");

			std::string substr = mLines[lineIndex].substr(idx);
			mLines[lineIndex].erase(idx);

			lineIndex++;
			mLines.insert(mLines.begin() + lineIndex, substr);

		} else {
			lineIndex++;
			mLines.insert(mLines.begin() + lineIndex, std::string(""));
		}


		int increments=1;
		// Cursors - On same line
		if(mCursors.size()>1){
			for(int j=i+1;j < mCursors.size() && mCursors[j].mCursorPosition.mLine==mCursors[i].mCursorPosition.mLine;j++){
				GL_INFO("CR SAMELINE");
				mCursors[j].mCursorPosition.mLine++;
				mCursors[j].mCursorPosition.mColumn-=mCursors[i].mCursorPosition.mColumn;
				increments++;
			}
		}

		mCursors[i].mCursorPosition.mLine++;

		//Cursors - Following lines
		if(mCursors.size()>1){
			for(int j=i+increments;j < mCursors.size() && mCursors[j].mCursorPosition.mLine >= mCursors[i].mCursorPosition.mLine;j++){
				GL_INFO("CR NEXT LINES");
				mCursors[j].mCursorPosition.mLine++;
			}
		}


		mCursors[i].mCursorPosition.mLine--;
		int prev_line=lineIndex-1;
		uint8_t tabCounts=GetTabCountsUptoCursor(mCursors[i].mCursorPosition);

		if(mLines[prev_line].size()>0){
			std::string begin="";
			while(begin.size()<tabCounts) begin+='\t';

			bool isOpenParen=false;
			if(mLines[prev_line].back()=='{') {
				isOpenParen=true;
				begin+='\t';
				tabCounts++;
			}
			if(isOpenParen && mLines[lineIndex].size() > 0 && mLines[lineIndex].back()=='}'){
				mLines.insert(mLines.begin()+lineIndex,begin);
				mLines[lineIndex+1].insert(0,begin.substr(0,begin.size()-1));
			}else{
				mLines[lineIndex].insert(0,begin.c_str());
			}
		}
		mCursors[i].mCursorPosition.mLine++;
		mCursors[i].mCursorPosition.mColumn = tabCounts*mTabWidth;
	}
	mState=mCursors[0];

	int start = std::min(int(mMinLineVisible),(int)mLines.size());
	int lineCount = (mEditorWindow->Size.y) / mLineHeight;

	//Scroll if cursor goes off screen while pressing enter
	if(mMinLineVisible+lineCount-2<=mState.mCursorPosition.mLine){
		ImGui::SetScrollY(ImGui::GetScrollY()+mLineHeight);
	}


	if(mCursors.size()==1)  mCursors.clear();
}


void Editor::DeleteCharacter(EditorState& cursor,int cidx){
	int lineIndex=cursor.mCursorPosition.mLine;
	if (lineIndex >= 0 && lineIndex < mLines.size() && cursor.mCursorPosition.mColumn > 0 &&
	    cursor.mCursorPosition.mColumn <= GetCurrentLineLength(lineIndex)) {

		int idx = GetCurrentLineIndex(cursor.mCursorPosition);
		GL_INFO("BACKSPACE IDX:{}", idx);


		char x = mLines[lineIndex][idx];

		if ((x == ')' || x == ']' || x == '}' || x == '\"' || x=='\'')) {
			bool shouldDelete = false;
			char y = mLines[lineIndex][idx - 1];

			if (x == ')' && y == '(') shouldDelete = true;
			else if (x == ']' && y == '[') shouldDelete = true;
			else if (x == '}' && y == '{') shouldDelete = true;
			else if (x == '\"' && y== '\"') shouldDelete=true;
			else if (x == '\'' && y== '\'') shouldDelete=true;


			if (shouldDelete) {
				mLines[lineIndex].erase(idx - 1, 2);
				cursor.mCursorPosition.mColumn--;
				for(int i=cidx+1;i<mCursors.size();i++){
					if(mCursors[i].mCursorPosition.mLine==cursor.mCursorPosition.mLine){
						mCursors[i].mCursorPosition.mColumn-=2;
						if(mSelectionMode==SelectionMode::Word){
							mCursors[i].mSelectionStart.mColumn-=2;
							mCursors[i].mSelectionEnd.mColumn-=2;
						}
					}
				}
				return;
			}

		}


		//Character Deletion
		if (mLines[lineIndex][idx - 1] == '\t') {
			GL_INFO("TAB REMOVE");
			mLines[lineIndex].erase(idx - 1, 1);
			// mCurrLineLength = GetCurrentLineLength(lineIndex);
			cursor.mCursorPosition.mColumn -= mTabWidth;

			if(mCursors.empty()) return;
			for(int i=cidx+1;i<mCursors.size();i++){
				if(mCursors[i].mCursorPosition.mLine==cursor.mCursorPosition.mLine){
					mCursors[i].mCursorPosition.mColumn-=mTabWidth;
					if(mSelectionMode==SelectionMode::Word){
						mCursors[i].mSelectionStart.mColumn-=mTabWidth;
						mCursors[i].mSelectionEnd.mColumn-=mTabWidth;
					}
				}
			}
		} else {
			GL_TRACE("CHAR REMOVE [chr:{} idx:{}]",(char)mLines[lineIndex][idx-1],idx-1);
			mLines[lineIndex].erase(idx - 1, 1);
			// mCurrLineLength = GetCurrentLineLength(lineIndex);
			cursor.mCursorPosition.mColumn--;
			if(mCursors.empty()) return;


			for(int i=cidx+1;i < mCursors.size() && mCursors[i].mCursorPosition.mLine==cursor.mCursorPosition.mLine;i++){
				mCursors[i].mCursorPosition.mColumn--;
				if(mSelectionMode==SelectionMode::Word){
					mCursors[i].mSelectionStart.mColumn--;
					mCursors[i].mSelectionEnd.mColumn--;
				}
			}
		}


	} 
	else if (lineIndex > 0 && cursor.mCursorPosition.mColumn == 0) {
		reCalculateBounds = true;


		GL_INFO("DELETING LINE");
		if (mLines[lineIndex].size() == 0) {
			GL_INFO("EMPTY LINE");
			mLines.erase(mLines.begin() + lineIndex);

			lineIndex--;
			cursor.mCursorPosition.mLine--;

			if(!mCursors.empty() && mSelectionMode==SelectionMode::Word){
				cursor.mSelectionStart.mLine--;
				cursor.mSelectionEnd.mLine--;
			}

			cursor.mCursorPosition.mColumn = GetCurrentLineLength(lineIndex);
		} else {
			GL_INFO("LINE BEGIN");
			int tempCursorX = GetCurrentLineLength(lineIndex - 1);

			mLines[lineIndex - 1] += mLines[lineIndex];
			mLines.erase(mLines.begin() + lineIndex);

			lineIndex--;
			cursor.mCursorPosition.mLine--;

			cursor.mCursorPosition.mColumn = tempCursorX;

			for(int i=cidx+1;i < mCursors.size() && cursor.mCursorPosition.mLine==(mCursors[i].mCursorPosition.mLine-1);i++){
				mCursors[i].mCursorPosition.mLine--;
				mCursors[i].mCursorPosition.mColumn+= tempCursorX;

				if(mSelectionMode==SelectionMode::Word){
					mCursors[i].mSelectionStart.mLine--;
					mCursors[i].mSelectionStart.mColumn+= tempCursorX;
					mCursors[i].mSelectionEnd.mLine--;
					mCursors[i].mSelectionEnd.mColumn+= tempCursorX;
				}

				cidx++;
			}
		}
		if(mCursors.empty()) return;
		for(int i=cidx+1;i < mCursors.size() && mCursors[i].mCursorPosition.mLine>(cursor.mCursorPosition.mLine-1);i++){
			mCursors[i].mCursorPosition.mLine--;
			if(mSelectionMode==SelectionMode::Word){
				mCursors[i].mSelectionStart.mLine--;
				mCursors[i].mSelectionEnd.mLine--;
			}
		}
	}
}


void Editor::Backspace()
{
	if(mSearchState.isValid()) mSearchState.reset();

	//Deletion of selection
	if (mSelectionMode == SelectionMode::Word || mSelectionMode==SelectionMode::Line) {

		mSelectionMode = SelectionMode::Normal;


		//Multiple Cursors
		if(mCursors.size()>0){
			GL_WARN("DELETING MULTIPLE CURSORS");
			for(size_t i=0;i<mCursors.size();i++){
				if(mCursors[i].mSelectionStart > mCursors[i].mSelectionEnd)
					std::swap(mCursors[i].mSelectionStart,mCursors[i].mSelectionEnd);

				uint8_t end=GetCurrentLineIndex(mCursors[i].mSelectionEnd);
				uint8_t start=GetCurrentLineIndex(mCursors[i].mSelectionStart);

				mCursors[i].mCursorPosition = mCursors[i].mSelectionStart;

				if(mCursors[i].mSelectionStart.mLine==mCursors[i].mSelectionEnd.mLine){
					uint8_t word_len = end-start;
					GL_INFO("LINE:{} START:{} END:{} LENGTH:{}",mCursors[i].mCursorPosition.mLine,start,end,word_len);
					mLines[mCursors[i].mCursorPosition.mLine].erase(start, word_len);

					for(size_t j=i+1;j<mCursors.size();j++){
						if(mCursors[j].mCursorPosition.mLine==mCursors[i].mCursorPosition.mLine){
							mCursors[j].mSelectionStart.mColumn-=word_len;
							mCursors[j].mSelectionEnd.mColumn-=word_len;
							mCursors[j].mCursorPosition.mColumn=mCursors[j].mSelectionStart.mColumn;
						}
					}
				}
			}
			mState.mCursorPosition=mCursors[0].mCursorPosition;
			return;
		}

		//Not executed if Multiple Cursors

		if(mState.mSelectionStart > mState.mSelectionEnd)
			std::swap(mState.mSelectionStart,mState.mSelectionEnd);

		uint8_t end=GetCurrentLineIndex(mState.mSelectionEnd);
		uint8_t start=GetCurrentLineIndex(mState.mSelectionStart);

		mState.mCursorPosition = mState.mSelectionStart;



		//Words on Single Line
		if(mState.mSelectionStart.mLine==mState.mSelectionEnd.mLine){
			uint8_t word_len = end-start;
			GL_INFO("WORD LEN:{}",word_len);
			mLines[mState.mCursorPosition.mLine].erase(start, word_len);
		}

		//Two Lines or More Lines
		else{

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


			if(mState.mSelectionStart.mLine < (int)floor(mMinLineVisible))
				ScrollToLineNumber(mState.mSelectionStart.mLine+1,false);

			reCalculateBounds=true;
			mState.mCursorPosition=mState.mSelectionEnd=mState.mSelectionStart;
		}
		return;
	}

	// Character deletion Inside line
	if(mCursors.empty()){
		DeleteCharacter(mState);
	}else{
		for(int i=0;i<mCursors.size();i++)
			DeleteCharacter(mCursors[i],i);

		//Removing cursors with same coordinates
		mCursors.erase(std::unique(mCursors.begin(), mCursors.end(),[&](const auto& left,const auto& right){
			return left.mCursorPosition==right.mCursorPosition;
		}), mCursors.end());

		//updating mState
		mState=mCursors[0];

		//clearing mCursors if size==1
		if(mCursors.size()==1) mCursors.clear();
	}
}

void Editor::SwapLines(bool up)
{
	if(!mCursors.empty()) return;
	int value = up ? -1 : 1;

	if(mSelectionMode==SelectionMode::Normal){

		if(mState.mCursorPosition.mLine==0 && up)  return;
		if(mState.mCursorPosition.mLine==mLines.size()-1 && !up) return;

		std::swap(mLines[mState.mCursorPosition.mLine], mLines[mState.mCursorPosition.mLine + value]);

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
}

