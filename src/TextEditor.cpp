#include "pch.h"
#include "DataTypes.h"
#include "Coordinates.h"
#include "Timer.h"
#include "UndoManager.h"
#include "imgui_internal.h"
#include "TextEditor.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <ctype.h>
#include <filesystem>
#include <ios>
#include <stdint.h>

#include <cctype>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <utility>

#include "Log.h"
#include "imgui.h"
#include "StatusBarManager.h"
#include "TabsManager.h"

#include "tree_sitter/api.h"

template <class InputIt1, class InputIt2, class BinaryPredicate>
bool equals(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, BinaryPredicate p)
{
	for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
		if (!p(*first1, *first2))
			return false;
	}

	return first1 == last1 && first2 == last2;
}

void Editor::SetPalette(const Palette & aValue)
{
	mPaletteBase = aValue;
}

void Editor::SetLanguageDefinition(const LanguageDefinition & aLanguageDef)
{
	mLanguageDefinition = aLanguageDef;
}

const Editor::LanguageDefinition& Editor::LanguageDefinition::CPlusPlus()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	return langDef;
}


Editor::Editor()
{
	InitPallet();
	InitFileExtensions();
	SetPalette(GetGruvboxPalette());
	SetLanguageDefinition(LanguageDefinition::CPlusPlus());
	mLines.push_back(Line());
	workerThread_ = std::thread(&Editor::WorkerThread, this);
	mState.mCursors.emplace_back(Cursor());
	for (int i = 0; i < (int)PaletteIndex::Max; ++i) {
		auto color = ImGui::ColorConvertU32ToFloat4(mPaletteBase[i]);
		color.w *= 1.0f;
		mPalette[i] = ImGui::ColorConvertFloat4ToU32(color);
	}
}
Editor::~Editor() {
	CloseDebounceThread();
	ts_query_delete(mQuery);
}

void Editor::ResetState(){
	DisableSearch();
	mState.mCursors.clear();

	Cursor aCursor;
	aCursor.mCursorPosition=aCursor.mSelectionStart=aCursor.mSelectionEnd=Coordinates(0,0);
	mState.mCursors.push_back(aCursor);
	
	mState.mCurrentCursorIdx=0;
	mSelectionMode=SelectionMode::Normal;
	mBracketMatch.mHasMatch=false;
}


void Editor::LoadFile(const char* filepath){

	//When creating a new file filepath is passed empty()
	if(strlen(filepath)==0){
		this->SetBuffer("");
		fileType="Unknown";
		isFileLoaded=true;
		return;
	}



	if(!std::filesystem::exists(filepath)){
		StatusBarManager::ShowNotification("Invalid Path",filepath,StatusBarManager::NotificationType::Error);
		return;
	}
	GL_INFO("Editor::LoadFile - {}",filepath);
	this->ResetState();

	std::ifstream file(filepath);
	if(!file.good())
		GL_CRITICAL("Failed to ReadFile:{}",filepath);

	mFilePath=filepath;
	fileType=GetFileType();

	file.seekg(0, std::ios::end);
	size_t fileSize = file.tellg();
	file.seekg(0,std::ios::beg);
	
	std::string content(fileSize, '\0');
	file.read(&content[0], fileSize);

	//Trimming the spaces located at the last line at the end
	content.erase(std::find_if(content.rbegin(), content.rend(), [](char ch) {
        return std::isspace(ch);
    }).base(), content.end());

	this->SetBuffer(content);
	isFileLoaded=true;
}

void Editor::SetBuffer(const std::string& text)
{
	GL_INFO("Editor::SetBuffer");
	mLines.clear();
	mLines.emplace_back(Line());

	for (auto chr : text)
	{
		if (chr == '\r')
			continue;

		if (chr == '\n'){
			mLines.emplace_back(Line());
		}
		else
		{
			mLines.back().emplace_back(chr, ColorSchemeIdx::Default);
		}
	}

	if (mLines.back().size() > 400)
		mLines.back().clear();

	mTextChanged = true;
	mScrollToTop = true;

	GL_INFO("FILE INFO --> Lines:{}", mLines.size());
	// mUndoManager.Clear();
	// GL_INFO("Extension:{}",);
	std::string ext=std::filesystem::path(mFilePath).extension().generic_string();
	if(ext==".cpp" || ext==".h" || ext==".hpp" || ext==".c")
	{
		mIsSyntaxHighlightingSupportForFile=true;
		ApplySyntaxHighlighting(text);
	}
	else 
		mIsSyntaxHighlightingSupportForFile=false;
}


void Editor::ClearEditor(){
	mFilePath.clear();
	isFileLoaded=false;
	fileType.clear();
	mLines.clear();
	ResetState();
}

void Editor::InitFileExtensions(){
    FileExtensions[".c"] = "C";
    FileExtensions[".cpp"] = "C++";
    FileExtensions[".h"] = "C/C++ Header";
    FileExtensions[".hpp"] = "C++ Header";
    FileExtensions[".java"] = "Java";
    FileExtensions[".py"] = "Python";
    FileExtensions[".html"] = "HTML";
    FileExtensions[".css"] = "CSS";
    FileExtensions[".js"] = "JavaScript";
    FileExtensions[".php"] = "PHP";
    FileExtensions[".rb"] = "Ruby";
    FileExtensions[".pl"] = "Perl";
    FileExtensions[".swift"] = "Swift";
    FileExtensions[".ts"] = "TypeScript";
    FileExtensions[".csharp"] = "C#";
    FileExtensions[".go"] = "Go";
    FileExtensions[".rust"] = "Rust";
    FileExtensions[".kotlin"] = "Kotlin";
    FileExtensions[".scala"] = "Scala";
    FileExtensions[".sql"] = "SQL";
    FileExtensions[".json"] = "JSON";
    FileExtensions[".xml"] = "XML";
    FileExtensions[".yaml"] = "YAML";
    FileExtensions[".makefile"] = "Makefile";
    FileExtensions[".bat"] = "Batch";
    FileExtensions[".sh"] = "Shell Script";
    FileExtensions[".md"] = "Markdown";
    FileExtensions[".tex"] = "LaTeX";
    FileExtensions[".csv"] = "CSV";
    FileExtensions[".tsv"] = "TSV";
    FileExtensions[".svg"] = "SVG";
    FileExtensions[".gitignore"] = "Git Ignore";
    FileExtensions[".dockerfile"] = "Dockerfile";
    FileExtensions[".lua"] = "Lua";
    FileExtensions[".sln"] = "Visual Studio Solution";
    FileExtensions[".gitmodules"] = "Git Modules";
    
    FileExtensions[".asm"] = "Assembly";
    FileExtensions[".bat"] = "Batch Script";
    FileExtensions[".conf"] = "Configuration";
    FileExtensions[".dll"] = "Dynamic Link Library";
    FileExtensions[".exe"] = "Executable";
    FileExtensions[".obj"] = "Object";
    FileExtensions[".pch"] = "Precompiled Header";
    FileExtensions[".pdf"] = "PDF";
    FileExtensions[".ppt"] = "PowerPoint";
    FileExtensions[".txt"] = "Plain Text";
    FileExtensions[".xls"] = "Excel";
    FileExtensions[".zip"] = "Zip Archive";
    FileExtensions[".log"] = "Log File";
}



std::string Editor::GetFileType(){
	std::string ext=std::filesystem::path(mFilePath).extension().generic_u8string();
	if(FileExtensions.find(ext)!=FileExtensions.end())
		return FileExtensions[ext];

	if(ext.size()>0){
		ext=ext.substr(1);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
	}
	return ext;
}



bool Editor::Render(bool* aIsOpen,std::string& aUUID){
	ImGuiStyle& style=ImGui::GetStyle();
	float width=style.ScrollbarSize;
	style.ScrollbarSize=20.0f;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin(aUUID.c_str(),aIsOpen,this->mBufferModified ?  ImGuiWindowFlags_UnsavedDocument : ImGuiWindowFlags_None);
		ImGui::PopStyleVar();
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[(uint8_t)Fonts::MonoLisaRegular]);

		bool isWindowFocused=ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		if(isWindowFocused)
			ImGui::SetNextWindowFocus();
		// else if(!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
		// {
		// 	ImGui::PopFont();
		// 	ImGui::End();  // #text_editor
		// 	return false;
		// }
		// 	GL_INFO("Collapsed:{}",aUUID);
		this->Draw();


		//Executes once when we focus on window
		if(ImGui::IsWindowDocked() && isWindowFocused){
			TabsManager::SetNewTabsDockSpaceId(ImGui::GetWindowDockID());
		}

		style.ScrollbarSize=width;
		ImGui::PopFont();
	ImGui::End();  // #text_editor

	return isWindowFocused;
}

