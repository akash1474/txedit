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
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <winnt.h>

void Editor::MoveUp(bool ctrl, bool shift)
{

	for (auto& aCursor : mState.mCursors) {
		DisableSearch();

		if (!shift && HasSelection(aCursor)) {
			DisableSelection();
			break;
		}

		if (aCursor.mCursorPosition.mLine == 0)
			continue;

		aCursor.mCursorPosition.mLine--;

		size_t lineMaxColumn = GetLineMaxColumn(aCursor.mCursorPosition.mLine);
		if (aCursor.mCursorPosition.mColumn > lineMaxColumn)
			aCursor.mCursorPosition.mColumn = lineMaxColumn;
	}

	EnsureCursorVisible();
}

void Editor::MoveDown(bool ctrl, bool shift)
{
	for (auto& aCursor : mState.mCursors) {
		DisableSearch();

		if (!shift && HasSelection(aCursor)) {
			DisableSelection();
			break;
		}

		if (aCursor.mCursorPosition.mLine == (int)mLines.size() - 1)
			continue;

		aCursor.mCursorPosition.mLine++;

		size_t lineLength = GetLineMaxColumn(aCursor.mCursorPosition.mLine);
		if (aCursor.mCursorPosition.mColumn > lineLength)
			aCursor.mCursorPosition.mColumn = lineLength;

		if(shift && HasSelection(aCursor)){
			aCursor.mSelectionEnd=aCursor.mCursorPosition;
		}
	}

	EnsureCursorVisible();
}

void Editor::MoveLeft(bool ctrl, bool shift)
{
	SelectionMode mode = mSelectionMode;
	for (auto& aCursor : mState.mCursors) {

		DisableSearch();


		if (!shift && HasSelection(aCursor)) {
			mode = SelectionMode::Normal;
			if (aCursor.mSelectionStart < aCursor.mSelectionEnd)
				aCursor.mCursorPosition = aCursor.mSelectionStart;

			aCursor.mSelectionStart=aCursor.mSelectionEnd=aCursor.mCursorPosition;
			continue;
		}

		if (shift && !HasSelection(aCursor)) {
			mode = SelectionMode::Normal;
			GL_INFO("No Selection");

			aCursor.mSelectionStart = aCursor.mSelectionEnd = aCursor.mCursorPosition;
			int idx=GetCharacterIndex(aCursor.mCursorPosition)-1;
			if(idx >= 0 && mLines[aCursor.mCursorPosition.mLine][idx].mChar=='\t'){
				aCursor.mSelectionEnd.mColumn-=mTabSize;
				aCursor.mCursorPosition.mColumn-=mTabSize;
			} 
			else
				aCursor.mSelectionEnd.mColumn = std::max(0, --aCursor.mCursorPosition.mColumn);

			// Selection Started From Line Begin then  mColumn will be -ve based on above operation
			if (aCursor.mCursorPosition.mColumn < 0) {
				aCursor.mCursorPosition.mLine--;

				aCursor.mCursorPosition.mColumn = GetLineMaxColumn(aCursor.mCursorPosition.mLine);
				aCursor.mSelectionEnd = aCursor.mCursorPosition;
			}

			if (aCursor.mSelectionStart > aCursor.mSelectionEnd)
				aCursor.mCursorDirectionChanged = true;

			continue;
		}

		//Handling ctrl+left
		auto& line=mLines[aCursor.mCursorPosition.mLine];
		int idx = GetCharacterIndex(aCursor.mCursorPosition);
		// Doesn't consider tab's in between
		if (ctrl && idx >0 && isalnum(line[idx - 1].mChar) && !IsUTFSequence(line[idx - 1].mChar)) {
			GL_INFO("WORD JUMP LEFT");
			uint8_t count = 0;

			while (idx > 0 && isalnum(line[idx - 1].mChar)) {
				idx--;
				count++;
			}

			aCursor.mCursorPosition.mColumn -= count;
			if (HasSelection(aCursor))
				aCursor.mSelectionEnd = aCursor.mCursorPosition;

			continue;
		}

		// Cursor at line start
		if (aCursor.mCursorPosition.mColumn == 0 && aCursor.mCursorPosition.mLine > 0) {
			//Restrict selection for multple cursors to one line
			if(mState.mCursors.size() > 1 && HasSelection(aCursor))
				continue;

			aCursor.mCursorPosition.mLine--;
			aCursor.mCursorPosition.mColumn = GetLineMaxColumn(aCursor.mCursorPosition.mLine);

		} else if (aCursor.mCursorPosition.mColumn > 0) {
			int idx = GetCharacterIndex(aCursor.mCursorPosition);
			if (idx == 0 && line[0].mChar == '\t') {
				aCursor.mCursorPosition.mColumn = 0;
				continue;
			}

			if (idx > 0 && line[idx - 1].mChar == '\t')
				aCursor.mCursorPosition.mColumn -= mTabSize;
			else
				aCursor.mCursorPosition.mColumn--;
		}

		if (shift && HasSelection(aCursor))
			aCursor.mSelectionEnd = aCursor.mCursorPosition;

		if (aCursor.mSelectionStart > aCursor.mSelectionEnd)
			aCursor.mCursorDirectionChanged = true;
	}

	MergeCursorsIfNeeded();

	mSelectionMode = mode;
	EnsureCursorVisible();
}


