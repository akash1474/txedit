#include "pch.h"
#include "FileNavigation.h"
#include "DataTypes.h"
#include "Coordinates.h"
#include "Timer.h"
#include "UndoManager.h"
#include "imgui_internal.h"
#include "TextEditor.h"

#include <cassert>
#include <cstdint>
#include <ctype.h>
#include <filesystem>
#include <regex>
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
#include "TokenType.h"
#include "Language.h"
#include "LanguageConfigManager.h"
#include "ThemeManager.h"

#undef min
#undef max

template <class InputIt1, class InputIt2, class BinaryPredicate>
bool equals(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, BinaryPredicate p)
{
	for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
		if (!p(*first1, *first2))
			return false;
	}

	return first1 == last1 && first2 == last2;
}


Editor::Editor()
{
	InitPallet();
	mLines.push_back(Line());
	workerThread_ = std::thread(&Editor::WorkerThread, this);
	mState.mCursors.emplace_back(Cursor());
}
Editor::~Editor() {
	CloseDebounceThread();
	// ts_query_delete(mQuery);
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
		isFileLoaded=true;
		mFileTypeName=FileNavigation::GetFileTypeNameFromFilePath(filepath);
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

	file.seekg(0, std::ios::end);
	size_t fileSize = file.tellg();
	file.seekg(0,std::ios::beg);
	
	std::string content(fileSize, ' ');
	file.read(&content[0], fileSize);

	//Trimming the spaces located at the last line at the end
	while(content.size()>0 && content[content.size()-1]==' ')
		content.pop_back();



	mFileTypeName=FileNavigation::GetFileTypeNameFromFilePath(filepath);
	// GL_INFO("mFileTypeName:{}",mFileTypeName);
	mHighlightType=TxEdit::GetHighlightType(mFilePath);
	// GL_INFO("HighLightType:{}",(int)mHighlightType);
	this->SetBuffer(content);
	isFileLoaded=true;
}

void Editor::SetBuffer(const std::string& aFileBuffer)
{
	GL_INFO("Editor::SetBuffer");
	mLines.clear();
	mLines.emplace_back(Line());

	for (auto chr : aFileBuffer)
	{
		if (chr == '\r')
			continue;

		if (chr == '\n'){
			mLines.emplace_back(Line());
		}
		else
		{
			mLines.back().emplace_back(chr, TxTokenType::TxDefault);
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
	// if(ext==".cpp" || ext==".h" || ext==".hpp" || ext==".c")
	// // if(ext==".py")
	// {
	// 	mIsSyntaxHighlightingSupportForFile=true;
	// 	ApplySyntaxHighlighting(aFileBuffer);
	// }
	// else 
	// 	mIsSyntaxHighlightingSupportForFile=false;

	mLanguageConfig=LanguageConfigManager::GetLanguageConfig(mHighlightType);
	mIsSyntaxHighlightingSupportForFile= mLanguageConfig ?  true :false;
	if(mIsSyntaxHighlightingSupportForFile)
		ApplySyntaxHighlighting(aFileBuffer);
}


void Editor::ClearEditor(){
	mFilePath.clear();
	isFileLoaded=false;
	mLines.clear();
	ResetState();
}


std::string Editor::GetFileTypeName(){
	return mFileTypeName;
}



bool Editor::Render(bool* aIsOpen,std::string& aUUID,bool& aIsTemp){
	ImGuiStyle& style=ImGui::GetStyle();
	float width=style.ScrollbarSize;
	style.ScrollbarSize=20.0f;

	bool isWindowFocused=false;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	if(ImGui::Begin(aUUID.c_str(),aIsOpen,this->mBufferModified ?  ImGuiWindowFlags_UnsavedDocument : ImGuiWindowFlags_None))
	{

		isWindowFocused=ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		if(isWindowFocused)
			ImGui::SetNextWindowFocus();

		// GL_INFO("Rendering:{}",aUUID);
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[(uint8_t)Fonts::MonoLisaRegular]);
		this->Draw();
		ImGui::PopFont();


		//Executes once when we focus on window
		if(ImGui::IsWindowDocked() && isWindowFocused){
			TabsManager::SetNewTabsDockSpaceId(ImGui::GetWindowDockID());
		}

		style.ScrollbarSize=width;
	}
	ImGui::End();  // #text_editor
	ImGui::PopStyleVar();

	return isWindowFocused;
}

ImVec2 Editor::GetLinePosition(const Coordinates& aCoords){
	return {ImGui::GetWindowPos().x-ImGui::GetScrollX(),ImGui::GetWindowPos().y+(aCoords.mLine*mLineHeight)-ImGui::GetScrollY()};
}



void HandleKeyBindings() {
    static const int timeoutMs = 150;  // Max time allowed between key presses
    static 	KeyBindingState keyBindingState;

    if (ImGui::IsKeyPressed(ImGuiKey_J, false)) { 
        keyBindingState.firstKeyPressed = true;
        keyBindingState.firstKeyTime = std::chrono::steady_clock::now();
    }

    if (keyBindingState.firstKeyPressed && ImGui::IsKeyPressed(ImGuiKey_K, false)) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - keyBindingState.firstKeyTime
        ).count();

        if (elapsed < timeoutMs) {
            // Combo detected: "jk"
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - keyBindingState.firstKeyTime
        	).count();
            GL_INFO("Combo 'jk' activated! --- {}ms",elapsed);
        }

        // Reset state
        keyBindingState.firstKeyPressed = false;
    }

    // Reset state if timeout expires
    if (keyBindingState.firstKeyPressed) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - keyBindingState.firstKeyTime
        ).count();
        if (elapsed > timeoutMs) {
            keyBindingState.firstKeyPressed = false;
        }
    }
}