ImVec2 Editor::GetLinePosition(const Coordinates& aCoords){
	return {ImGui::GetWindowPos().x-ImGui::GetScrollX(),ImGui::GetWindowPos().y+(aCoords.mLine*mLineHeight)-ImGui::GetScrollY()};
}

bool Editor::Draw()
{

	// OpenGL::ScopedTimer timer("Editor::Draw");
	if(!isFileLoaded) return false;
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ChildBg,mGruvboxPalletDark[(size_t)Pallet::Background]);
	float sizeY= mLines.size() * (mLineSpacing + mCharacterSize.y);
	ImGui::SetNextWindowContentSize(ImVec2(ImGui::GetContentRegionMax().x + 1500.0f, sizeY+250.0f));
	ImGui::BeginChild("TextEditor", {0,0}, 0,  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::PopStyleColor();

	if(ImGui::IsWindowCollapsed())
		GL_INFO("Collapsed");
	
	mEditorWindow = ImGui::GetCurrentWindow();
	mEditorSize = ImVec2(mEditorWindow->ContentRegionRect.Max.x, mLines.size() * (mLineSpacing + mCharacterSize.y));
	mEditorPosition = mEditorWindow->Pos;

	mEditorPosition = mEditorWindow->Pos;

	mCharacterSize = ImVec2(ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr));

	mLineBarMaxCountWidth=GetNumberWidth(mLines.size());
	mLineBarWidth=ImGui::CalcTextSize(std::to_string(mLines.size()).c_str()).x + 2 * mLineBarPadding;

	mLinePosition = ImVec2({mEditorPosition.x + mLineBarWidth + mPaddingLeft, mEditorPosition.y});
	mLineHeight = mLineSpacing + mCharacterSize.y;

	mSelectionMode = SelectionMode::Normal;

	const ImGuiIO& io = ImGui::GetIO();

	// if( ImGui::IsMouseDown(0) && mSelectionMode==SelectionMode::Word && (ImGui::GetMousePos().y>(mEditorPosition.y+mEditorWindow->Size.y))){
	// 	ImGui::SetScrollY(ImGui::GetScrollY()+mLineHeight);
	// 	if(mState.mSelectionEnd.mLine < mLines.size()-1){
	// 		mState.mSelectionEnd.mLine++;
	// 		mState.mCursorPosition.mLine++;
	// 	}
	// }

	// if(ImGui::IsMouseDown(0) && mSelectionMode==SelectionMode::Word && (ImGui::GetMousePos().y<mEditorPosition.y)){
	// 	ImGui::SetScrollY(ImGui::GetScrollY()-mLineHeight);
	// 	if(mState.mSelectionEnd.mLine > 0){
	// 		mState.mSelectionEnd.mLine--;
	// 		mState.mCursorPosition.mLine--;
	// 	}
	// }




	if(mScrollAnimation.hasStarted){
		static bool isFirst=true;
		if(isFirst){
			mInitialScrollY=ImGui::GetScrollY();
			isFirst=false;
		}

		ImGui::SetScrollY(mInitialScrollY+(mScrollAnimation.update()*mScrollAmount));
		if(!mScrollAnimation.hasStarted) isFirst=true;
	}

	if (ImGui::IsWindowFocused() && io.MouseWheel != 0.0f) {
		GL_INFO("SCROLLX:{} SCROLLY:{}",ImGui::GetScrollX(),ImGui::GetScrollY());
		if(mSearchState.IsValid() && !mSearchState.mIsGlobal)
			FindAllOccurancesOfWordInVisibleBuffer();
	}

	auto scrollX = ImGui::GetScrollX();
	auto scrollY = ImGui::GetScrollY();


	Cursor& aCursor=GetCurrentCursor();
	mLinePosition.y = GetLinePosition(aCursor.mCursorPosition).y;
	mLinePosition.x = mEditorPosition.x + mLineBarWidth + mPaddingLeft-ImGui::GetScrollX();


	int start = std::min((int)mLines.size()-1,(int)floor(scrollY / mLineHeight));
	int lineCount = (mEditorWindow->Size.y) / mLineHeight;
	int end = std::min(start+lineCount+1,(int)mLines.size());
	// GL_INFO("{} -- {}",start,end);

	//Highlight Selections
	if (HasSelection(aCursor)) {
		Coordinates selectionStart=aCursor.mSelectionStart;
		Coordinates selectionEnd=aCursor.mSelectionEnd;

		if(selectionStart > selectionEnd)
			std::swap(selectionStart,selectionEnd);

		if(selectionStart.mLine==selectionEnd.mLine){

			for(const auto& aCursor:mState.mCursors){
				if(aCursor.mSelectionStart.mLine>=start && aCursor.mSelectionStart.mLine < end){
					int posY=GetLinePosition(aCursor.mCursorPosition).y;
					ImVec2 start(GetSelectionPosFromCoords(aCursor.mSelectionStart), posY);
					ImVec2 end(GetSelectionPosFromCoords(aCursor.mSelectionEnd), posY + mLineHeight);

					mEditorWindow->DrawList->AddRectFilled(start, end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);
				}
			}

		}
		else
		{ 
			// Selection multiple lines
			int start=selectionStart.mLine+1;
			int end=selectionEnd.mLine;
			int diff=end-start;


			float prevLinePositonY=mLinePosition.y;
			if(aCursor.mCursorDirectionChanged){
				mLinePosition.y = GetLinePosition(selectionEnd).y;
			}

			ImVec2 p_start(GetSelectionPosFromCoords(selectionStart), mLinePosition.y-(diff+1)*mLineHeight);
			ImVec2 p_end(mLinePosition.x+GetLineMaxColumn(selectionStart.mLine)*mCharacterSize.x+mCharacterSize.x, mLinePosition.y-diff*mLineHeight);

			mEditorWindow->DrawList->AddRectFilled(p_start, p_end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);


			while(start<end){
				diff=end-start;

				ImVec2 p_start(mLinePosition.x,mLinePosition.y-diff*mLineHeight);
				ImVec2 p_end(p_start.x+GetLineMaxColumn(start)*mCharacterSize.x+mCharacterSize.x,mLinePosition.y-(diff-1)*mLineHeight);

				mEditorWindow->DrawList->AddRectFilled(p_start, p_end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);
				start++;
			}


			p_start={mLinePosition.x, mLinePosition.y};
			p_end={GetSelectionPosFromCoords(selectionEnd), mLinePosition.y + mLineHeight};

			mEditorWindow->DrawList->AddRectFilled(p_start, p_end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);

			if(aCursor.mCursorDirectionChanged){
				mLinePosition.y = prevLinePositonY;
			}
		}
	}




	int i_prev=0;
	mLineBuffer.clear();
	auto drawList = ImGui::GetWindowDrawList();
	float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

	//Rendering Lines and Vertical Indentation Lines
	for (int lineNo=start;lineNo < end;lineNo++) {

		Line& line=mLines[lineNo];
		ImVec2 linePosition = ImVec2(mEditorPosition.x - scrollX, mEditorPosition.y + (lineNo * mLineHeight) - scrollY);
		ImVec2 textScreenPos = ImVec2(linePosition.x + mLineBarWidth + mPaddingLeft, linePosition.y + (0.5 * mLineSpacing));

		// Render colorized text
		auto prevColor = line.empty() ? mPalette[(int)ColorSchemeIdx::Default] : GetGlyphColor(line[0]);
		ImVec2 bufferOffset;

		for (int i = 0; i < line.size();) {
			auto& glyph = line[i];
			auto color = GetGlyphColor(glyph);

			if ((color != prevColor || glyph.mChar == '\t' || glyph.mChar == ' ') && !mLineBuffer.empty()) {
				const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
				drawList->AddText(newOffset, prevColor, mLineBuffer.c_str());
				auto textSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, mLineBuffer.c_str(), nullptr, nullptr);
				bufferOffset.x += textSize.x;
				mLineBuffer.clear();
			}
			prevColor = color;

			if (glyph.mChar == '\t') {
				auto oldX = bufferOffset.x;
				bufferOffset.x += mTabSize*spaceSize;
				++i;
			} else if (glyph.mChar == ' ') {
				bufferOffset.x += spaceSize;
				i++;
			} else {
				auto l = UTF8CharLength(glyph.mChar);
				while (l-- > 0) mLineBuffer.push_back(line[i++].mChar);
			}
		}

		if (!mLineBuffer.empty()) {
			const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
			drawList->AddText(newOffset, prevColor, mLineBuffer.c_str());
			mLineBuffer.clear();
		}


		if(mLines[lineNo].empty()){
			int i=i_prev;
			while(i>-1){
				ImVec2 indentStart{mLinePosition.x+(i*mTabSize*mCharacterSize.x), linePosition.y};
				mEditorWindow->DrawList->AddLine(indentStart, {indentStart.x,indentStart.y+mLineHeight}, mGruvboxPalletDark[(size_t)Pallet::Indentation]);
				i--;
			}
		}else{
			int i=0;
			while(mLines[lineNo].size() > i && mLines[lineNo][i].mChar=='\t'){
				ImVec2 indentStart{mLinePosition.x+(i*mTabSize*mCharacterSize.x), linePosition.y};
				mEditorWindow->DrawList->AddLine(indentStart, {indentStart.x,indentStart.y+mLineHeight}, mGruvboxPalletDark[(size_t)Pallet::Indentation]);
				i++;
			}
			i_prev=--i;
		}


		//Highlighting Brackets
		if(HasBracketMatch())
		{
			if(mBracketMatch.mStartBracket.mLine==lineNo)
				HighlightBracket(mBracketMatch.mStartBracket);

			if(mBracketMatch.mEndBracket.mLine==lineNo)
				HighlightBracket(mBracketMatch.mEndBracket);
		}
	}





	//Cursors
	for(const Cursor& cursor:mState.mCursors){
		ImVec2 linePos = GetLinePosition(cursor.mCursorPosition);
		float cx = TextDistanceFromLineStart(cursor.mCursorPosition);

		ImVec2 cstart(linePos.x+mLineBarWidth+mPaddingLeft + cx - 1.0f, linePos.y);
		ImVec2 cend(linePos.x+mLineBarWidth+mPaddingLeft + cx+1.0f, linePos.y + mLineHeight);
		mEditorWindow->DrawList->AddRectFilled(cstart,cend,ImColor(255, 255, 255, 255));
	}






	//Line Number Background
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition}, {mEditorPosition.x + mLineBarWidth, mEditorSize.y}, mGruvboxPalletDark[(size_t)Pallet::Background]); // LineNo
	// Highlight Current Lin
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition.x,mEditorPosition.y+(aCursor.mCursorPosition.mLine*mLineHeight)-scrollY},{mEditorPosition.x+mLineBarWidth, mEditorPosition.y+(aCursor.mCursorPosition.mLine*mLineHeight)-scrollY + mLineHeight},mGruvboxPalletDark[(size_t)Pallet::Highlight]); // Code
	mLineHeight = mLineSpacing + mCharacterSize.y;

	//Horizonal scroll Shadow
	if(ImGui::GetScrollX()>0.0f){
		ImVec2 pos_start{mEditorPosition.x+mLineBarWidth,0.0f};
		mEditorWindow->DrawList->AddRectFilledMultiColor(pos_start,{pos_start.x+15.0f,mEditorWindow->Size.y}, ImColor(19,21,21,130),ImColor(19,21,21,0),ImColor(19,21,21,0),ImColor(19,21,21,130));
	}


	if(mSearchState.IsValid())
	{
		HighlightCurrentWordInBuffer();

		//Rendering the indicator at lineNumber containing the match
		if(HasSelection(GetCurrentCursor()))
		{
			for(const auto& coord:mSearchState.mFoundPositions)
			{
				float linePosY = GetLinePosition(coord).y;
				ImVec2 start{mEditorPosition.x,linePosY};
				ImVec2 end{start.x+4.0f,linePosY+mLineHeight};

				mEditorWindow->DrawList->AddRectFilled(start,end, mGruvboxPalletDark[(size_t)Pallet::Text]);
			}
		}
	}

	//Rendering Line Number
	for (int lineNo=start;lineNo<end;lineNo++) {
		float linePosY =mEditorPosition.y + (lineNo * mLineHeight) + 0.5f*mLineSpacing - scrollY;
		float linePosX=mEditorPosition.x + mLineBarPadding + (mLineBarMaxCountWidth-GetNumberWidth(lineNo+1))*mCharacterSize.x;

		mEditorWindow->DrawList->AddText({linePosX, linePosY}, (lineNo==aCursor.mCursorPosition.mLine) ? mGruvboxPalletDark[(size_t)Pallet::Text] : mGruvboxPalletDark[(size_t)Pallet::Comment], std::to_string(lineNo + 1).c_str());
	}

	ImGuiID hover_id = ImGui::GetHoveredID();
	bool scrollbarHovered = hover_id && (hover_id == ImGui::GetWindowScrollbarID(mEditorWindow, ImGuiAxis_X) || hover_id == ImGui::GetWindowScrollbarID(mEditorWindow, ImGuiAxis_Y));
	if(scrollbarHovered) 
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

	HandleKeyboardInputs();
	HandleMouseInputs();

	ImGui::EndChild();
	ImGui::PopStyleVar();