void Editor::MoveRight(bool ctrl, bool shift)
{

	SelectionMode mode = mSelectionMode;

	for (auto& aCursor : mState.mCursors) {

		Line& line=mLines[aCursor.mCursorPosition.mLine];

		if (mLinePosition.y > mEditorWindow->Size.y)
			ImGui::SetScrollY(ImGui::GetScrollY() + mLineHeight);

		DisableSearch();

		// Disable selection if only right key pressed without
		if (!shift && HasSelection(aCursor)) {
			mode = SelectionMode::Normal;
			if (aCursor.mSelectionStart > aCursor.mSelectionEnd)
				aCursor.mCursorPosition = aCursor.mSelectionStart;

			aCursor.mSelectionStart=aCursor.mSelectionEnd=aCursor.mCursorPosition;
			continue;
		}

		if (shift && !HasSelection(aCursor)) {
			mode = SelectionMode::Normal;
			aCursor.mSelectionStart = aCursor.mSelectionEnd = aCursor.mCursorPosition;

			int idx=GetCharacterIndex(aCursor.mCursorPosition);
			if(idx < mLines[aCursor.mCursorPosition.mLine].size() && mLines[aCursor.mCursorPosition.mLine][idx].mChar=='\t'){
				aCursor.mSelectionEnd.mColumn+=mTabSize;
				aCursor.mCursorPosition.mColumn+=mTabSize;
			}else
				aCursor.mSelectionEnd.mColumn = (++aCursor.mCursorPosition.mColumn);

			// Selection Started From Line End the Cursor mColumn > len
			if (aCursor.mCursorPosition.mColumn > GetLineMaxColumn(aCursor.mCursorPosition.mLine)) {
				aCursor.mCursorPosition.mLine++;

				aCursor.mCursorPosition.mColumn = 0;
				aCursor.mSelectionEnd = aCursor.mCursorPosition;
			}

			continue;
		}

		int idx=GetCharacterIndex(aCursor.mCursorPosition);
		//Only execute when not at LineEnd
		if (ctrl && idx<line.size() && isalnum(line[idx].mChar) && !IsUTFSequence(line[idx].mChar)) {
			GL_INFO("WORD JUMP RIGHT");
			uint8_t count = 0;

			while (isalnum(line[idx].mChar) && !IsUTFSequence(line[idx].mChar)) {
				idx++;
				count++;
			}

			aCursor.mCursorPosition.mColumn += count;
			if (HasSelection(aCursor))
				aCursor.mSelectionEnd = aCursor.mCursorPosition;

			continue;
		}


		size_t lineLength = GetLineMaxColumn(aCursor.mCursorPosition.mLine);
		if (aCursor.mCursorPosition.mColumn == lineLength) {

			//Restrict selection for multple cursors to one line
			if(mState.mCursors.size() > 1 && HasSelection(aCursor))
				continue;

			aCursor.mCursorPosition.mColumn = 0;
			aCursor.mCursorPosition.mLine++;

		} else if (aCursor.mCursorPosition.mColumn < lineLength) {

			if (aCursor.mCursorPosition.mColumn == 0 && line[0].mChar == '\t') {
				aCursor.mCursorPosition.mColumn += mTabSize;
			} else if (line[GetCharacterIndex(aCursor.mCursorPosition)].mChar == '\t') {
				aCursor.mCursorPosition.mColumn += mTabSize;
			} else {
				aCursor.mCursorPosition.mColumn++;
			}
		}

		if (shift && HasSelection(aCursor)) {
			aCursor.mSelectionEnd = aCursor.mCursorPosition;
		}
	}
	mSelectionMode = mode;

	MergeCursorsIfNeeded();
	EnsureCursorVisible();
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

	std::string buffer=GetNearbyLinesString(GetCurrentCursor().mCursorPosition.mLine,1);

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
	ts_tree_delete(tree);
	ts_parser_delete(parser);
}