bool Editor::Draw()
{
	HandleKeyBindings();
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

	for (int lineNo=start;lineNo < end;lineNo++) {

		Line& line=mLines[lineNo];
		ImVec2 linePosition = ImVec2(mEditorPosition.x - scrollX, mEditorPosition.y + (lineNo * mLineHeight) - scrollY);
		ImVec2 textScreenPos = ImVec2(linePosition.x + mLineBarWidth + mPaddingLeft, linePosition.y + (0.5 * mLineSpacing));

		// Render colorized text
		auto prevColor = line.empty() ? ThemeManager::GetTokenToColorMap()[(int)TxTokenType::TxDefault] : GetGlyphColor(line[0]);
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


		//Indentation line rendering
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

		if(mHighlight.isPresent)
		{
			RenderHighlight(mHighlight);
		}
	}





	//Cursors
	if(ImGui::IsWindowFocused())
	{
		for(const Cursor& cursor:mState.mCursors){
			ImVec2 linePos = GetLinePosition(cursor.mCursorPosition);
			float cx = TextDistanceFromLineStart(cursor.mCursorPosition);
			
			ImVec2 cstart(linePos.x+mLineBarWidth+mPaddingLeft + cx - 1.0f, linePos.y);
			ImVec2 cend(linePos.x+mLineBarWidth+mPaddingLeft + cx+1.0f, linePos.y + mLineHeight);
			mEditorWindow->DrawList->AddRectFilled(cstart,cend,ImColor(255, 255, 255, 255));
		}
	}






	//Line Number Background
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition}, {mEditorPosition.x + mLineBarWidth, mEditorSize.y}, mGruvboxPalletDark[(size_t)Pallet::Background]); // LineNo
	// Highlight Current Lin
	mEditorWindow->DrawList->AddRectFilled(
		{mEditorPosition.x,mEditorPosition.y+(aCursor.mCursorPosition.mLine*mLineHeight)-scrollY},
		{mEditorPosition.x+mLineBarWidth, mEditorPosition.y+(aCursor.mCursorPosition.mLine*mLineHeight)-scrollY + mLineHeight},
		mGruvboxPalletDark[(size_t)Pallet::Highlight]
	); // Code
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

	if(ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) && !mSuggestions.empty())
	{
		RenderSuggestionBox(mSuggestions,iCurrentSuggestion);
	}

	//Rendering Line Number
	for (int lineNo=start;lineNo<end;lineNo++) {
		float linePosY =mEditorPosition.y + (lineNo * mLineHeight) + 0.5f*mLineSpacing - scrollY;
		float linePosX=mEditorPosition.x + mLineBarPadding + (mLineBarMaxCountWidth-GetNumberWidth(lineNo+1))*mCharacterSize.x;

		// //Error Highlighting
		// if(mErrorMarkers[lineNo])
		// {
		// 	mEditorWindow->DrawList->AddRectFilled(
		// 		{mEditorPosition.x,mEditorPosition.y+(lineNo*mLineHeight)-scrollY},
		// 		{mEditorPosition.x+mLineBarWidth, mEditorPosition.y+(lineNo*mLineHeight)-scrollY + mLineHeight},
		// 		mPalette[(size_t)PaletteIndex::RED]
		// 	); // Code
		// }

		//Line Number
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

void Editor::ApplySuggestion(const std::string& aString,Cursor& aCursor)
{
	auto [start_idx,end_idx] = GetIndexOfWordAtCursor(aCursor.mCursorPosition);
	if(start_idx==end_idx) 
		return;

	int lineNumber=aCursor.mCursorPosition.mLine;
	Coordinates dStart(lineNumber,GetCharacterColumn(lineNumber, start_idx));
	Coordinates dEnd(lineNumber,GetCharacterColumn(lineNumber, end_idx));

	UndoRecord uRecord;
	uRecord.mBefore=mState;

	UndoOperation uRemoved;
	uRemoved.mType=UndoOperationType::Delete;
	uRemoved.mStart=dStart;
	uRemoved.mText=GetCurrentlyTypedWord();
	GL_INFO(uRemoved.mText);

	DeleteRange(dStart,dEnd);

	uRemoved.mEnd=dEnd; //after deletion
	uRecord.mOperations.push_back(uRemoved);



	UndoOperation uAdded;
	uAdded.mStart=dStart;
	uAdded.mType=UndoOperationType::Add;
	uAdded.mText=aString;

	InsertTextAt(dStart, aString.c_str());
	aCursor.mCursorPosition.mColumn=GetCharacterColumn(lineNumber, start_idx+aString.size());
	aCursor.mSelectionStart=aCursor.mSelectionEnd=aCursor.mCursorPosition;

	uAdded.mEnd=aCursor.mCursorPosition;
	uRecord.mOperations.push_back(uAdded);


	uRecord.mAfter=mState;

	mUndoManager.AddUndo(uRecord);
}

bool CustomAutoCompleteSuggestion(const std::string& aFileName,bool aIsSelected,int type=0)
{
	//Settingup custom component
	ImGuiWindow* window=ImGui::GetCurrentWindow();
	if(window->SkipItems) return false;

	const ImGuiID id=window->GetID(aFileName.c_str());
	const ImVec2 label_size=ImGui::CalcTextSize(aFileName.c_str());

	const ImVec2 itemSize(window->WorkRect.Max.x-window->WorkRect.Min.x,label_size.y+2.0f+(2*ImGui::GetStyle().FramePadding.y));
	ImVec2 pos=window->DC.CursorPos;


	const ImRect bb(pos,ImVec2(pos.x+itemSize.x,pos.y+itemSize.y));
	const float height=bb.Max.y-bb.Min.y;


	ImGui::ItemSize(bb,0.0f);
	if(!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);


    ImU32 col;
    if (aIsSelected && !hovered)
        col = IM_COL32(50, 50, 50, 255);
    else
        col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? IM_COL32(40, 40, 40, 255) : ImGuiCol_WindowBg);

	ImGui::RenderFrame(bb.Min, bb.Max, col, false, 0.0f);

	float img_size=itemSize.y;
	ImVec2 pmin(pos.x,pos.y+((height-img_size)*0.5f));
	ImVec2 pmax(pmin.x+img_size,pmin.y+img_size);
	window->DrawList->AddRectFilled(pmin, pmax, IM_COL32(40, 40, 40, 150));

	float sizex=ImGui::CalcTextSize("#").x;
	float texty=pos.y+(height-label_size.y)*0.5f;

	window->DrawList->AddText({pos.x+(img_size-sizex)*0.5f,texty},IM_COL32(255, 255, 255, 255),"t");

	const ImVec2 text_min(pos.x+40.0f,texty);
    const ImVec2 text_max(text_min.x + label_size.x, text_min.y + label_size.y);
	ImGui::RenderTextClipped(text_min,text_max,aFileName.c_str(),0,&label_size, ImGui::GetStyle().SelectableTextAlign, &bb);


	return pressed;
}