// #ifdef GL_DEBUG
// 	DebugDisplayNearByText();
// #endif
	return true;
}

void Editor::HighlightBracket(const Coordinates& aCoords){
	if (aCoords.mColumn >= mLines[aCoords.mLine].size()) 
		return;

	int column=GetCharacterColumn(aCoords.mLine,aCoords.mColumn);
	int tabs=GetTabCountsUptoCursor(aCoords)*(mTabSize-1);
	float linePositionY=GetLinePosition(aCoords).y;

	ImVec2 start{mLinePosition.x+column*mCharacterSize.x,linePositionY};
	mEditorWindow->DrawList->AddRect(start,{start.x+mCharacterSize.x+1,start.y+mLineHeight}, mGruvboxPalletDark[(size_t)Pallet::HighlightOne]);
}


void Editor::FindBracketMatch(const Coordinates& aCoords)
{
	OpenGL::ScopedTimer timer("Editor::FindBracketMatch");
	mBracketMatch.mHasMatch=false;

	const Coordinates cStart=FindStartBracket(aCoords);
	if(cStart.mLine==INT_MAX) 
		return;

	GL_INFO("BracketMatch Start:[{},{}]",cStart.mLine,cStart.mColumn);

	const Coordinates cEnd=FindEndBracket(aCoords);
	
	if(cEnd.mLine==INT_MAX) 
		return;

	mBracketMatch.mHasMatch=true;
	mBracketMatch.mStartBracket=cStart;
	mBracketMatch.mEndBracket=cEnd;
	GL_INFO("BracketMatch End:[{},{}]",mBracketMatch.mEndBracket.mLine,mBracketMatch.mEndBracket.mColumn);
}

Coordinates Editor::FindStartBracket(const Coordinates& coords){
	Coordinates coord{INT_MAX,INT_MAX};

	int cLine=coords.mLine;
	int cColumn=std::max(0,(int)GetCharacterIndex(coords)-1);

	bool isFound=false;
	int ignore=0;
	char x=-1;

	for(;cLine>=0;cLine--)
	{
		const auto& line=mLines[cLine];
		if(line.empty()){
			if(cLine>0 && !mLines[cLine - 1].empty()) 
				cColumn=mLines[cLine - 1].size() - 1;
			else cColumn=0;
			continue;
		}

		for(;cColumn>=0;cColumn--){
			if(line[cColumn].mChar==x){ x=-1; continue; }
			if(line[cColumn].mChar=='\'' || line[cColumn].mChar=='"'){
				x=line[cColumn].mChar; 
				continue; 
			}
			if(x!=-1) continue;

			switch(line[cColumn].mChar) {
				case ')': ignore++;break;
				case ']': ignore++;break;
				case '}': ignore++;break;
				case '(':{if(!ignore){isFound=true;} if(ignore>0) ignore--; break; }
				case '[':{if(!ignore){isFound=true;} if(ignore>0) ignore--; break; }
				case '{':{if(!ignore){isFound=true;} if(ignore>0) ignore--; break; }
			}

			if(isFound)	{
				coord.mLine=cLine;
				coord.mColumn=cColumn;
				return coord;
			}
		}

		if(cLine>0 && !mLines[cLine - 1].empty()) 
			cColumn=mLines[cLine - 1].size() - 1;
		else cColumn=0;
	}
	return coord;
}


Coordinates Editor::FindEndBracket(const Coordinates& coords){
	Coordinates coord{INT_MAX,INT_MAX};

	int cLine=coords.mLine;
	int cColumn=(int)GetCharacterIndex(coords);

	int ignore=0;
	bool isFound=false;
	char stringQuotes=-1;
	for(;cLine < mLines.size();cLine++)
	{
		const auto& line=mLines[cLine];
		if(line.empty()) continue;

		for(;cColumn < line.size();cColumn++)
		{
			//Ignoring the brackets inside quotes
			if(line[cColumn].mChar==stringQuotes){ stringQuotes=-1; continue; }
			if(line[cColumn].mChar=='\'' || line[cColumn].mChar=='"'){
				stringQuotes=line[cColumn].mChar;
				continue; 
			}
			if(stringQuotes!=-1) continue;


			switch(line[cColumn].mChar){
				case '(': ignore++; break;
				case '[': ignore++; break;
				case '{': ignore++; break;
				case ')':{if(!ignore) isFound=true; if(ignore>0) ignore--; break; }
				case ']':{if(!ignore) isFound=true; if(ignore>0) ignore--; break; }
				case '}':{if(!ignore) isFound=true; if(ignore>0) ignore--; break; }
			}

			if(isFound){
				coord.mLine=cLine;
				coord.mColumn=cColumn;
				return coord;
			}
		}
		cColumn=0;
	}
	return coord;
}