uint32_t Editor::GetLineLengthInBytes(int aLineIdx){
	return mLines[aLineIdx].size();
}

bool IsOpeningBracket(char aChar){
	return aChar == '(' || aChar == '{' || aChar == '[';
}

char GetClosingBracketFor(char x)
{
	switch (x) {
		case '(': x = ')'; break;
		case '[': x = ']'; break;
		case '{': x = '}'; break;
	}
	return x;
}

void Editor::InsertCharacter(char chr){
	DisableSearch();

	uint8_t aChar=chr;
	UndoRecord u;
	u.mBefore = mState;

	char buff[7];
	int e = ImTextCharToUtf8(buff,7, aChar);
	if (e <= 0) return;

	buff[e] = '\0';
	bool addedQuotesOrBrackets=false;

	for(auto& aCursor:mState.mCursors)
	{
		Coordinates& coord=aCursor.mCursorPosition;
		auto& line = mLines[coord.mLine];
		auto cindex = GetCharacterIndex(coord);

		UndoOperation added;
		added.mType = UndoOperationType::Add;
		added.mStart = coord;

		for (auto p = buff; *p != '\0'; p++, ++cindex)
			line.insert(line.begin() + cindex, Glyph(*p, ColorSchemeIdx::Default));

		added.mText = buff;

		if (IsOpeningBracket(chr)) {
			char closingBracket=GetClosingBracketFor(chr);
            line.insert(line.begin() + cindex, Glyph(closingBracket, ColorSchemeIdx::Default));
            added.mText+=closingBracket;
            addedQuotesOrBrackets=true;
        }

        // if at (line end ) or (next character is not same) -> insert one more chr
        if((chr=='\'' || chr == '\"') && (cindex==line.size() || (cindex < line.size() && line[cindex].mChar!=chr))){
            line.insert(line.begin() + cindex, Glyph(chr, ColorSchemeIdx::Default));
            added.mText+=chr;
            addedQuotesOrBrackets=true;
        }

		coord.mColumn++;
		aCursor.mSelectionStart=aCursor.mSelectionEnd=coord;

		bool isNextCursor=false;
		for(auto& aNextCursor:mState.mCursors){
			if(aNextCursor.mCursorPosition==aCursor.mCursorPosition)
				isNextCursor=true;
			else if(isNextCursor && aNextCursor.mCursorPosition.mLine==aCursor.mCursorPosition.mLine){
				aNextCursor.mCursorPosition.mColumn++;
			}
		}

		added.mEnd = addedQuotesOrBrackets ? Coordinates(coord.mLine,coord.mColumn+1): coord;

		u.mOperations.push_back(added);

	}

	//TODO: Optimize Highlighting when dealing with multiple cursors
	Cursor& aCursor=GetCurrentCursor();

	bool hasMultipleCursors=mState.mCursors.size()>1;
	for(auto& aCursor:mState.mCursors){
		if(hasMultipleCursors)
			UpdateSyntaxHighlighting(aCursor.mCursorPosition.mLine,2);
		else{
			UpdateSyntaxHighlighting(aCursor.mCursorPosition.mLine,10);
			FindBracketMatch(aCursor.mCursorPosition);
		}
	}

	u.mAfter = mState;
	mUndoManager.AddUndo(u);


	EnsureCursorVisible();
	DebouncedReparse();
	SetIsBufferModified(true);
}