void Editor::RenderSuggestionBox(const std::vector<std::string>& aSuggestions, size_t& selectedIndex) 
{
    Cursor& aCursor=GetCurrentCursor();

    ImVec2 pos=GetLinePosition(aCursor.mCursorPosition);
    pos.x+=mLineBarWidth+mPaddingLeft+(aCursor.mCursorPosition.mColumn*mCharacterSize.x);
    pos.y+=mLineHeight;

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);

    size_t nSuggestions=aSuggestions.size();
    float suggestionSize=ImGui::GetIO().Fonts->Fonts[0]->FontSize+2.0f+(2*ImGui::GetStyle().FramePadding.y);
    float winSize=std::min((floor(ImGui::GetWindowSize().y-pos.y)/suggestionSize)*suggestionSize,nSuggestions>10 ? nSuggestions*10.0f : nSuggestions*suggestionSize);
    ImGui::SetNextWindowSize(ImVec2(500, winSize));
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize,15.0f);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[(int)Fonts::JetBrainsMonoNLRegular]);
    if(ImGui::Begin("##suggestionBox",NULL, ImGuiWindowFlags_NoFocusOnAppearing  | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
    {

	    for (size_t i = 0; i < aSuggestions.size(); ++i) 
	    {
	        bool isSelected = (selectedIndex == i);

	        if (isSelected) 
	        {
	            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
	        }

	       	bool isClicked=CustomAutoCompleteSuggestion(aSuggestions[i], isSelected);

	        if (isSelected)
	            ImGui::PopStyleColor();

	       	if(isClicked)
	       	{
	            selectedIndex = i;
	            for(auto& cursor:mState.mCursors)
	            	ApplySuggestion(aSuggestions[selectedIndex],cursor);

	            ClearSuggestions();
	            break;
	       	}
	    }

	    //Scrolling with the selectedIndex
	    if(ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))
	    {
		    float scrollY=ImGui::GetScrollY();
		    int minIdx=scrollY/suggestionSize;
		    int maxIdx=(scrollY+winSize)/suggestionSize;

		    if(ImGui::IsKeyPressed(ImGuiKey_UpArrow))
		    {
				if(selectedIndex==0)
					selectedIndex=mSuggestions.size()-1;
				else
					selectedIndex--;


		    }
		    else if(ImGui::IsKeyPressed(ImGuiKey_DownArrow))
		    {

				if(selectedIndex==mSuggestions.size()-1)
					selectedIndex=0;
				else
					selectedIndex++;

		    }

		    if (selectedIndex <= minIdx) 
		        ImGui::SetScrollY(selectedIndex * suggestionSize);
		    else if (selectedIndex >= maxIdx)
		        ImGui::SetScrollY((selectedIndex + 1) * suggestionSize - winSize);
	    }



    }
	ImGui::End();
    ImGui::PopFont();
    ImGui::PopStyleVar();
}