// Function to set PaletteIndex based on Tree-sitter node type
Editor::ColorSchemeIdx Editor::GetColorSchemeIndexForNode(const std::string &type)
{
    if (type == "type" || type=="keyword" || type=="statement" || type=="function.namespace")
        return Editor::ColorSchemeIdx::Keyword;
    else if (type == "type.indentifier")
        return Editor::ColorSchemeIdx::Identifier;
    else if (type == "string")
        return Editor::ColorSchemeIdx::String;
    else if (type == "primitive_type" || type=="number")
        return Editor::ColorSchemeIdx::Number;
    else if (type == "comment")
        return Editor::ColorSchemeIdx::Comment;
    else if (type == "assignment")
        return Editor::ColorSchemeIdx::KnownIdentifier;
    else if (type == "function")
        return Editor::ColorSchemeIdx::KnownIdentifier;
    else if (type == "primitive_type")
        return Editor::ColorSchemeIdx::Preprocessor;
    else if (type == "namespace")
        return Editor::ColorSchemeIdx::String;
    else if (type == "scope_resolution")
        return Editor::ColorSchemeIdx::Default;
    else
        return Editor::ColorSchemeIdx::Default;
}

void Editor::ReparseEntireTree(){
	std::string sourceCode=GetFullText();
	ApplySyntaxHighlighting(sourceCode);
	GL_WARN("Reparsing:{} Bytes",sourceCode.size());
}


void Editor::DebouncedReparse()
{
	{
        std::lock_guard<std::mutex> lock(mutex_);
        needsUpdate_ = true;
    }
    cv_.notify_one();
}

void Editor::WorkerThread() {
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        cv_.wait(lock, [this] { return needsUpdate_ || terminate_; });
        
        if (terminate_) {
            break;
        }
        
        // Reset the update flag before waiting
        needsUpdate_ = false;
        
        // Release lock while waiting for debounce period
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        lock.lock();
        
        // Only process if no new updates came in during wait
        if (!needsUpdate_) {
            lock.unlock();
            ReparseEntireTree();
        }
    }
}


// Function to update Glyphs using Tree-sitter
void Editor::ApplySyntaxHighlighting(const std::string &sourceCode)
{
	if(sourceCode.size() <2  || !mIsSyntaxHighlightingSupportForFile) return;
	OpenGL::ScopedTimer timer("ApplySyntaxHighlighting");
	// Initialize Tree-sitter parser
    TSParser* parser= ts_parser_new();
    ts_parser_set_language(parser,tree_sitter_cpp());

    TSTree* tree = ts_parser_parse_string(parser, nullptr, sourceCode.c_str(), sourceCode.size());

	static std::string query_string = R"(
	[   
		"if" 
		"else" 
		"while" 
		"do" 
		"for" 
		"switch" 
		"case" 
		"default" 
		"return" 
		"break" 
		"continue"
		"static"
		"const"
	] @keyword

	(auto) @keyword

	; anything.[captured]
	(field_expression
		field:(field_identifier) @assignment)

	;array access captured[]
	(subscript_expression
		argument:(identifier) @assignment)

	(number_literal) @number
	(true) @number
	(false) @number
	(auto) @type

	; Constants
	(this) @number
	(null "nullptr" @number)

	(type_identifier) @type.indentifier
	(parameter_declaration
		(identifier) @scope_resolution) 
	(comment) @comment 
	(string_literal) @string 
	(raw_string_literal) @string
	(primitive_type) @keyword
	(function_declarator declarator:(_) @function) 
	;(namespace_identifier) @namespace

	;Preprocessor
	(preproc_include (string_literal) @string)
	(preproc_include (system_lib_string) @string)
	(preproc_include) @keyword

    (preproc_def) @keyword
    (preproc_def name:(identifier) @type.indentifier)
    (preproc_ifdef) @keyword
    (preproc_ifdef name:(identifier) @type.indentifier)
    (preproc_else) @keyword
    (preproc_directive) @keyword

	"::" @scope_resolution
	; Keywords
	[
	  "try"
	  "catch"
	  "noexcept"
	  "throw"
	] @keyword

	[
	  "decltype"
	  "explicit"
	  "friend"
	  "override"
	  "using"
	  "requires"
	  "constexpr"
	] @keyword

	[
	  "class"
	  "namespace"
	  "template"
	  "typename"
	  "concept"
	] @keyword

	(access_specifier) @keyword

	[
	  "co_await"
	  "co_yield"
	  "co_return"
	] @keyword

	[
	  "new"
	  "delete"
	  "xor"
	  "bitand"
	  "bitor"
	  "compl"
	  "not"
	  "xor_eq"
	  "and_eq"
	  "or_eq"
	  "not_eq"
	  "and"
	  "or"
	] @keyword


	; functions
	(call_expression
		function: (identifier) @function)

	(function_declarator
	  (qualified_identifier
	    name: (identifier) @function.namespace))

	(function_declarator
	  (template_function
	    (identifier) @function))

	(operator_name) @function

	"operator" @function

	"static_assert" @function

	(call_expression
	  (qualified_identifier
	    (identifier) @function))

	(call_expression
	  (template_function
	    (identifier) @function))

	((namespace_identifier) @namespace
			(#match? @namespace "^[A-Z]"))

    )";

	// Define a simple query to match syntax elements
    OpenGL::Timer timerx;
	uint32_t error_offset;
	TSQueryError error_type;
	if(!mQuery)
		mQuery= ts_query_new(tree_sitter_cpp(), query_string.c_str(), query_string.size(), &error_offset, &error_type);

	// Check if the query was successfully createD
	if (!mQuery) {
		GL_ERROR("Error creating query at offset {}, error type: {}", error_offset, error_type);
		ts_tree_delete(tree);
		ts_parser_delete(parser);
		ts_query_delete(mQuery);
		return;
	}
    char buff[32];
    sprintf_s(buff,"%fms",timerx.ElapsedMillis());
    StatusBarManager::ShowNotification("Query Execution:", buff,StatusBarManager::NotificationType::Success);

	TSQueryCursor* cursor = ts_query_cursor_new();
	ts_query_cursor_exec(cursor, mQuery, ts_tree_root_node(tree));
    // GL_INFO("Duration:{}",timerx.ElapsedMillis());

	// 5. Highlight matching nodes
	TSQueryMatch match;
	while (ts_query_cursor_next_match(cursor, &match)) {
		for (unsigned int i = 0; i < match.capture_count; ++i) {
			TSQueryCapture capture = match.captures[i];
			TSNode node = capture.node;

		    // Get the type of the current node
		    const char *nodeType = ts_node_type(node);
		    unsigned int length;
		    const char *captureName = ts_query_capture_name_for_id(mQuery, capture.index, &length);
			// GL_INFO(match.captures[i].index);
			// GL_INFO(captureName);
		    // GL_INFO(nodeType);

		    TSPoint startPoint = ts_node_start_point(node);
		    TSPoint endPoint = ts_node_end_point(node);
		    endPoint.column--;

		    ColorSchemeIdx colorIndex = GetColorSchemeIndexForNode(captureName);
		    // GL_INFO("[{},{}]-[{},{}]:{}--{}",startPoint.row,startPoint.column,endPoint.row,endPoint.column,nodeType,(int)colorIndex);

		    for (unsigned int row = startPoint.row; row < mLines.size() && row <= endPoint.row; ++row)
		    {
		        for (unsigned int column = (row == startPoint.row ? startPoint.column : 0);
		             column < mLines[row].size() && (row < endPoint.row || column <= endPoint.column); ++column)
		        {
		            mLines[row][column].mColorIndex = colorIndex;
				    // GL_INFO("[{},{}]-[{},{}]:{}--{}--{}",startPoint.row,startPoint.column,endPoint.row,endPoint.column,nodeType,(int)colorIndex,(char)mLines[row][column].mChar);
		        }
		    }
		}
	}

    // Get the start and end positions of the node
    ts_tree_delete(tree);
	ts_parser_delete(parser);
	ts_query_cursor_delete(cursor);
	//Don't free mQuery as I am reusing it
}




void Editor::PrintTree(const TSNode &node, const std::string &source_code,std::string& output, int indent){
    // Create indentation for structured formatting
    std::string indent_space(indent * 2, ' ');

    // Get the type of the node and its text range
    const char *node_type = ts_node_type(node);
    uint32_t start_byte = ts_node_start_byte(node);
    uint32_t end_byte = ts_node_end_byte(node);

    // Extract text for the node from the source code
    std::string node_text = source_code.substr(start_byte, end_byte - start_byte);

    std::ostringstream oss;
    // Print the node information
    oss << indent_space << node_type << " [" << start_byte << ":" << end_byte
              << "]\n";
    // ImGui::Text("%s", oss.str().c_str());
    output+=oss.str();
    // Recursively print child nodes
    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        PrintTree(ts_node_child(node, i), source_code,output, indent + 1);
    }
}