void Editor::MoveTop(bool aShift)
{
	DisableSearch();
	Cursor& aState=mState.mCursors[mState.mCurrentCursorIdx];
	mState.mCursors.clear();

	auto oldPos = aState.mCursorPosition;
	aState.mCursorPosition=Coordinates(0, 0);

	if (aState.mCursorPosition != oldPos)
	{
		if (aShift)
		{
			aState.mSelectionStart=oldPos;
			aState.mSelectionEnd=aState.mCursorPosition;
			aState.mCursorDirectionChanged=true;
		}
		else
			aState.mSelectionStart = aState.mSelectionEnd = aState.mCursorPosition;
	}

	mState.mCursors.push_back(aState);
	mState.mCurrentCursorIdx=0;

	EnsureCursorVisible();
}

void Editor::MoveBottom(bool aShift)
{
	DisableSearch();
	Cursor& aState=mState.mCursors[mState.mCurrentCursorIdx];
	mState.mCursors.clear();

	auto oldPos = aState.mCursorPosition;
	aState.mCursorPosition=Coordinates((int)mLines.size() - 1, 0);

	if (aState.mCursorPosition != oldPos)
	{
		if (aShift)
		{
			aState.mSelectionEnd=aState.mCursorPosition;
			aState.mSelectionStart=oldPos;
		}
		else
			aState.mSelectionStart = aState.mSelectionEnd = aState.mCursorPosition;
	}

	mState.mCursors.push_back(aState);
	mState.mCurrentCursorIdx=0;

	EnsureCursorVisible();
}

void Editor::MoveHome(bool aShift)
{
	DisableSearch();
	for(auto& aCursor:mState.mCursors)
	{

		auto oldPos = aCursor.mCursorPosition;
		int tabCounts=GetTabCountsAtLineStart(oldPos);

		aCursor.mCursorPosition=Coordinates(aCursor.mCursorPosition.mLine, tabCounts*mTabSize);

		if (aCursor.mCursorPosition != oldPos)
		{
			if (aShift)
			{
				aCursor.mSelectionStart=aCursor.mCursorPosition;
				aCursor.mSelectionEnd=oldPos;
			}
			else
				aCursor.mSelectionStart = aCursor.mSelectionEnd = aCursor.mCursorPosition;
		}

	}
	EnsureCursorVisible();
}

void Editor::MoveEnd(bool aShift)
{
	DisableSearch();
	for(auto& aCursor:mState.mCursors){

		auto oldPos = aCursor.mCursorPosition;
		aCursor.mCursorPosition=Coordinates(aCursor.mCursorPosition.mLine, GetLineMaxColumn(oldPos.mLine));

		if (aCursor.mCursorPosition != oldPos)
		{
			if (aShift)
			{
				aCursor.mSelectionEnd=aCursor.mCursorPosition;
				aCursor.mSelectionStart=oldPos;
			}
			else
				aCursor.mSelectionStart = aCursor.mSelectionEnd = aCursor.mCursorPosition;
		}

	}
	EnsureCursorVisible();
}