std::string Editor::GetCurrentlyTypedWord(){
	const Cursor& aCursor=GetCurrentCursor();
	auto [start_idx,end_idx] = GetIndexOfWordAtCursor(aCursor.mCursorPosition);
	if(start_idx==end_idx) 
		return "";

	std::string currentWord;
	for(int i=start_idx;i<end_idx;i++)
		currentWord+=(char)mLines[aCursor.mCursorPosition.mLine][i].mChar;

	return currentWord;
}


void Editor::RenderHighlight(const Highlight& aHighlight)
{
	ImVec2 linePos=GetLinePosition(mHighlight.aStart);
	float leftOffset=mLineBarWidth+mPaddingLeft;	
	linePos.x+=leftOffset;

	ImVec2 pos(linePos.x+(mHighlight.aStart.mColumn*mCharacterSize.x)-1.0f,linePos.y);

	float currentTime=(float)ImGui::GetTime();
	float alpha = 0.35f + 0.35f * sinf(currentTime * 2.0f);

	mEditorWindow->DrawList->AddRect(pos,{linePos.x+(mHighlight.aEnd.mColumn*mCharacterSize.x)+1.0f,linePos.y+mLineHeight}, IM_COL32(250, 189, 47, 255));
	float diff=currentTime - mHighlight.startTime;
	if(diff > 2.0f)
	{
		mHighlight.startTime=0.0f;
		mHighlight.isPresent=false;
	}
}