std::string Editor::GetNearbyLinesString(int aLineNo,int aLineCount) {
    std::string result;

    // Determine the range of lines to include
    int startLine = std::max(0, aLineNo - aLineCount);                       // One line above (if it exists)
    int endLine = std::min((int)mLines.size() - 1, aLineNo + aLineCount);   // One line below (if it exists)

    // Concatenate the lines within the range
    for (int lineIndex = startLine; lineIndex <= endLine; ++lineIndex) {
        for (const Glyph& glyph : mLines[lineIndex]) {
            result += (char)glyph.mChar;
        }
        result += '\n';
    }

    return result;
}

void Editor::UpdateSyntaxHighlighting(int aLineNo,int aLineCount)
{
	if(!mIsSyntaxHighlightingSupportForFile || !mQuery) return;

	OpenGL::ScopedTimer timer("UpdateSyntaxHighlighting");
	std::string sourceCode=GetNearbyLinesString(aLineNo,aLineCount);
	if(sourceCode.size() < 2) return;

    TSParser* parser= ts_parser_new();
    ts_parser_set_language(parser,tree_sitter_cpp());

    TSTree* tree = ts_parser_parse_string(parser, nullptr, sourceCode.c_str(), sourceCode.size());

	TSQueryCursor* cursor = ts_query_cursor_new();
	ts_query_cursor_exec(cursor, mQuery, ts_tree_root_node(tree));

	// Rest Color
	// int start=std::max(0,aLineNo - aLineCount);
	// int end=std::min((int)mLines.size()-1,aLineNo+aLineCount);
	// for(int row=start;row<=end;row++){
	// 	for(auto& glyph:mLines[row])
	// 		glyph.mColorIndex=ColorSchemeIdx::Default;
	// }

	TSQueryMatch match;
	while (ts_query_cursor_next_match(cursor, &match)) {
		for (unsigned int i = 0; i < match.capture_count; ++i) {
			TSQueryCapture capture = match.captures[i];
			TSNode node = capture.node;

		    // Get the type of the current node
		    const char *nodeType = ts_node_type(node);
		    unsigned int length;
		    const char *captureName = ts_query_capture_name_for_id(mQuery, capture.index, &length);

		    int startLine = std::max(0, aLineNo-aLineCount);
		    TSPoint startPoint = ts_node_start_point(node);
		    startPoint.row+=startLine;
		    TSPoint endPoint = ts_node_end_point(node);
		    endPoint.row+=startLine;

		    ColorSchemeIdx colorIndex = GetColorSchemeIndexForNode(captureName);
            // GL_INFO("Range:({},{}) -> ({},{})",startPoint.row,startPoint.column,endPoint.row,endPoint.column);
		    for (unsigned int row = startPoint.row; row < mLines.size() && row <= endPoint.row; ++row)
		    {
		        for (unsigned int column = (row == startPoint.row ? startPoint.column : 0);
		             column < mLines[row].size() && (row < endPoint.row || column < endPoint.column); ++column)
		        {
		            mLines[row][column].mColorIndex = colorIndex;
		        }
		    }
		}
	}
	
    ts_tree_delete(tree);
    ts_parser_delete(parser);
    ts_query_cursor_delete(cursor);
}

const Editor::Palette& Editor::GetGruvboxPalette()
{
	constexpr const static Palette p = {{
	    IM_COL32(40, 40, 40, 255),    // BG (#282828)
	    IM_COL32(204, 36, 29, 255),   // RED (#cc241d)
	    IM_COL32(152, 151, 26, 255),  // GREEN (#98971a)
	    IM_COL32(215, 153, 33, 255),  // YELLOW (#d79921)
	    IM_COL32(69, 133, 136, 255),  // BLUE (#458588)
	    IM_COL32(177, 98, 134, 255),  // PURPLE (#b16286)
	    IM_COL32(104, 157, 106, 255), // AQUA (#689d6a)
	    IM_COL32(168, 153, 132, 255), // GRAY (#a89984)
	    IM_COL32(29, 32, 33, 255),    // BG0_H (#1d2021)
	    IM_COL32(40, 40, 40, 255),    // BG0 (#282828)
	    IM_COL32(50, 48, 47, 255),    // BG0_S (#32302f)
	    IM_COL32(60, 56, 54, 255),    // BG1 (#3c3836)
	    IM_COL32(80, 73, 69, 255),    // BG2 (#504945)
	    IM_COL32(102, 92, 84, 255),   // BG3 (#665c54)
	    IM_COL32(124, 118, 100, 255), // BG4 (#7c6f64)
	    IM_COL32(235, 219, 178, 255), // FG (#ebdbb2)
	    IM_COL32(251, 241, 199, 255), // FG0 (#fbf1c7)
	    IM_COL32(235, 219, 178, 255), // FG1 (#ebdbb2)
	    IM_COL32(213, 196, 161, 255), // FG2 (#d5c4a1)
	    IM_COL32(189, 174, 147, 255), // FG3 (#bdae93)
	    IM_COL32(168, 153, 132, 255), // FG4 (#a89984)
	    IM_COL32(214, 93, 14, 255),   // ORANGE (#d65d0e)
	    IM_COL32(251, 73, 52, 255),   // LIGHT_RED (#fb4934)
	    IM_COL32(184, 187, 38, 255),  // LIGHT_GREEN (#b8bb26)
	    IM_COL32(131, 165, 152, 255), // LIGHT_BLUE (#83a598)
	    IM_COL32(211, 134, 155, 255), // LIGHT_PURPLE (#d3869b)
	    IM_COL32(250, 189, 47, 255),  // LIGHT_YELLOW (#fabd2f)
	    IM_COL32(142, 192, 124, 255), // LIGHT_AQUA (#8ec07c)
	    IM_COL32(254, 128, 25, 255),  // BRIGHT_ORANGE (#fe8019)
	    IM_COL32(168, 153, 132, 255), // FG_HIGHLIGHT (Repeated FG4 #a89984)
	    IM_COL32(92, 84, 78, 255)     // FG_DARK (#5c544e)
	}};

	return p;
}



ImU32 Editor::GetGlyphColor(const Glyph& aGlyph) const
{
	return mPalette[(int)aGlyph.mColorIndex];
}


int Editor::GetLineMaxColumn(int aLine) const
{
	if (aLine >= mLines.size())
		return 0;
	auto& line = mLines[aLine];
	int col = 0;
	for (unsigned i = 0; i < line.size();) {
		auto c = line[i].mChar;
		if (c == '\t')
			col += mTabSize;
		else
			col++;
		i += UTF8CharLength(c);
	}
	return col;
}

int Editor::GetCurrentLineMaxColumn() 
{ 
	Cursor& aCursor=GetCurrentCursor();
	return GetLineMaxColumn(aCursor.mCursorPosition.mLine); 
}


uint8_t Editor::GetTabCountsUptoCursor(const Coordinates& aCoords)const
{
	uint8_t tabCounts = 0;
	int i = 0;

	int max=GetCharacterIndex(aCoords);
	const auto& line=mLines[aCoords.mLine];
	for (; i < max;) 
	{
		if (line[i].mChar == '\t')
			tabCounts++;

		i+=UTF8CharLength(line[i].mChar);
	}

	return tabCounts;
}

uint8_t Editor::GetTabCountsAtLineStart(const Coordinates& aCoords){
	uint8_t tabCounts = 0;
	int i = 0;

	int max=GetCharacterIndex(aCoords);
	const auto& line=mLines[aCoords.mLine];
	for (; i < max;) 
	{
		if (line[i].mChar == '\t')
			tabCounts++;
		else 
			break;

		i+=UTF8CharLength(line[i].mChar);
	}

	return tabCounts;
}



size_t Editor::GetCharacterIndex(const Coordinates& aCoords) const
{
	if (aCoords.mLine >= mLines.size())
		return -1;
	auto& line = mLines[aCoords.mLine];
	int c = 0;
	int i = 0;
	for (; i < line.size() && c < aCoords.mColumn;)
	{
		if (line[i].mChar == '\t')
			c += mTabSize;
		else
			++c;
		i += UTF8CharLength(line[i].mChar);
	}
	return i;
}