void Editor::InsertLineBreak(){
	GL_INFO("InsertLineBreak");
	// if(HasSelection()) Backspace(); //Just a temporary comment out

	DisableSearch();

	auto& cursors=mState.mCursors;
	UndoRecord uRecord;
	uRecord.mBefore=mState;

	UndoOperation uAdded;
	uAdded.mType=UndoOperationType::Add;

	for(int i=cursors.size()-1;i>-1;i--){
		GL_INFO("Loopeing");
		Cursor& cursor=cursors[i];
		Coordinates& coord=cursor.mCursorPosition;
		uAdded.mStart=coord;

		InsertLine(coord.mLine + 1);
		auto& line = mLines[coord.mLine];
		auto& newLine = mLines[coord.mLine + 1];

		auto cindex = GetCharacterIndex(coord);
		uAdded.mText='\n';


		//Add tabs to maintain indentation
		for (int i = 0; i < line.size() && isascii(line[i].mChar) && line[i].mChar=='\t'; ++i)
		{
			newLine.push_back(line[i]);
			uAdded.mText+='\t';
		}

		const size_t idx=newLine.size();
		newLine.insert(newLine.end(), line.begin() + cindex, line.end());
		line.erase(line.begin() + cindex, line.begin() + line.size());

		cursor.mCursorPosition=Coordinates(coord.mLine + 1, GetCharacterColumn(coord.mLine + 1, idx));

		for(int j=i+1;j<cursors.size();j++)
			cursors[j].mCursorPosition.mLine++;

		uAdded.mEnd=cursor.mCursorPosition;
		uRecord.mOperations.push_back(uAdded);
	}

	uRecord.mAfter=mState;
	mUndoManager.AddUndo(uRecord);

	MergeCursorsIfNeeded();
	FindBracketMatch(GetCurrentCursor().mCursorPosition);
	EnsureCursorVisible();
}