void Editor::CreateHighlight(int aLineNumber,int aStartIndex,int aEndIndex){
	mHighlight.aStart=Coordinates(aLineNumber,GetCharacterColumn(aLineNumber, aStartIndex));
	mHighlight.aEnd=Coordinates(aLineNumber,GetCharacterColumn(aLineNumber, aEndIndex));

	auto& aCursor = GetCurrentCursor();
	aCursor.mCursorPosition=aCursor.mSelectionEnd=mHighlight.aEnd;
	aCursor.mSelectionStart=mHighlight.aStart;

	mHighlight.isPresent=true;
	mHighlight.startTime=(float)ImGui::GetTime();
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

void Editor::WorkerThread() 
{
    while (true) 
    {
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
    ts_parser_set_language(parser,mLanguageConfig->tsLanguage());

    TSTree* tree = ts_parser_parse_string(parser, nullptr, sourceCode.c_str(), sourceCode.size());


	// Define a simple query to match syntax elements
    OpenGL::Timer timerx;
	uint32_t error_offset;
	TSQueryError error_type;
	
	if(!mLanguageConfig->pQuery)
		mLanguageConfig->pQuery= ts_query_new(mLanguageConfig->tsLanguage(), mLanguageConfig->pQueryString.c_str(), mLanguageConfig->pQueryString.size(), &error_offset, &error_type);

	// Check if the query was successfully createD
	if (!mLanguageConfig->pQuery) {
		GL_ERROR("Error creating query at offset {}, error type: {}", error_offset, error_type);
		ts_tree_delete(tree);
		ts_parser_delete(parser);
		ts_query_delete(mLanguageConfig->pQuery);
		return;
	}
    char buff[32];
    sprintf_s(buff,"%fms",timerx.ElapsedMillis());
    StatusBarManager::ShowNotification("Query Execution:", buff,StatusBarManager::NotificationType::Success);

	TSQueryCursor* cursor = ts_query_cursor_new();
	ts_query_cursor_exec(cursor, mLanguageConfig->pQuery, ts_tree_root_node(tree));
    // GL_INFO("Duration:{}",timerx.ElapsedMillis());
    // mErrorMarkers.clear();

    // mSuggestions.clear();
	static bool isFirst=true;
	// 5. Highlight matching nodes
	TSQueryMatch match;
	Trie::Node* aGlobalTokens=TabsManager::GetTrieRootNode();
	std::unordered_map<std::string, TxTokenType>& captureToToken=ThemeManager::GetCaptureToTokenMap();

	while (ts_query_cursor_next_match(cursor, &match)) {
		for (unsigned int i = 0; i < match.capture_count; ++i) {
			TSQueryCapture capture = match.captures[i];
			TSNode node = capture.node;

		    // Get the type of the current node
		    const char *nodeType = ts_node_type(node);
		    unsigned int length;
		    const char *captureName = ts_query_capture_name_for_id(mLanguageConfig->pQuery, capture.index, &length);
			// GL_INFO(match.captures[i].index);
			// GL_INFO(captureName);
		    // GL_INFO(nodeType);

		    TSPoint startPoint = ts_node_start_point(node);
		    TSPoint endPoint = ts_node_end_point(node);

		    if(isFirst){
			    if(std::string(captureName)=="type.indentifier" || std::string(captureName)=="function.namespace" || std::string(captureName)=="function"){
		    	    uint32_t startByte = ts_node_start_byte(node);
				    uint32_t endByte = ts_node_end_byte(node);
			    	Trie::Insert(aGlobalTokens,sourceCode.substr(startByte, endByte - startByte));
			    }
		    }
		    // {
		    // 	for(int i=startPoint.row;i<=endPoint.row;i++)
		    // 		mErrorMarkers[i]=true;
		    	
		    // 	continue;
		    // }

		    endPoint.column--;

		    TxTokenType colorIndex = captureToToken[captureName];
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
	isFirst=false;

    // Get the start and end positions of the node
    ts_tree_delete(tree);
	ts_parser_delete(parser);
	ts_query_cursor_delete(cursor);
	//Don't free mQuery as I am reusing it
}

void Editor::UpdateSyntaxHighlightingForRange(int aLineStart,int aLineEnd){
	if(!mIsSyntaxHighlightingSupportForFile || !mLanguageConfig->pQuery) return;
	OpenGL::ScopedTimer timer("UpdateSyntaxHighlightingForRange");

	Coordinates start(aLineStart,0);
	Coordinates end(aLineEnd,GetLineMaxColumn(aLineEnd));

	std::string sourceCode=GetText(start,end);
	if(sourceCode.empty())
		return;

    TSParser* parser= ts_parser_new();
    ts_parser_set_language(parser,mLanguageConfig->tsLanguage());

    TSTree* tree = ts_parser_parse_string(parser, nullptr, sourceCode.c_str(), sourceCode.size());

	TSQueryCursor* cursor = ts_query_cursor_new();
	ts_query_cursor_exec(cursor, mLanguageConfig->pQuery, ts_tree_root_node(tree));

	std::unordered_map<std::string, TxTokenType> captureToToken=ThemeManager::GetCaptureToTokenMap();

	for(size_t lineNo=aLineStart;lineNo<=aLineEnd;lineNo++){
		for(auto& glyph:mLines[lineNo])
			glyph.mColorIndex=TxTokenType::TxDefault;
	}

	TSQueryMatch match;
	while (ts_query_cursor_next_match(cursor, &match)) {
		for (unsigned int i = 0; i < match.capture_count; ++i) {
			TSQueryCapture capture = match.captures[i];
			TSNode node = capture.node;

		    // Get the type of the current node
		    const char *nodeType = ts_node_type(node);
		    unsigned int length;
		    const char *captureName = ts_query_capture_name_for_id(mLanguageConfig->pQuery, capture.index, &length);

		    int startLine = std::max(0, aLineStart);
		    TSPoint startPoint = ts_node_start_point(node);
		    startPoint.row+=startLine;
		    TSPoint endPoint = ts_node_end_point(node);
		    endPoint.row+=startLine;

		    TxTokenType colorIndex = captureToToken[captureName];
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

void Editor::UpdateSyntaxHighlighting(int aLineNo,int aLineCount)
{
	if(!mIsSyntaxHighlightingSupportForFile || !mLanguageConfig->pQuery) return;

	OpenGL::ScopedTimer timer("UpdateSyntaxHighlighting");
	std::string sourceCode=GetAboveLines(aLineNo,aLineCount);
	if(sourceCode.size() < 2) return;

    TSParser* parser= ts_parser_new();
    ts_parser_set_language(parser,mLanguageConfig->tsLanguage());

    TSTree* tree = ts_parser_parse_string(parser, nullptr, sourceCode.c_str(), sourceCode.size());

	TSQueryCursor* cursor = ts_query_cursor_new();
	ts_query_cursor_exec(cursor, mLanguageConfig->pQuery, ts_tree_root_node(tree));

	std::unordered_map<std::string, TxTokenType> captureToToken=ThemeManager::GetCaptureToTokenMap();

	TSQueryMatch match;
	while (ts_query_cursor_next_match(cursor, &match)) {
		for (unsigned int i = 0; i < match.capture_count; ++i) {
			TSQueryCapture capture = match.captures[i];
			TSNode node = capture.node;

		    // Get the type of the current node
		    const char *nodeType = ts_node_type(node);
		    unsigned int length;
		    const char *captureName = ts_query_capture_name_for_id(mLanguageConfig->pQuery, capture.index, &length);

		    int startLine = std::max(0, aLineNo-aLineCount);
		    TSPoint startPoint = ts_node_start_point(node);
		    startPoint.row+=startLine;
		    TSPoint endPoint = ts_node_end_point(node);
		    endPoint.row+=startLine;

		    TxTokenType colorIndex = captureToToken[captureName];
		    // if(startPoint.row != aLineNo) continue;
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


std::string Editor::GetAboveLines(int aLineNo,int aLineCount) {
    std::string result;

    int startLine = std::max(0, aLineNo - aLineCount);
    int endLine = std::min((int)mLines.size() - 1, aLineNo);

    for (int lineIndex = startLine; lineIndex <= endLine; ++lineIndex) {
        for (const Glyph& glyph : mLines[lineIndex]) {
            result += (char)glyph.mChar;
        }
        result += '\n';
    }

    return result;
}



ImU32 Editor::GetGlyphColor(const Glyph& aGlyph) const
{
	return ThemeManager::GetTokenToColorMap()[(int)aGlyph.mColorIndex];
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
void Editor::FindAllOccurancesOfWord(std::string aWord,size_t aStartLineIdx,size_t aEndLineIdx){

	OpenGL::ScopedTimer timer("Editor::FindAllOccurancesOfWord");
	mSearchState.mFoundPositions.clear();


	for(size_t i=aStartLineIdx;i<=aEndLineIdx;i++){
		auto& line=mLines[i];
		if(line.empty()) continue;

		size_t searchOffset=0;
		std::string clText=GetText(Coordinates(i,0),Coordinates(i,GetLineMaxColumn(i)));

		if(StatusBarManager::IsRegexSearch() || !StatusBarManager::IsCaseSensitiveSearch() || StatusBarManager::IsWholeWordSearch())
		{
		    std::regex_constants::syntax_option_type options = std::regex_constants::ECMAScript;
		    if (!StatusBarManager::IsCaseSensitiveSearch())
		        options |= std::regex_constants::icase; // Add case-insensitivity flag
		    
		    std::string regexPattern = StatusBarManager::IsWholeWordSearch() ?  "\\b" + aWord + "\\b" :  aWord;
		    std::regex pattern(regexPattern, options);
		    std::smatch match;
		    
		    std::string::const_iterator searchStart(clText.cbegin());
		    
		    // Search for matches using regex
		    while (std::regex_search(searchStart, clText.cend(), match, pattern)) {
		        // Store the start index of the match
		        size_t idx=match.position() + std::distance(clText.cbegin(), searchStart);
		        mSearchState.mFoundPositions.emplace_back(i,GetCharacterColumn(i,idx));
		        GL_TRACE("Line {} : Found '{}' at [{},{}] ",i+1,aWord,idx,idx+mSearchState.mWordLen);
		        
		        // Move the search start to after the last match
		        searchStart += match.position() + match.length();
		    }

		}
		else
		{
	        while ((searchOffset = clText.find(aWord, searchOffset)) != std::string::npos) {

	            size_t startIndex = searchOffset;
	            size_t endIndex = searchOffset + aWord.length() - 1;


	            GL_TRACE("Line {} : Found '{}' at [{},{}] ",i+1,aWord,startIndex,endIndex);

	            mSearchState.mFoundPositions.emplace_back(i,GetCharacterColumn(i,startIndex));
	            searchOffset = endIndex + 1;
	        }
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

void Editor::ExecuteSearch(const std::string& aWord)
{
	OpenGL::ScopedTimer timer("Editor::ExecuteSearch");
	if(aWord.empty())
	{
		DisableSearch();
		return;
	}
	PerformSearch(aWord);
	GotoNextMatch();
}


void Editor::GotoNextMatch()
{
	if(!mSearchState.IsValid()) 
		return;

	OpenGL::ScopedTimer timer("Editor::GotoNextMatch");
	const Coordinates& coord = mSearchState.mFoundPositions[mSearchState.mIdx];
	ScrollToLineNumber(coord.mLine);

	Cursor& aCursor=GetCurrentCursor();
	aCursor.mSelectionStart = aCursor.mSelectionEnd = coord;
	aCursor.mSelectionEnd.mColumn = coord.mColumn + mSearchState.mWordLen;
	aCursor.mCursorPosition = aCursor.mSelectionEnd;

	GL_INFO("[{}  {} {}]", aCursor.mSelectionStart.mColumn, aCursor.mSelectionEnd.mColumn, mSearchState.mWord.size());


	mSearchState.mIdx++;

	if (mSearchState.mIdx == mSearchState.mFoundPositions.size())
		mSearchState.mIdx = 0;
}


void Editor::GotoPreviousMatch()
{
	if(!mSearchState.IsValid()) 
		return;

	OpenGL::ScopedTimer timer("Editor::GotoPreviousMatch");
	if(mSearchState.mIdx == 0)
		mSearchState.mIdx=mSearchState.mFoundPositions.size()-1;
	else 
		mSearchState.mIdx--;

	const Coordinates& coord = mSearchState.mFoundPositions[mSearchState.mIdx];
	ScrollToLineNumber(coord.mLine);

	Cursor& aCursor=GetCurrentCursor();
	aCursor.mSelectionStart = aCursor.mSelectionEnd = coord;
	aCursor.mSelectionEnd.mColumn = coord.mColumn + mSearchState.mWordLen;
	aCursor.mCursorPosition = aCursor.mSelectionEnd;

	// GL_INFO("[{}  {} {}]", aCursor.mSelectionStart.mColumn, aCursor.mSelectionEnd.mColumn, mSearchState.mWord.size());
}

void Editor::HighlightAllMatches()
{
	if(!mSearchState.IsValid())
		return;

	OpenGL::ScopedTimer timer("Editor::HighlightAllMatches");
	ClearCursors();
	for(Coordinates& aMatchCoord:mSearchState.mFoundPositions)
	{
		Cursor nCursor;

		nCursor.mSelectionStart = nCursor.mSelectionEnd = aMatchCoord;
		nCursor.mSelectionEnd.mColumn = aMatchCoord.mColumn + mSearchState.mWordLen;
		nCursor.mCursorPosition = nCursor.mSelectionEnd;

		mState.mCursors.push_back(nCursor);
		mState.mCurrentCursorIdx++;
	}

	DisableSearch();
}

void Editor::PerformSearch(const std::string& aWord)
{
	OpenGL::ScopedTimer timer("Editor::PerformSearch");
	if(!(aWord==mSearchState.mWord && mSearchState.mIsGlobal))
	{
		mSearchState.SetSearchWord(aWord);
		mSearchState.mIsGlobal=true;
		mSearchState.mIdx=0;

		GL_INFO("WORD:{} Len:{}",aWord,mSearchState.mWordLen);

		FindAllOccurancesOfWord(aWord,0,mLines.size()-1);
	}


	auto& aCursor=GetCurrentCursor();
	// Find coord of match closest to current line
	int minDist=mLines.size();
	for(int i=0;i<mSearchState.mFoundPositions.size();i++)
	{
		int dist=std::abs(mSearchState.mFoundPositions[i].mLine-aCursor.mCursorPosition.mLine);
		if(dist<minDist)
		{
			mSearchState.mIdx=i;
			minDist=dist;
		}
		else if(dist > minDist)
			break; //As the match positions are sorted
	}
}


void Editor::HandleCtrlD(){
	//First time
	if(mSelectionMode!=SelectionMode::Word)
	{
		auto& aCursor=GetCurrentCursor();
		if(!HasSelection(aCursor))
		{
			SelectWordUnderCursor(aCursor);
			// if still no selection then search can't be performed
			if(!HasSelection(aCursor))
				return;
		}

		const std::string word=GetSelectedText(aCursor);
		PerformSearch(word);

		mSelectionMode=SelectionMode::Word;
		mSearchState.mIdx++;
	}
	else
	{
		if(!mSearchState.IsValid()) 
			return;

		// We have added cursors at all the matched positions
		if(mSearchState.mFoundPositions.size() == mState.mCursors.size())
			return;

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

	int start = std::min((int)mLines.size()-1,(int)floor(mEditorWindow->Scroll.y / mLineHeight));
	int lineCount = (mEditorWindow->Size.y) / mLineHeight;
	int end = std::min(start+lineCount,(int)mLines.size()-1);
	int currLine=GetCurrentCursor().mCursorPosition.mLine;

	GL_INFO("StartLine:{} EndLine:{}",start,end);


	if(aToLine > (start+1) && aToLine < (end-1)) 
		return;


	int diff=aToLine-start;
	diff= diff < 0 ? diff-4 : aToLine-end+4;
	mScrollAmount=diff*mLineHeight;
	GL_INFO("To:{},From:{}, Diff:{}, sAmount:{}",aToLine,currLine,diff,mScrollAmount);

	if(!aAnimate){
		mEditorWindow->Scroll.y += mScrollAmount;
		return;
	}

	//Handling quick change in nextl
	if((ImGui::GetTime()-mLastClick)<0.25f){
		mEditorWindow->Scroll.y += mScrollAmount;
	}else{
		mInitialScrollY=mEditorWindow->Scroll.y;
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
	assert(aStart <= aEnd);

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

std::string Editor::GetSelectedText(){
	auto& aCursor=GetCurrentCursor();
	return std::move(GetSelectedText(aCursor));
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
			if(aCursor.mSelectionStart > aCursor.mSelectionEnd)
				std::swap(aCursor.mSelectionStart,aCursor.mSelectionEnd);
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
			while (d-- > 0 && *aValue != '\0') line.insert(line.begin() + cindex++, Glyph(*aValue++, TxTokenType::TxDefault));
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