float Editor::GetSelectionPosFromCoords(const Coordinates& coords)const{
	float offset{0.0f};
	if(coords==mState.mCursors[mState.mCurrentCursorIdx].mSelectionStart) offset=-1.0f;
	return mLinePosition.x - offset + (coords.mColumn * mCharacterSize.x);
}


void Editor::HighlightCurrentWordInBuffer() {

	int start = std::min((int)mLines.size()-1,(int)floor(ImGui::GetScrollY() / mLineHeight));
	int lineCount = (mEditorWindow->Size.y) / mLineHeight;
	int end = std::min(start+lineCount+1,(int)mLines.size());
	bool hasSelection=HasSelection(GetCurrentCursor());

	for(const Coordinates& coord:mSearchState.mFoundPositions){
		// if(coord.mLine==mState.mSelectionStart.mLine && coord.mColumn==mState.mSelectionStart.mColumn) continue;

		if((coord.mLine < start || coord.mLine > end)) continue;

		ImVec2 linePos=GetLinePosition(coord);
		float offset=hasSelection ?  1.0f : (mLineHeight+1.0f-0.5f*mLineSpacing);
		float linePosY = linePos.y + offset;
		

		ImVec2 start{mLinePosition.x+coord.mColumn*mCharacterSize.x-hasSelection,linePosY};
		ImVec2 end{start.x+mSearchState.mWordLen*mCharacterSize.x+(hasSelection*2),linePosY+(hasSelection ? mLineHeight : 0.0f)};
		ImDrawList* drawlist=ImGui::GetCurrentWindow()->DrawList;

		if(hasSelection)
			drawlist->AddRect(start,end, mGruvboxPalletDark[(size_t)Pallet::HighlightOne]);
		else
			drawlist->AddLine(start,end, mGruvboxPalletDark[(size_t)Pallet::HighlightOne]);
	}
}

// FIX: Needs a find function to search for a substring in buffer
void Editor::FindAllOccurancesOfWord(std::string word,size_t aStartLineIdx,size_t aEndLineIdx){

	OpenGL::ScopedTimer timer("Editor::FindAllOccurancesOfWord");
	mSearchState.mFoundPositions.clear();


	for(size_t i=aStartLineIdx;i<=aEndLineIdx;i++){
		auto& line=mLines[i];
		if(line.empty()) continue;

		size_t searchOffset=0;
		std::string clText=GetText(Coordinates(i,0),Coordinates(i,GetLineMaxColumn(i)));
        while ((searchOffset = clText.find(word, searchOffset)) != std::string::npos) {

            size_t startIndex = searchOffset;
            size_t endIndex = searchOffset + word.length() - 1;


            GL_TRACE("Line {} : Found '{}' at [{},{}] ",i+1,word,startIndex,endIndex);

            mSearchState.mFoundPositions.emplace_back(i,GetCharacterColumn(i,startIndex));
            searchOffset = endIndex + 1;
        }
	}
}

std::string Editor::GetWordAt(const Coordinates& aCoords) const
{
	auto start = FindWordStart(aCoords);
	auto end = FindWordEnd(aCoords);

	std::string r;

	auto istart = GetCharacterIndex(start);
	auto iend = GetCharacterIndex(end);

	for (auto it = istart; it < iend; ++it) r.push_back(mLines[aCoords.mLine][it].mChar);

	return r;
}



void Editor::FindAllOccurancesOfWordInVisibleBuffer()
{
	auto& aCursor=GetCurrentCursor();
	//Get character idx
	auto [start_idx,end_idx] = GetIndexOfWordAtCursor(aCursor.mCursorPosition);
	if(start_idx==end_idx) return;

	std::string currentWord;
	for(int i=start_idx;i<end_idx;i++)
		currentWord+=(char)mLines[aCursor.mCursorPosition.mLine][i].mChar;

	GL_INFO("text:{} StartIDx:{}  EndIdx:{}",currentWord,start_idx,end_idx);

	if(currentWord.empty() || currentWord.size()<2)return;

	if(mSearchState.mWord==currentWord) return;

	mSearchState.Reset();
	mSearchState.SetSearchWord(currentWord);
	mSearchState.mIsGlobal=false;


	GL_WARN("Searching: {}",currentWord);

	int start = std::min((int)mLines.size()-1,(int)floor(ImGui::GetScrollY() / mLineHeight));
	int lineCount = (mEditorWindow->Size.y) / mLineHeight;
	int end = std::min(start+lineCount+1,(int)mLines.size()-1);

	FindAllOccurancesOfWord(currentWord, start, end);
}

// FIX: Unicode support
std::pair<int, int> Editor::GetIndexOfWordAtCursor(const Coordinates& coords) const {
    int idx = GetCharacterIndex(coords);
    int start_idx = idx;
    int end_idx = idx;

    bool search_left = true, search_right = true;

    while (search_left || search_right) {
        char chr = 0;

        // Searching left
        if (search_left) {
            if (start_idx <= 0) {
                search_left = false;
                start_idx = 0;
            } else {
                int len = UTF8CharLength(static_cast<uint8_t>(mLines[coords.mLine][start_idx - 1].mChar));
                int prev_start_idx = start_idx - len;

                if (prev_start_idx < 0) {
                    search_left = false;
                    start_idx = 0;
                } else {
                    chr = mLines[coords.mLine][prev_start_idx].mChar;
                    if(IsUTFSequence(chr)|| !(isalnum(chr) || chr == '_'))
                    	search_left=false;
                    else
                        start_idx = prev_start_idx;
                }
            }
        }

        // Searching right
        if (search_right) {
            if (end_idx >= static_cast<int>(mLines[coords.mLine].size())) {
                search_right = false;
                end_idx = mLines[coords.mLine].size();
            } else {
                chr = mLines[coords.mLine][end_idx].mChar;

                if ( chr < 0 ||  IsUTFSequence(chr) || !(isalnum(chr) || chr == '_')) {
                    search_right = false;
                } else {
                    end_idx += UTF8CharLength(static_cast<uint8_t>(chr));
                }
            }
        }
    }

    return {start_idx, end_idx};
}



void Editor::HandleCtrlD(){
	//First time
	auto& aCursor=GetCurrentCursor();
	if(mSelectionMode!=SelectionMode::Word)
	{
		if(!HasSelection(aCursor))
			SelectWordUnderCursor(aCursor);

		if(!HasSelection(aCursor))
			return;

		mSelectionMode=SelectionMode::Word;
		std::string word=GetText(aCursor.mSelectionStart,aCursor.mSelectionEnd);

		mSearchState.SetSearchWord(word);
		mSearchState.mIsGlobal=true;

		GL_INFO("WORD:{} Len:{}",word,mSearchState.mWordLen);

		FindAllOccurancesOfWord(word,0,mLines.size()-1);

		// Finding Index of Position same as currentLine to get next occurance
		auto it = std::find_if(
			mSearchState.mFoundPositions.begin(), mSearchState.mFoundPositions.end(),
		    [&](const auto& coord) { return coord.mLine == aCursor.mCursorPosition.mLine; 
		});

		if (it != mSearchState.mFoundPositions.end())
		{
			const Coordinates& coord=*it;

			mSearchState.mIdx = std::min(
				(int)mSearchState.mFoundPositions.size() - 1,
			    (int)std::distance(mSearchState.mFoundPositions.begin(), it) + 1
			);
		}
	}
	else
	{
		if(!mSearchState.IsValid()) return;
		// assert(false && "Feature not implemented!");

		GL_INFO("Finding Next");
		const Coordinates& coord = mSearchState.mFoundPositions[mSearchState.mIdx];
		ScrollToLineNumber(coord.mLine);

		Cursor nCursor;
		nCursor.mSelectionStart = nCursor.mSelectionEnd = coord;
		nCursor.mSelectionEnd.mColumn = coord.mColumn + mSearchState.mWordLen;
		GL_INFO("[{}  {} {}]", nCursor.mSelectionStart.mColumn, nCursor.mSelectionEnd.mColumn, mSearchState.mWord.size());

		nCursor.mCursorPosition = nCursor.mSelectionEnd;
		mState.mCursors.push_back(nCursor);
		mState.mCurrentCursorIdx++;
		mSearchState.mIdx++;

		if (mSearchState.mIdx == mSearchState.mFoundPositions.size())
			mSearchState.mIdx = 0;
	}
}