// // TODO: Implement the character deletion UndoRecord Storage
void Editor::DeleteCharacter(Cursor& aCursor, bool aDeletePreviousCharacter,UndoRecord* uRecord)
{

	bool updateLineNumber=false;
	bool isTabDeleted=false;
	int prevLineSize;


	UndoOperation uRemoved;
	uRemoved.mType=UndoOperationType::Delete;

	if(aDeletePreviousCharacter)
		uRemoved.mEnd=aCursor.mCursorPosition;
	else
		uRemoved.mStart=aCursor.mCursorPosition;

	if(aDeletePreviousCharacter)
	{
		if (aCursor.mCursorPosition.mColumn == 0)
		{
			if (aCursor.mCursorPosition.mLine == 0)
				return;


			auto& line = mLines[aCursor.mCursorPosition.mLine];
			auto& prevLine = mLines[aCursor.mCursorPosition.mLine - 1];
			prevLineSize = GetLineMaxColumn(aCursor.mCursorPosition.mLine - 1);
			prevLine.insert(prevLine.end(), line.begin(), line.end());

			RemoveLine(aCursor.mCursorPosition.mLine);
			--aCursor.mCursorPosition.mLine;
			aCursor.mCursorPosition.mColumn = prevLineSize;

			uRemoved.mText='\n';
			updateLineNumber=true;

		}
		else
		{
			auto& line = mLines[aCursor.mCursorPosition.mLine];
			auto cindex = GetCharacterIndex(aCursor.mCursorPosition) - 1;
			if(line[cindex].mChar=='\t')
			{
				auto start=line.begin()+cindex;
				line.erase(start,start+1);
				aCursor.mCursorPosition.mColumn -= mTabSize;

				uRemoved.mText='\t';

				FindBracketMatch(aCursor.mCursorPosition);
				isTabDeleted=true;
			}
			else
			{
				auto cend = cindex + 1;
				while (cindex > 0 && IsUTFSequence(line[cindex].mChar))
					--cindex;

				--aCursor.mCursorPosition.mColumn;

				std::string rText;
				while (cindex < line.size() && cend-- > cindex){
					rText+=(char)line[cindex].mChar;
					line.erase(line.begin() + cindex);
				}


				uRemoved.mText=rText;
			}
		}

		mTextChanged = true;


	}
	else //Delete Key
	{
		Coordinates& aCoords=aCursor.mCursorPosition;

		auto& line = mLines[aCoords.mLine];

		if (aCoords.mColumn == GetLineMaxColumn(aCoords.mLine))
		{
			if (aCoords.mLine == (int)mLines.size() - 1)
				return;

			//Used for updating the next cursor positions
			prevLineSize=aCoords.mColumn;

			auto& nextLine = mLines[aCoords.mLine + 1];
			// nextLine.erase()
			line.insert(line.end(), nextLine.begin(), nextLine.end());
			RemoveLine(aCoords.mLine + 1);
			updateLineNumber=true;

			uRemoved.mText='\n';
		}
		else
		{
			auto cindex = GetCharacterIndex(aCoords);

			auto d = UTF8CharLength(line[cindex].mChar);
			std::string rText;
			while (d-- > 0 && cindex < (int)line.size()){
				rText+=(char)line[cindex].mChar;
				line.erase(line.begin() + cindex);
			}

			uRemoved.mText=rText;
		}

		mTextChanged = true;

	}

	if(aDeletePreviousCharacter)
		uRemoved.mStart=aCursor.mCursorPosition;
	else
		uRemoved.mEnd=aCursor.mCursorPosition;
	//FIX: handling one at line start and other on same line

	//Updating the mLine for all the cursors after current cursor
	bool isNextCursor=false;
	for(auto& aNextCursor:mState.mCursors){
		if(aNextCursor.mCursorPosition==aCursor.mCursorPosition)
			isNextCursor=true;
		else if(isNextCursor)
		{
			GL_INFO("Executing Outer");
			//aCursor was at line start before backspace
			if(updateLineNumber)
			{
				// and aCursor & aNextCursor were on same line
				if(aNextCursor.mCursorPosition.mLine==aCursor.mCursorPosition.mLine+1) 
				{
					//+1 as aCursor's mLine was updated above
					aNextCursor.mCursorPosition.mLine--;
					aNextCursor.mCursorPosition.mColumn+=prevLineSize;
				}
				else // but different line
				{
					aNextCursor.mCursorPosition.mLine--;
				}
			}
			else if(aNextCursor.mCursorPosition.mLine==aCursor.mCursorPosition.mLine) //aCursor was inBetween line
			{
					aNextCursor.mCursorPosition.mColumn--;
			}
		}
	}


	if(uRecord)
		uRecord->mOperations.push_back(uRemoved);

}


void Editor::Backspace()
{
	OpenGL::ScopedTimer timer("Editor::Backspace");
	DisableSearch();

	UndoRecord uRecord;
	uRecord.mBefore=mState;

	Cursor aCurrentCursor=GetCurrentCursor();
	// Deletion of selection
	for(size_t i=0;i<mState.mCursors.size();i++)
	{
		auto& aCursor=mState.mCursors[i];

		if (HasSelection(aCursor))
		{
			if(aCursor.mSelectionStart>aCursor.mSelectionEnd) 
				std::swap(aCursor.mSelectionStart,aCursor.mSelectionEnd);

			int selectionLen=aCursor.mSelectionEnd.mColumn - aCursor.mSelectionStart.mColumn;
			uRecord.mOperations.push_back({ GetSelectedText(aCursor), aCursor.mSelectionStart, aCursor.mSelectionEnd, UndoOperationType::Delete });
			DeleteSelection(aCursor);

			// Handling Cursors on the same line
			for(int j=i+1;j<mState.mCursors.size();j++){
				auto& nextCursor=mState.mCursors[j];

				//when selections on sameline
				if(nextCursor.mCursorPosition.mLine==aCursor.mCursorPosition.mLine){
					nextCursor.mSelectionStart.mColumn=std::max(0,nextCursor.mSelectionStart.mColumn-selectionLen);
					nextCursor.mSelectionEnd.mColumn=std::max(0,nextCursor.mSelectionEnd.mColumn-selectionLen);
				}
			}
			uRecord.mAfter=mState;
		}
		else
		{
			DeleteCharacter(aCursor,true,&uRecord);
			uRecord.mAfter=mState;
		}
	}

	mUndoManager.AddUndo(uRecord);

	int i=0;
	for(auto& aCursor:mState.mCursors)
		GL_INFO("Cursor({},{}) » {}",aCursor.mCursorPosition.mLine,aCursor.mCursorPosition.mColumn,i);


	// Removing cursors with same coordinates
	MergeCursorsIfNeeded();


	// updating mState.mCurrentCursorIdx
	for(size_t i=0;i<mState.mCursors.size();i++)
		if(mState.mCursors[i].mCursorPosition==aCurrentCursor.mCursorPosition)
			mState.mCurrentCursorIdx=i;


	Cursor& newCursor = GetCurrentCursor();
	EnsureCursorVisible();
	UpdateSyntaxHighlighting(newCursor.mCursorPosition.mLine,0);
	FindBracketMatch(newCursor.mCursorPosition);
	DebouncedReparse();
}



void Editor::Delete()
{
	DisableSearch();
	
	assert(!mReadOnly);

	if (mLines.empty())
		return;

	UndoRecord uRecord;
	uRecord.mBefore = mState;

	for(size_t i=0;i<mState.mCursors.size();i++)
	{
		auto& aCursor=mState.mCursors[i];

		if (HasSelection(aCursor))
		{
			if(aCursor.mSelectionStart>aCursor.mSelectionEnd) 
				std::swap(aCursor.mSelectionStart,aCursor.mSelectionEnd);

			int selectionLen=aCursor.mSelectionEnd.mColumn - aCursor.mSelectionStart.mColumn;
			uRecord.mOperations.push_back({ GetSelectedText(aCursor), aCursor.mSelectionStart, aCursor.mSelectionEnd, UndoOperationType::Delete });
			DeleteSelection(aCursor);

			// Handling Cursors on the same line
			for(int j=i+1;j<mState.mCursors.size();j++){
				auto& nextCursor=mState.mCursors[j];

				//when selections on sameline
				if(nextCursor.mCursorPosition.mLine==aCursor.mCursorPosition.mLine){
					nextCursor.mSelectionStart.mColumn=std::max(0,nextCursor.mSelectionStart.mColumn-selectionLen);
					nextCursor.mSelectionEnd.mColumn=std::max(0,nextCursor.mSelectionEnd.mColumn-selectionLen);
				}
			}

			uRecord.mAfter=mState;
		}
		else
		{
			DeleteCharacter(aCursor,false,&uRecord);
			uRecord.mAfter=mState;
		}
	}

	mUndoManager.AddUndo(uRecord);
	MergeCursorsIfNeeded();
	EnsureCursorVisible();
}