Coordinates Editor::ScreenPosToCoordinates(const ImVec2& aPosition) const
{
	ImVec2 origin = mEditorWindow->Pos;
	ImVec2 scroll = mEditorWindow->Scroll;
	ImVec2 local(aPosition.x - origin.x + scroll.x, aPosition.y - origin.y + scroll.y);
	GL_INFO("WinY:{} MosY:{} local:{}",origin.y,aPosition.y,aPosition.y-origin.y+scroll.y);

	int lineNo = std::max(0, (int)floor(local.y / mLineHeight));

	int columnCoord = 0;

	if (lineNo >= 0 && lineNo < (int)mLines.size()) {
		auto& line = mLines.at(lineNo);

		int columnIndex = 0;
		float columnX = 0.0f;

		while ((size_t)columnIndex < line.size()) {
			float columnWidth = 0.0f;

			if (line[columnIndex].mChar == '\t') {
				float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ").x;
				float oldX = columnX;
				float newColumnX = (1.0f + std::floor((1.0f + columnX) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
				columnWidth = newColumnX - oldX;
				if (mLineBarWidth + mPaddingLeft + columnX + columnWidth * 0.5f > local.x)
					break;
				columnX = newColumnX;
				columnCoord = (columnCoord / mTabSize) * mTabSize + mTabSize;
				columnIndex++;
			} else {
				char buf[7];
				auto d = UTF8CharLength(line[columnIndex].mChar);
				int i = 0;
				while (i < 6 && d-- > 0) buf[i++] = line[columnIndex++].mChar;
				buf[i] = '\0';
				columnWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf).x;
				if (mLineBarWidth + mPaddingLeft + columnX + columnWidth * 0.5f > local.x)
					break;
				columnX += columnWidth;
				columnCoord++;
			}
		}
	}

	return SanitizeCoordinates(Coordinates(lineNo, columnCoord));
}




int Editor::GetCharacterColumn(int aLine, int aIndex) const
{
	if (aLine >= mLines.size())
		return 0;
	auto& line = mLines[aLine];
	int col = 0;
	int i = 0;
	while (i < aIndex && i < (int)line.size()) {
		auto c = line[i].mChar;
		i += UTF8CharLength(c);
		if (c == '\t')
			col += mTabSize;
		else
			col++;
	}
	return col;
}



void Editor::ScrollToLineNumber(int aToLine,bool aAnimate){

	aToLine=std::max(0,std::min((int)mLines.size()-1,aToLine));

	int start = std::min((int)mLines.size()-1,(int)floor(ImGui::GetScrollY() / mLineHeight));
	int lineCount = (mEditorWindow->Size.y) / mLineHeight;
	int end = std::min(start+lineCount+1,(int)mLines.size()) - 1;
	int currLine=GetCurrentCursor().mCursorPosition.mLine;

	GL_INFO("StartLine:{} EndLine:{}",start,end);

	if(aToLine > start && aToLine < end) return;

	int diff=std::abs(aToLine-currLine+2);
	mScrollAmount=diff*mLineHeight;

	if(!aAnimate){
		ImGui::SetScrollY(ImGui::GetScrollY() + mScrollAmount);
		return;
	}

	//Handling quick change in nextl
	if((ImGui::GetTime()-mLastClick)<0.25f){
		ImGui::SetScrollY(ImGui::GetScrollY() + mScrollAmount);
	}else{
		mInitialScrollY=ImGui::GetScrollY();
		mScrollAnimation.start();
	}

	mLastClick=(float)ImGui::GetTime();
}

Coordinates Editor::SanitizeCoordinates(const Coordinates& aValue) const
{
	auto line = aValue.mLine;
	auto column = aValue.mColumn;
	if (line >= (int)mLines.size()) {
		if (mLines.empty()) {
			line = 0;
			column = 0;
		} else {
			line = (int)mLines.size() - 1;
			column = GetLineMaxColumn(line);
		}
		return Coordinates(line, column);
	} else {
		column = mLines.empty() ? 0 : std::min(column, GetLineMaxColumn(line));
		return Coordinates(line, column);
	}
}

std::string Editor::GetText()
{
	int colMax=GetLineMaxColumn(mLines.size()-1);
	return GetText({0,0},{(int)mLines.size(),colMax});
}

std::string Editor::GetText(const Coordinates& aStart, const Coordinates& aEnd) const
{
	assert(aStart < aEnd);

	std::string result;
	auto lstart = aStart.mLine;
	auto lend = aEnd.mLine;
	auto istart = GetCharacterIndex(aStart);
	auto iend = GetCharacterIndex(aEnd);
	size_t s = 0;

	for (size_t i = lstart; i < lend; i++)
		s += mLines[i].size();

	result.reserve(s + s / 8);

	while (istart < iend || lstart < lend)
	{
		if (lstart >= (int)mLines.size())
			break;

		auto& line = mLines[lstart];
		if (istart < (int)line.size())
		{
			result += line[istart].mChar;
			istart++;
		}
		else
		{
			istart = 0;
			++lstart;
			result += '\n';
		}
	}

	return result;
}


std::string Editor::GetSelectedText(const Cursor& aCursor) const { 
	return std::move(GetText(aCursor.mSelectionStart, aCursor.mSelectionEnd)); 
}

void Editor::Copy()
{
	OpenGL::ScopedTimer timer("Editor::Copy");
	Cursor& aCursor=GetCurrentCursor();
	if (HasSelection(aCursor)) 
	{
		if(aCursor.mSelectionStart>aCursor.mSelectionEnd)
			std::swap(aCursor.mSelectionStart,aCursor.mSelectionEnd);

		std::string text=GetSelectedText(aCursor);
		GL_INFO("Selected Text:\n{}",text);

		if(	!text.empty())
			ImGui::SetClipboardText(text.c_str());
	} 
	else 
	{
		if (!mLines.empty()) 
		{
			std::string str;
			auto& line = mLines[SanitizeCoordinates(aCursor.mCursorPosition).mLine];
			for (auto& g : line) 
				str.push_back(g.mChar);

			ImGui::SetClipboardText(str.c_str());
		}
	}
}


void Editor::Paste(){
	OpenGL::ScopedTimer timer("Editor::Paste");
	std::string text{ImGui::GetClipboardText()};

	if(text.size()<=0) return;

	UndoRecord uRecord;
	uRecord.mBefore=mState;

	for(int i=mState.mCursors.size()-1;i>=0;i--){
		size_t strLen=GetUTF8StringLength(text);
		Cursor& aCursor=mState.mCursors[i];


		if(HasSelection(aCursor)){
			uRecord.mOperations.push_back({ GetSelectedText(aCursor), aCursor.mSelectionStart, aCursor.mSelectionEnd, UndoOperationType::Delete });
			DeleteSelection(aCursor);
		}

		UndoOperation uAdded;
		uAdded.mStart=aCursor.mCursorPosition;
		uAdded.mType=UndoOperationType::Add;
		uAdded.mText=text;

		int lineNumberBeforeInsertion=aCursor.mCursorPosition.mLine;
		int nLines=InsertTextAt(aCursor.mCursorPosition, text.c_str());
		aCursor.mSelectionStart=aCursor.mSelectionEnd=aCursor.mCursorPosition;

		for (int j = i + 1; j < mState.mCursors.size(); j++) 
		{
			// GL_INFO("Loop Running");
			// if (mState.mCursors[j].mCursorPosition.mLine == mState.mCursors[i].mCursorPosition.mLine)
			// 	mState.mCursors[j].mCursorPosition.mColumn += strLen;

			// GL_INFO(nLines);
			if(nLines>0 && mState.mCursors[j].mCursorPosition.mLine >= lineNumberBeforeInsertion)
			{
				GL_INFO("Executing..");
				mState.mCursors[j].mCursorPosition.mLine+=nLines;
			}
		}

		uAdded.mEnd=aCursor.mCursorPosition;
		uRecord.mOperations.push_back(uAdded);
	}

	uRecord.mAfter=mState;
	mUndoManager.AddUndo(uRecord);

	DebouncedReparse();
}

bool Editor::HasSelection(const Cursor& aCursor) const
{
	return aCursor.mSelectionEnd != aCursor.mSelectionStart;
}

void Editor::CloseDebounceThread(){
	{
        std::lock_guard<std::mutex> lock(mutex_);
        terminate_ = true;
    }
    cv_.notify_one();
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    GL_WARN("Debounce Thread Closed!");
}


void Editor::DeleteSelection(Cursor& aCursor) {
	if(!HasSelection(aCursor)) return;

	if(aCursor.mSelectionStart>aCursor.mSelectionEnd) 
		std::swap(aCursor.mSelectionStart,aCursor.mSelectionEnd);

	DeleteRange(aCursor.mSelectionStart,aCursor.mSelectionEnd);
	aCursor.mCursorPosition=aCursor.mSelectionEnd=aCursor.mSelectionStart;

	GL_INFO("nLines:{}",mLines.size());
	GL_INFO("CursorPos:({},{})",aCursor.mCursorPosition.mLine,aCursor.mCursorPosition.mColumn);

}




void Editor::RemoveLine(int aStart, int aEnd)
{
	assert(aEnd >= aStart);
	assert(mLines.size() > (size_t)(aEnd - aStart));

	mLines.erase(mLines.begin() + aStart, mLines.begin() + aEnd);
}

void Editor::RemoveLine(int aIndex)
{
	assert(!mReadOnly);
	assert(mLines.size() > 1);

	mLines.erase(mLines.begin() + aIndex);
	assert(!mLines.empty());

}

void Editor::SetCursorPosition(const Coordinates& aPosition)
{
	Cursor& aState=GetCurrentCursor();
	if (aState.mCursorPosition != aPosition) {
		aState.mSelectionStart=aState.mSelectionEnd=aState.mCursorPosition = aPosition;
		// mCursorPositionChanged = true;
		EnsureCursorVisible();
	}
}

Editor::Line& Editor::InsertLine(int aIndex)
{
	auto& result = *mLines.insert(mLines.begin() + aIndex, Line());
	return result;
}


void Editor::InsertText(const std::string& aValue) { InsertText(aValue.c_str()); }

void Editor::InsertText(const char* aValue)
{
	if (aValue == nullptr)
		return;

	Cursor& aCursor=GetCurrentCursor();
	auto pos = aCursor.mCursorPosition;
	auto start = std::min(pos, aCursor.mSelectionStart);
	int totalLines = pos.mLine - start.mLine;

	totalLines += InsertTextAt(pos, aValue);

	SetCursorPosition(pos);
	UpdateSyntaxHighlighting(start.mLine - 1, totalLines + 2);
}

int Editor::InsertTextAt(Coordinates& aWhere, const char* aValue)
{
	assert(!mReadOnly);
	DisableSearch();

	int cindex = GetCharacterIndex(aWhere);
	int totalLines = 0;
	while (*aValue != '\0') {
		assert(!mLines.empty());

		if (*aValue == '\r') {
			// skip
			++aValue;
		} else if (*aValue == '\n') {
			if (cindex < (int)mLines[aWhere.mLine].size()) {
				auto& newLine = InsertLine(aWhere.mLine + 1);
				auto& line = mLines[aWhere.mLine];
				newLine.insert(newLine.begin(), line.begin() + cindex, line.end());
				line.erase(line.begin() + cindex, line.end());
			} else {
				InsertLine(aWhere.mLine + 1);
			}
			++aWhere.mLine;
			aWhere.mColumn = 0;
			cindex = 0;
			++totalLines;
			++aValue;
		} else {
			auto& line = mLines[aWhere.mLine];
			auto d = UTF8CharLength(*aValue);
			while (d-- > 0 && *aValue != '\0') line.insert(line.begin() + cindex++, Glyph(*aValue++, ColorSchemeIdx::Default));
			++aWhere.mColumn;
		}

		mTextChanged = true;
	}

	return totalLines;
}

Coordinates Editor::FindWordStart(const Coordinates& aFrom) const
{
	Coordinates at = aFrom;
	if (at.mLine >= (int)mLines.size())
		return at;

	auto& line = mLines[at.mLine];
	auto cindex = GetCharacterIndex(at);

	if (cindex >= (int)line.size())
		return at;

	// Check if cursor is at space if so move back
	char chr = line[cindex].mChar;
	if (cindex > 0 && (IsUTFSequence(chr) || (!isalnum(chr) || chr != '_'))) {
		if(IsUTFSequence(chr)){
			cindex-=UTF8CharLength(chr);
			chr=line[cindex].mChar;
		}
		else
			chr = line[--cindex].mChar;

		if (cindex == 0)
			return at;
		if (!isalnum(chr) && chr != '_')
			return at;
	}

	while (cindex > 0) {
		auto c = line[cindex].mChar;
		while (cindex > 0 && IsUTFSequence(c)) cindex--;
		if (isalnum(c) || c == '_')
			cindex--;
		else
			break;
	}
	return Coordinates(at.mLine, GetCharacterColumn(at.mLine, cindex + 1));
}

Coordinates Editor::FindWordEnd(const Coordinates& aFrom) const
{
	Coordinates at = aFrom;
	if (at.mLine >= (int)mLines.size())
		return at;

	auto& line = mLines[at.mLine];
	auto cindex = GetCharacterIndex(at);

	if (cindex >= (int)line.size())
		return at;

	while (cindex < (int)line.size() - 1) {
		auto c = line[cindex].mChar;
		if (IsUTFSequence(c))
			cindex += UTF8CharLength(c);
		else {
			if ((std::isalnum(c) || c == '_'))
				cindex++;
			else
				break;
		}
	}
	return Coordinates(aFrom.mLine, GetCharacterColumn(aFrom.mLine, cindex));
}

void Editor::Cut(){
	OpenGL::ScopedTimer timer("Editor::Cut");
	Copy();
	Backspace();
}



void Editor::SelectAll(){
	GL_INFO("SELECT ALL");
	Cursor& aCursor=GetCurrentCursor();
	aCursor.mSelectionEnd=aCursor.mCursorPosition=Coordinates(mLines.size()-1,GetLineMaxColumn(mLines.size()-1));
	aCursor.mSelectionStart=Coordinates(0,0);
	mSelectionMode=SelectionMode::Normal;
}


void Editor::Find(){
	GL_INFO("FIND");
	StatusBarManager::ShowInputPanel("Word:",[](const char* value){
		GL_INFO("CallbackFN:{}",value);
		// StatusBarManager::ShowNotification("Created:", file_path,StatusBarManager::NotificationType::Success);
	},nullptr,true,"Save");
}


void Editor::DisableSelection(){
	for(auto& aCursor:mState.mCursors){
		aCursor.mSelectionStart=aCursor.mSelectionEnd=aCursor.mCursorPosition;
	}
	mSelectionMode=SelectionMode::Normal;
}


Coordinates Editor::GetCoordinatesFromOffset(uint32_t aOffset) {
    uint32_t currentOffset = 0;
    uint32_t row = 0;
    
    for (const auto& line : mLines) {
        uint32_t lineLength = line.size() + 1; // +1 for newline
        if (currentOffset + lineLength > aOffset) {
            // Found the line containing ouraOffset 
            uint32_t column = aOffset - currentOffset;
            int i=0;
            while(i<line.size() && line[i++].mChar=='\t') column+=(mTabSize-1);
            return {(int)row, (int)column};
        }
        currentOffset += lineLength;
        row++;
    }
    
    return {(int)row, 0};
}


void Editor::EnsureCursorVisible()
{
	// if (!mWithinRender)
	// {
	// 	mScrollToCursor = true;
	// 	return;
	// }

	float scrollX = ImGui::GetScrollX();
	float scrollY = ImGui::GetScrollY();

	auto height = ImGui::GetWindowHeight();
	auto width = ImGui::GetWindowWidth()-ImGui::GetStyle().ScrollbarSize-mCharacterSize.x*10.0f;

	auto top = 1 + (int)ceil(scrollY / mLineHeight);
	auto bottom = (int)ceil((scrollY + height) / mLineHeight);

	auto pos = GetCurrentCursor().mCursorPosition;
	auto len = TextDistanceFromLineStart(pos);

	float leftOffset=mLineBarWidth+mPaddingLeft;

	float posX=len;

	if (pos.mLine < top)
		ImGui::SetScrollY(std::max(0.0f, (pos.mLine - 1.0f) * mLineHeight));
	if (pos.mLine > bottom - 4)
		ImGui::SetScrollY(std::max(0.0f, (pos.mLine + 4) * mLineHeight - height));

	if (posX > width+scrollX)
		ImGui::SetScrollX(std::max(0.0f, posX-width));

	if (posX < scrollX)
		ImGui::SetScrollX(std::max(0.0f,posX));	
}

float Editor::TextDistanceFromLineStart(const Coordinates& aFrom) const
{
	auto& line = mLines[aFrom.mLine];
	float distance = 0.0f;
	int colIndex = GetCharacterIndex(aFrom);
	for (size_t it = 0u; it < line.size() && it < colIndex;) {
		if (line[it].mChar == '\t') {
			distance += mTabSize*mCharacterSize.x;
			++it;
		} else {
			auto d = UTF8CharLength(line[it].mChar);
			char tempCString[7];
			int i = 0;
			for (; i < 6 && d-- > 0 && it < (int)line.size(); i++, it++) tempCString[i] = line[it].mChar;

			tempCString[i] = '\0';
			distance += mCharacterSize.x;
		}
	}

	return distance;
}