void Editor::SwapLines(bool up)
{
	if (mState.mCursors.size()>1)
		return;

	int value = up ? -1 : 1;
	Cursor& aCursor=GetCurrentCursor();

	if (!HasSelection(aCursor)) {

		if (aCursor.mCursorPosition.mLine == 0 && up)
			return;
		if (aCursor.mCursorPosition.mLine == mLines.size() - 1 && !up)
			return;

		std::swap(mLines[aCursor.mCursorPosition.mLine], mLines[aCursor.mCursorPosition.mLine + value]);

	} else {

		if (aCursor.mSelectionStart > aCursor.mSelectionEnd)
			std::swap(aCursor.mSelectionStart, aCursor.mSelectionEnd);

		int startLine = aCursor.mSelectionStart.mLine;
		int endLine = aCursor.mSelectionEnd.mLine;


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

	aCursor.mSelectionStart.mLine += value;
	aCursor.mSelectionEnd.mLine += value;
	aCursor.mCursorPosition.mLine += value;
	EnsureCursorVisible();
}


void Editor::InsertTab(bool isShiftPressed)
{
	DisableSearch();

	UndoRecord uRecord;
	uRecord.mBefore = mState;

	if (!HasSelection(GetCurrentCursor())) {
		for (int i = 0; i < mState.mCursors.size(); i++) {
			Cursor& aCursor=mState.mCursors[i];
			auto& line=mLines[aCursor.mCursorPosition.mLine];

			UndoOperation uAdded;
			uAdded.mStart=aCursor.mCursorPosition;
			uAdded.mType=UndoOperationType::Add;
			uAdded.mText='\t';


			int idx = GetCharacterIndex(aCursor.mCursorPosition);

			GL_INFO("IDX:", idx);

			// if(idx>0 && line[idx-1].mChar!='\t') continue;
			line.insert(line.begin() + idx, 1, Glyph('\t',ColorSchemeIdx::Default));

			aCursor.mCursorPosition.mColumn += mTabSize;

			//updating mColumn for cursor on sameline
			for (int j = i + 1; j < mState.mCursors.size(); j++) {
				if (mState.mCursors[j].mCursorPosition.mLine == mState.mCursors[i].mCursorPosition.mLine)
					mState.mCursors[j].mCursorPosition.mColumn += mTabSize;
			}

			uAdded.mEnd=aCursor.mCursorPosition;
			uRecord.mOperations.push_back(uAdded);
		}

	}
	else
	{
		// Indenting is not supported by multiple cursors
		if(mState.mCursors.size()>1) return;
		GL_INFO("INDENTING");

		Cursor& aCursor=GetCurrentCursor();
		int startLine = aCursor.mSelectionStart.mLine;
		int endLine = aCursor.mSelectionEnd.mLine;

		if (startLine > endLine)
			std::swap(startLine, endLine);
		int indentValue = isShiftPressed ? -1 : 1;


		bool isFirst = true;

		while (startLine <= endLine) {

			if (isShiftPressed) {
				UndoOperation uRemoved;
				uRemoved.mType=UndoOperationType::Delete;
				if (mLines[startLine][0].mChar == '\t'){
					mLines[startLine].erase(mLines[startLine].begin());
					uRemoved.mStart=Coordinates(startLine, 0);
					uRemoved.mEnd=Coordinates(startLine, mTabSize);
					uRemoved.mText='\t';
					uRecord.mOperations.push_back(uRemoved);
				}
				else
					indentValue=0;

			} else {
				mLines[startLine].insert(mLines[startLine].begin(), 1, Glyph('\t',ColorSchemeIdx::Default));

				UndoOperation uAdded;
				uAdded.mType=UndoOperationType::Add;
				uAdded.mStart=Coordinates(startLine, 0);
				uAdded.mEnd=Coordinates(startLine, mTabSize);
				uAdded.mText='\t';
				uRecord.mOperations.push_back(uAdded);
			}

			startLine++;
		}
		aCursor.mSelectionStart.mColumn += mTabSize * indentValue;
		aCursor.mSelectionEnd.mColumn += mTabSize * indentValue;
		aCursor.mCursorPosition.mColumn += mTabSize * indentValue;
	}


	uRecord.mAfter=mState;
	mUndoManager.AddUndo(uRecord);
}

void Editor::DeleteRange(const Coordinates& aStart, const Coordinates& aEnd)
{
	assert(aEnd >= aStart);
	assert(!mReadOnly);
	DisableSearch();

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
