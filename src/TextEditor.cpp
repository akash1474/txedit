#include "Coordinates.h"
#include "FontAwesome6.h"
#include "Lexer.h"
#include "Timer.h"
#include "UndoManager.h"
#include "imgui_internal.h"
#include "pch.h"
#include "TextEditor.h"

#include <cassert>
#include <cstdint>
#include <ctype.h>
#include <filesystem>
#include <ios>
#include <iterator>
#include <stack>
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
#include "StatusBarManager.h"
#include "TabsManager.h"

#include "tree_sitter/api.h"
#include "tree_sitter/parser.h"


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
	mRegexList.clear();

	// for (auto& r : mLanguageDefinition.mTokenRegexStrings)
	// 	mRegexList.push_back(std::make_pair(std::regex(r.first, std::regex_constants::optimize), r.second));

	Colorize();
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
	// #undef IM_TABSIZE
	// #define IM_TABSIZE mTabSize
}
Editor::~Editor() {}

void Editor::ResetState(){
	mSearchState.reset();
	mCursors.clear();
	mState.mCursorPosition.mColumn=0;
	mState.mCursorPosition.mLine=0;
	mState.mSelectionEnd =mState.mSelectionStart=mState.mCursorPosition;
	mSelectionMode=SelectionMode::Normal;
	mBracketsCoordinates.hasMatched=false;
}


void Editor::LoadFile(const char* filepath){
	if(!std::filesystem::exists(filepath)){
		StatusBarManager::ShowNotification("Invalid Path",filepath,StatusBarManager::NotificationType::Error);
		return;
	}
	GL_INFO("Editor::LoadFile - {}",filepath);
	mFileContents.clear();
	this->ResetState();
	size_t size{0};
	std::ifstream t(filepath);
	if(t.good()) mFilePath=filepath;
	fileType=GetFileType();

	t.seekg(0, std::ios::end);
	size = t.tellg();
	mFileContents.resize(size, ' ');
	t.seekg(0);
	t.read(&mFileContents[0], size);
	this->SetBuffer(mFileContents);
	isFileLoaded=true;
	reCalculateBounds=true;
	// this->InitTreeSitter();
	// lex.SetData(file_data);
	// lex.Tokenize();
}

void Editor::ClearEditor(){
	mFilePath.clear();
	fileType.clear();
	mLines.clear();
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



void Editor::Render(){
	ImGuiStyle& style=ImGui::GetStyle();
	float width=style.ScrollbarSize;
	style.ScrollbarSize=20.0f;



	//Hiding Tab Bar
	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoDockingOverMe;
	static int winFlags=ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingOverMe;

	ImGui::SetNextWindowClass(&window_class);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("#editor_container",0,winFlags | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
		ImGui::PopStyleVar();




	    ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
	    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

		static bool isFirst=true;
		if(isFirst){
			isFirst=false;
			ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
			ImGui::DockBuilderAddNode(dockspace_id,dockspace_flags | ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetWindowSize());

			auto doc_id_top = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.0f, nullptr, &dockspace_id);
			ImGui::DockBuilderSetNodeSize(doc_id_top, {ImGui::GetWindowWidth(),40.0f});
			ImGui::DockBuilderDockWindow("#tabs_area", doc_id_top);
			ImGui::DockBuilderDockWindow("#text_editor", dockspace_id);

			ImGui::DockBuilderFinish(dockspace_id);
		}


		TabsManager::Render(window_class,winFlags);



		ImGui::SetNextWindowClass(&window_class);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg,mGruvboxPalletDark[(size_t)Pallet::Background]);
		ImGui::SetNextWindowContentSize(ImVec2(ImGui::GetContentRegionMax().x + 1500.0f, 0));
		ImGui::Begin("#text_editor", 0, ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_HorizontalScrollbar|winFlags);
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[(uint8_t)Fonts::MonoLisaRegular]);
			this->Draw();
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImGuiID hover_id = ImGui::GetHoveredID();
			bool scrollbarHovered = hover_id && (hover_id == ImGui::GetWindowScrollbarID(window, ImGuiAxis_X) || hover_id == ImGui::GetWindowScrollbarID(window, ImGuiAxis_Y));
			if(scrollbarHovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
			ImGui::PopFont();
			style.ScrollbarSize=width;
		ImGui::End();  // #text_editor



	ImGui::End();

}

ImVec2 Editor::GetLinePosition(const Coordinates& aCoords){
	return {ImGui::GetWindowPos().x-ImGui::GetScrollX(),ImGui::GetWindowPos().y+(aCoords.mLine*mLineHeight)-ImGui::GetScrollY()};
}



bool Editor::Draw()
{
	// OpenGL::ScopedTimer timer("Editor::Draw");
	if(!isFileLoaded) return false;
	
	static bool isInit = false;
	if (!isInit) {
		mEditorWindow = ImGui::GetCurrentWindow();
		mEditorPosition = mEditorWindow->Pos;

		mCharacterSize = ImVec2(ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr));

		mLineBarMaxCountWidth=GetNumberWidth(mLines.size());
		mLineBarWidth=ImGui::CalcTextSize(std::to_string(mLines.size()).c_str()).x + 2 * mLineBarPadding;

		mLinePosition = ImVec2({mEditorPosition.x + mLineBarWidth + mPaddingLeft, mEditorPosition.y});
		mLineHeight = mLineSpacing + mCharacterSize.y;

		mSelectionMode = SelectionMode::Normal;

		/* Update palette with the current alpha from style */
		for (int i = 0; i < (int)PaletteIndex::Max; ++i) {
			auto color = ImGui::ColorConvertU32ToFloat4(mPaletteBase[i]);
			color.w *= 1.0f;
			mPalette[i] = ImGui::ColorConvertFloat4ToU32(color);
		}

		GL_WARN("LINE HEIGHT:{}", mLineHeight);
		isInit = true;
	}


	if (mEditorPosition.x != mEditorWindow->Pos.x || mEditorPosition.y != mEditorWindow->Pos.y) reCalculateBounds = true;
	if (reCalculateBounds) UpdateBounds();


	const ImGuiIO& io = ImGui::GetIO();
	const ImGuiID id = ImGui::GetID("##Editor");


	ImGui::ItemSize(mEditorBounds, 0.0f);
	if (!ImGui::ItemAdd(mEditorBounds, id)) return false;

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

	// // BackGrounds
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

	if (ImGui::IsWindowFocused() && io.MouseWheel != 0.0f) {
		GL_INFO("SCROLLX:{} SCROLLY:{}",ImGui::GetScrollX(),ImGui::GetScrollY());
		if(mSearchState.isValid() && !mSearchState.mIsGlobal)SearchWordInCurrentVisibleBuffer();
	}

	auto scrollX = ImGui::GetScrollX();
	auto scrollY = ImGui::GetScrollY();

	mMinLineVisible = (int)floor(scrollY / mLineHeight);
	mLinePosition.y = GetLinePosition(mState.mCursorPosition).y;
	mLinePosition.x = mEditorPosition.x + mLineBarWidth + mPaddingLeft-ImGui::GetScrollX();



	//Highlight Selections
	if (HasSelection()) {
		Coordinates selectionStart=mState.mSelectionStart;
		Coordinates selectionEnd=mState.mSelectionEnd;

		if(selectionStart > selectionEnd)
			std::swap(selectionStart,selectionEnd);

		if(selectionStart.mLine==selectionEnd.mLine){

			if(mCursors.empty()){
				ImVec2 start(GetSelectionPosFromCoords(selectionStart), mLinePosition.y);
				ImVec2 end(GetSelectionPosFromCoords(selectionEnd), mLinePosition.y + mLineHeight);

				mEditorWindow->DrawList->AddRectFilled(start, end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);
			}
			else{
				int start = std::min(int(mMinLineVisible),(int)mLines.size());
				int lineCount = (mEditorWindow->Size.y) / mLineHeight;
				int end = std::min(start + lineCount + 1,(int)mLines.size());
				for(const auto& cursor:mCursors){
					if(cursor.mSelectionStart.mLine>=start && cursor.mSelectionStart.mLine < end){
						int posY=GetLinePosition(cursor.mCursorPosition).y;
						ImVec2 start(GetSelectionPosFromCoords(cursor.mSelectionStart), posY);
						ImVec2 end(GetSelectionPosFromCoords(cursor.mSelectionEnd), posY + mLineHeight);

						mEditorWindow->DrawList->AddRectFilled(start, end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);
					}
				}
			}

		}else if ((selectionStart.mLine+1)==selectionEnd.mLine){ //Rendering selection two lines

			float prevLinePositonY=mLinePosition.y;
			if(mState.mCursorDirectionChanged){
				mLinePosition.y = GetLinePosition(selectionEnd).y;
			}

			ImVec2 start(GetSelectionPosFromCoords(selectionStart), mLinePosition.y-mLineHeight);
			ImVec2 end(mLinePosition.x+GetLineMaxColumn(selectionStart.mLine)*mCharacterSize.x+mCharacterSize.x, mLinePosition.y);

			mEditorWindow->DrawList->AddRectFilled(start, end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);


			start={mLinePosition.x, mLinePosition.y};
			end={GetSelectionPosFromCoords(selectionEnd), mLinePosition.y + mLineHeight};

			mEditorWindow->DrawList->AddRectFilled(start, end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);


			if(mState.mCursorDirectionChanged){
				mLinePosition.y = prevLinePositonY;
			}
		}else if((selectionEnd.mLine-selectionStart.mLine) > 1){ // Selection multiple lines
			int start=selectionStart.mLine+1;
			int end=selectionEnd.mLine;
			int diff=end-start;


			float prevLinePositonY=mLinePosition.y;
			if(mState.mCursorDirectionChanged){
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

			if(mState.mCursorDirectionChanged){
				mLinePosition.y = prevLinePositonY;
			}
		}
	}


	int start = (int)floor(scrollY / mLineHeight);
	int lineCount = (mEditorWindow->Size.y) / mLineHeight;
	int end = std::min(start+lineCount+1,(int)mLines.size());

	// GL_INFO("Lines:{}-{}",start,end);
	if (start >= mLines.size()) {
		ImGui::SetScrollY(0);
		start = 0;
		end = std::min(start+lineCount+1,(int)mLines.size());
	}

	int lineNo = 0;
	int i_prev=0;
	mLineBuffer.clear();
	auto drawList = ImGui::GetWindowDrawList();
	float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

	//Rendering Lines and Vertical Indentation Lines
	while (start != end) {

		ImVec2 linePosition = ImVec2(mEditorPosition.x - scrollX, mEditorPosition.y + (start * mLineHeight) - scrollY);
		ImVec2 textScreenPos = ImVec2(linePosition.x + mLineBarWidth + mPaddingLeft, linePosition.y + (0.5 * mLineSpacing));

		// Old Rendering
		// float linePosY = mEditorPosition.y + (lineNo * mLineHeight) +(0.5*mLineSpacing);
		// mEditorWindow->DrawList->AddText({mLinePosition.x, linePosY}, mGruvboxPalletDark[(size_t)Pallet::Text], mLines[start].c_str());

		Line& line=mLines[start];
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


		if(mLines[start].empty()){
			int i=i_prev;
			while(i>-1){
				ImVec2 indentStart{mLinePosition.x+(i*mTabSize*mCharacterSize.x), linePosition.y};
				mEditorWindow->DrawList->AddLine(indentStart, {indentStart.x,indentStart.y+mLineHeight}, mGruvboxPalletDark[(size_t)Pallet::Indentation]);
				i--;
			}
		}else{
			int i=0;
			while(mLines[start].size() > i && mLines[start][i].mChar=='\t'){
				ImVec2 indentStart{mLinePosition.x+(i*mTabSize*mCharacterSize.x), linePosition.y};
				mEditorWindow->DrawList->AddLine(indentStart, {indentStart.x,indentStart.y+mLineHeight}, mGruvboxPalletDark[(size_t)Pallet::Indentation]);
				i++;
			}
			i_prev=--i;
		}


		if (ImGui::IsWindowFocused() && start==mState.mCursorPosition.mLine) {
			float cx = TextDistanceFromLineStart(mState.mCursorPosition);

			ImVec2 cstart(textScreenPos.x + cx - 2.0f, linePosition.y);
			ImVec2 cend(textScreenPos.x + cx, linePosition.y + mLineHeight);
			drawList->AddRectFilled(cstart, cend, ImColor(255, 255, 255, 255));
		}


		start++;
		lineNo++;
	}




	// // Cursor
	// if(mCursors.empty()){
	// 	float cx = TextDistanceFromLineStart(mState.mCursorPosition);

	// 	ImVec2 cstart(textScreenPos.x + cx - 2.0f, linePosition.y);
	// 	ImVec2 cend(textScreenPos.x + cx, linePosition.y + mLineHeight);
	// 	drawList->AddRectFilled(cstart, cend, ImColor(255, 255, 255, 255));
	// 	// ImVec2 cursorPos=TextDistanceFromLineStart()
	// 	// ImVec2 cursorPosition(mLinePosition.x - 1.0f + (mState.mCursorPosition.mColumn * mCharacterSize.x), mEditorPosition.y+(mState.mCursorPosition.mLine*mLineHeight)-scrollY);
	// 	// mEditorWindow->DrawList->AddRectFilled(cursorPosition, {cursorPosition.x + 2.0f, cursorPosition.y + mLineHeight},ImColor(255, 255, 255, 255));
	// }else{
	// 	assert(false && "Multiple Cursor Feature not supported");
	// 	// for(const EditorState& cursor:mCursors){
	// 	// 	int lineY = (mEditorPosition.y + (cursor.mCursorPosition.mLine-floor(mMinLineVisible)) * mLineHeight);
	// 	// 	ImVec2 cursorPosition(mLinePosition.x - 1.0f + (cursor.mCursorPosition.mColumn * mCharacterSize.x), lineY);
	// 	// 	mEditorWindow->DrawList->AddRectFilled(cursorPosition, {cursorPosition.x + 2.0f, cursorPosition.y + mLineHeight},ImColor(255, 255, 255, 255));
	// 	// }

	// }




	start = (int)floor(scrollY / mLineHeight);
	lineCount = (mEditorWindow->Size.y) / mLineHeight;
	end = std::min(start+lineCount+1,(int)mLines.size());



	bool isTrue=mSearchState.isValid() && mSelectionMode!=SelectionMode::Line;
	if(isTrue)
		HighlightCurrentWordInBuffer();

	//Line Number Background
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition}, {mEditorPosition.x + mLineBarWidth, mEditorSize.y}, mGruvboxPalletDark[(size_t)Pallet::Background]); // LineNo
	// Highlight Current Lin
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition.x,mEditorPosition.y+(mState.mCursorPosition.mLine*mLineHeight)-scrollY},{mEditorPosition.x+mLineBarWidth, mEditorPosition.y+(mState.mCursorPosition.mLine*mLineHeight)-scrollY + mLineHeight},mGruvboxPalletDark[(size_t)Pallet::Highlight]); // Code
	mLineHeight = mLineSpacing + mCharacterSize.y;



	//Rendering for selected word
	if(isTrue){
		bool isNormalMode=mSelectionMode==SelectionMode::Normal;
		for(const auto& coord:mSearchState.mFoundPositions){
			float linePosY = mEditorPosition.y+(mState.mCursorPosition.mLine*mLineHeight)-scrollY;
			ImVec2 start{mEditorPosition.x,linePosY};
			ImVec2 end{mEditorPosition.x+4.0f,linePosY+(isNormalMode ? 0 : mLineHeight)};

			mEditorWindow->DrawList->AddRectFilled(start,end, mGruvboxPalletDark[(size_t)Pallet::Text]);
		}
	}


	//Highlighting Brackets
	// if(mBracketsCoordinates.hasMatched){
	// 	for(Coordinates& coord:mBracketsCoordinates.coords){
	// 		if (coord.mColumn >= mLines[coord.mLine].size()) continue;
	// 		int column=GetCharacterColumn(coord.mLine,coord.mColumn);

	// 		float linePosY = (mEditorPosition.y  + (coord.mLine-floor(mMinLineVisible)) * mLineHeight);
	// 		int tabs=GetTabCountsUptoCursor(coord)*(mTabSize-1);

	// 		ImVec2 start{mLinePosition.x+column*mCharacterSize.x,linePosY};
	// 		mEditorWindow->DrawList->AddRect(start,{start.x+mCharacterSize.x+1,start.y+mLineHeight}, mGruvboxPalletDark[(size_t)Pallet::HighlightOne]);
	// 	}
	// }


	//Horizonal scroll Shadow
	if(ImGui::GetScrollX()>0.0f){
		ImVec2 pos_start{mEditorPosition.x+mLineBarWidth,0.0f};
		mEditorWindow->DrawList->AddRectFilledMultiColor(pos_start,{pos_start.x+10.0f,mEditorWindow->Size.y}, ImColor(19,21,21,130),ImColor(19,21,21,0),ImColor(19,21,21,0),ImColor(19,21,21,130));
	}


	//Rendering Line Number
	lineNo = 0;
	while (start != end) {
		float linePosY =mEditorPosition.y + (start * mLineHeight) + 0.5f*mLineSpacing - scrollY;
		float linePosX=mEditorPosition.x + mLineBarPadding + (mLineBarMaxCountWidth-GetNumberWidth(start+1))*mCharacterSize.x;

		mEditorWindow->DrawList->AddText({linePosX, linePosY}, (start==mState.mCursorPosition.mLine) ? mGruvboxPalletDark[(size_t)Pallet::Text] : mGruvboxPalletDark[(size_t)Pallet::Comment], std::to_string(start + 1).c_str());

		start++;
		lineNo++;
	}


	HandleKeyboardInputs();
	HandleMouseInputs();

	return true;
}

// void Editor::CalculateBracketMatch(){
// 	const auto pos=GetMatchingBracketsCoordinates();
// 	if(mBracketsCoordinates.hasMatched) 
// 		mBracketsCoordinates.coords=pos;
// }



void Editor::InitTreeSitter(){
	OpenGL::ScopedTimer timer("TreeSitter Parsing");
	mParser = ts_parser_new();
	ts_parser_set_language(mParser, tree_sitter_cpp());

	mTree = ts_parser_parse_string(mParser, nullptr, mFileContents.data(), mFileContents.size());

	std::string query_string = R"((type_identifier) @type 
    							(comment) @comment 
    							(string_literal) @string 
    							(primitive_type) @keyword 
    							(function_declarator declarator:(_) @function) 
    							(namespace_identifier) @namespace)";

	// Define a simple query to match syntax elements
	uint32_t error_offset;
	TSQueryError error_type;
	mQuery = ts_query_new(tree_sitter_cpp(), query_string.c_str(), query_string.size(), &error_offset, &error_type);

	// Check if the query was successfully created
	if (!mQuery) {
		GL_ERROR("Error creating query at offset {}, error type: {}", error_offset, error_type);
		ts_tree_delete(mTree);
		ts_parser_delete(mParser);
		return;
	}

	mCursor = ts_query_cursor_new();
	ts_query_cursor_exec(mCursor, mQuery, ts_tree_root_node(mTree));


	// 4. Styles for each syntax type
	std::unordered_map<std::string, int> syntax_styles = {{"function", 1}, // Green for functions
	                                                      {"type", 2},    // Blue for types
	                                                      {"comment", 3}, // Grey for comments
	                                                      {"string", 4},  // Yellow for strings
	                                                      {"keyword", 5}, // Red for keywords
	                                                      {"namespace", 6}};

	GL_INFO("File Parsed");

	struct Highlight{
		size_t startByte;
		size_t endByte;
		std::string type;
		Highlight(size_t s,size_t e,std::string t):startByte(s),endByte(e),type(t){}
	};
	std::vector<Highlight> highlights;

	TSQueryMatch match;
	unsigned long cursor_position = 0;
	while (ts_query_cursor_next_match(mCursor, &match)) {
		for (unsigned int i = 0; i < match.capture_count; ++i) {
			TSQueryCapture capture = match.captures[i];
			TSPoint point=ts_node_start_point(capture.node);
			// if(point.row< start || point.row > end) continue;
			uint32_t len;
			std::string capture_name = ts_query_capture_name_for_id(mQuery, capture.index, &len);
			TSNode node = capture.node;

			// Extract the start and end byte positions of the node
			uint32_t start_byte = ts_node_start_byte(node);
			uint32_t end_byte = ts_node_end_byte(node);
			std::string text=mFileContents.substr(cursor_position,start_byte-cursor_position);

			highlights.emplace_back(start_byte,end_byte,capture_name);
			// Update cursor position
			cursor_position = end_byte;
		}
	}

	// for(Highlight& highlight:highlights){
	// 	GL_INFO("{}:{}",highlight.type,mFileContents.substr(highlight.startByte,highlight.endByte-highlight.startByte));
	// }
}


// std::array<Coordinates,2> Editor::GetMatchingBracketsCoordinates(){
// 	OpenGL::ScopedTimer timer("BracketMatching");
// 	std::array<Coordinates,2> coords;
// 	bool hasBracketsMatched=true;
// 	mBracketsCoordinates.hasMatched=false;
// 	GL_INFO("CHAR:{}",mLines[mState.mCursorPosition.mLine][GetCharacterIndex(mState.mCursorPosition)]);

// 	// const Coordinates cStart=FindMatchingBracket(mState.mCursorPosition, false);
// 	const Coordinates cStart=FindStartBracket(mState.mCursorPosition);
// 	if(cStart.mLine==INT_MAX) hasBracketsMatched=false;
// 	if(!hasBracketsMatched) return coords;
// 	coords[0]=cStart;
// 	GL_INFO("BracketMatch Start:[{},{}]",coords[0].mLine,coords[0].mColumn);

// 	// const Coordinates cEnd=FindMatchingBracket(mState.mCursorPosition, true);
// 	const Coordinates cEnd=FindEndBracket(mState.mCursorPosition);
// 	if(cStart.mLine==INT_MAX) hasBracketsMatched=false;
// 	if(cEnd.mLine==INT_MAX) hasBracketsMatched=false;

// 	if(hasBracketsMatched){
// 		coords[1]=cEnd;
// 		mBracketsCoordinates.hasMatched=true;
// 		GL_INFO("BracketMatch End:[{},{}]",coords[1].mLine,coords[1].mColumn);
// 	}

// 	return coords;
// }



int IsBracket(const char x){
	if(x=='(' || x=='{' || x=='[') return -1;
	else if(x==')' || x=='}' || x==']') return 1;

	return 0;
}


// Coordinates Editor::FindStartBracket(const Coordinates& coords){
// 	Coordinates coord{INT_MAX,INT_MAX};

// 	int cLine=coords.mLine;
// 	int cColumn=std::max(0,(int)GetCharacterIndex(coords)-1);

// 	bool isFound=false;
// 	int ignore=0;
// 	char x=-1;

// 	for(;cLine>=0;cLine--)
// 	{
// 		const auto& line=mLines[cLine];
// 		if(line.empty()){
// 			if(cLine>0 && !mLines[cLine - 1].empty()) 
// 				cColumn=mLines[cLine - 1].size() - 1;
// 			else cColumn=0;
// 			continue;
// 		}

// 		for(;cColumn>=0;cColumn--){
// 			if(line[cColumn]==x){ x=-1; continue; }
// 			if(line[cColumn]=='\'' || line[cColumn]=='"'){
// 				x=line[cColumn]; 
// 				continue; 
// 			}
// 			if(x!=-1) continue;

// 			switch(line[cColumn]) {
// 				case ')': ignore++;break;
// 				case ']': ignore++;break;
// 				case '}': ignore++;break;
// 				case '(':{if(!ignore){isFound=true;} if(ignore>0) ignore--; break; }
// 				case '[':{if(!ignore){isFound=true;} if(ignore>0) ignore--; break; }
// 				case '{':{if(!ignore){isFound=true;} if(ignore>0) ignore--; break; }
// 			}

// 			if(isFound)	{
// 				coord.mLine=cLine;
// 				coord.mColumn=cColumn;
// 				return coord;
// 			}
// 		}

// 		if(cLine>0 && !mLines[cLine - 1].empty()) 
// 			cColumn=mLines[cLine - 1].size() - 1;
// 		else cColumn=0;
// 	}
// 	return coord;
// }


// Coordinates Editor::FindEndBracket(const Coordinates& coords){
// 	Coordinates coord{INT_MAX,INT_MAX};

// 	int cLine=coords.mLine;
// 	int cColumn=(int)GetCharacterIndex(coords);

// 	int ignore=0;
// 	bool isFound=false;
// 	char stringQuotes=-1;
// 	for(;cLine < mLines.size();cLine++)
// 	{
// 		const auto& line=mLines[cLine];
// 		if(line.empty()) continue;

// 		for(;cColumn < line.size();cColumn++)
// 		{
// 			//Ignoring the brackets inside quotes
// 			if(line[cColumn]==stringQuotes){ stringQuotes=-1; continue; }
// 			if(line[cColumn]=='\'' || line[cColumn]=='"'){
// 				stringQuotes=line[cColumn];
// 				continue; 
// 			}
// 			if(stringQuotes!=-1) continue;


// 			switch(line[cColumn]){
// 				case '(': ignore++; break;
// 				case '[': ignore++; break;
// 				case '{': ignore++; break;
// 				case ')':{if(!ignore) isFound=true; if(ignore>0) ignore--; break; }
// 				case ']':{if(!ignore) isFound=true; if(ignore>0) ignore--; break; }
// 				case '}':{if(!ignore) isFound=true; if(ignore>0) ignore--; break; }
// 			}

// 			if(isFound){
// 				coord.mLine=cLine;
// 				coord.mColumn=cColumn;
// 				return coord;
// 			}
// 		}
// 		cColumn=0;
// 	}
// 	return coord;
// }

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


// Function to update Glyphs using Tree-sitter
void Editor::UpdateSyntaxHighlighting(const std::string &sourceCode)
{
	OpenGL::ScopedTimer timer("UpdateSyntaxHighlighting");
	// Initialize Tree-sitter parser
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser,tree_sitter_cpp());
    TSTree *tree = ts_parser_parse_string(parser, nullptr, sourceCode.c_str(), sourceCode.size());

	static std::string query_string = R"(
	[ "if" 
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
	] @keyword

	(auto) @keyword

	; anything.[captured]
	(field_expression
		field:(field_identifier) @assignment)

	;array access captured[]
	(subscript_expression
		argument:(identifier) @assignment)

	(number_literal) @number

	(type_identifier) @type.indentifier
	(comment) @comment 
	(string_literal) @string 
	(raw_string_literal) @string
	(primitive_type) @keyword
	(function_declarator declarator:(_) @function) 
	;(namespace_identifier) @namespace

	(preproc_include (string_literal) @string)
	(preproc_include (system_lib_string) @string)
	(preproc_include) @keyword
    (preproc_def) @keyword
    (preproc_ifdef) @keyword
    (preproc_ifdef
    	(identifier) @scope_resolution) 
    (preproc_else) @keyword

	"::" @scope_resolution
	(assignment_expression
		left:(identifier) @assignment)
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

	[
	  "co_await"
	  "co_yield"
	  "co_return"
	] @keyword

	[
	  "public"
	  "private"
	  "protected"
	  "final"
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
	TSQuery* query = ts_query_new(tree_sitter_cpp(), query_string.c_str(), query_string.size(), &error_offset, &error_type);

	// Check if the query was successfully created
	if (!query) {
		GL_ERROR("Error creating query at offset {}, error type: {}", error_offset, error_type);
		ts_tree_delete(tree);
		ts_parser_delete(parser);
		return;
	}

	TSQueryCursor* cursor = ts_query_cursor_new();
	ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));
    // GL_INFO("Duration:{}",timerx.ElapsedMillis());
    char buff[32];
    sprintf_s(buff,"%fms",timerx.ElapsedMillis());
    StatusBarManager::ShowNotification("Query Execution:", buff,StatusBarManager::NotificationType::Success);

	// 5. Highlight matching nodes
	TSQueryMatch match;
	while (ts_query_cursor_next_match(cursor, &match)) {
		for (unsigned int i = 0; i < match.capture_count; ++i) {
			TSQueryCapture capture = match.captures[i];
			TSNode node = capture.node;

		    // Get the type of the current node
		    const char *nodeType = ts_node_type(node);
		    unsigned int length;
		    const char *captureName = ts_query_capture_name_for_id(query, capture.index, &length);
			// GL_INFO(match.captures[i].index);
			// GL_INFO(captureName);
		    // GL_INFO(nodeType);

		    TSPoint startPoint = ts_node_start_point(node);
		    TSPoint endPoint = ts_node_end_point(node);
		    endPoint.column--;

		    ColorSchemeIdx colorIndex = GetColorSchemeIndexForNode(captureName);
		    // GL_INFO("[{},{}]-[{},{}]:{}--{}",startPoint.row,startPoint.column,endPoint.row,endPoint.column,nodeType,(int)colorIndex);

		    for (unsigned int row = startPoint.row; row <= endPoint.row; ++row)
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

		if (chr == '\n')
			mLines.emplace_back(Line());
		else
			mLines.back().emplace_back(Glyph(chr, ColorSchemeIdx::Default));
	}

	if (mLines.back().size() > 400)
		mLines.back().clear();

	mTextChanged = true;
	mScrollToTop = true;

	GL_INFO("FILE INFO --> Lines:{}", mLines.size());
	// mUndoManager.Clear();
	UpdateSyntaxHighlighting(text);
	Colorize();

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

void Editor::Colorize(int aFromLine, int aLines)
{
	int toLine = aLines == -1 ? (int)mLines.size() : std::min((int)mLines.size(), aFromLine + aLines);
	mColorRangeMin = std::min(mColorRangeMin, aFromLine);
	mColorRangeMax = std::max(mColorRangeMax, toLine);
	mColorRangeMin = std::max(0, mColorRangeMin);
	mColorRangeMax = std::max(mColorRangeMin, mColorRangeMax);
	mCheckComments = true;
}

void Editor::ColorizeRange(int aFromLine, int aToLine)
{
	if (mLines.empty() || aFromLine >= aToLine)
		return;

	std::string buffer;
	std::cmatch results;
	std::string id;

	int endLine = std::max(0, std::min((int)mLines.size(), aToLine));
	for (int i = aFromLine; i < endLine; ++i) {
		auto& line = mLines[i];

		if (line.empty())
			continue;

		buffer.resize(line.size());
		for (size_t j = 0; j < line.size(); ++j) {
			auto& col = line[j];
			buffer[j] = col.mChar;
			col.mColorIndex = ColorSchemeIdx::Default;
		}

		const char* bufferBegin = &buffer.front();
		const char* bufferEnd = bufferBegin + buffer.size();

		auto last = bufferEnd;

		for (auto first = bufferBegin; first != last;) {
			const char* token_begin = nullptr;
			const char* token_end = nullptr;
			ColorSchemeIdx token_color = ColorSchemeIdx::Default;

			bool hasTokenizeResult = false;

			// if (mLanguageDefinition.mTokenize != nullptr) {
			// 	if (mLanguageDefinition.mTokenize(first, last, token_begin, token_end, token_color))
			// 		hasTokenizeResult = true;
			// }

			// if (hasTokenizeResult == false) {
			// 	// todo : remove
			// 	// printf("using regex for %.*s\n", first + 10 < last ? 10 : int(last - first), first);

			// 	for (auto& p : mRegexList) {
			// 		if (std::regex_search(first, last, results, p.first, std::regex_constants::match_continuous)) {
			// 			hasTokenizeResult = true;

			// 			auto& v = *results.begin();
			// 			token_begin = v.first;
			// 			token_end = v.second;
			// 			token_color = p.second;
			// 			break;
			// 		}
			// 	}
			// }

			if (hasTokenizeResult == false) {
				first++;
			} else {
				const size_t token_length = token_end - token_begin;

				if (token_color == ColorSchemeIdx::Identifier) {
					id.assign(token_begin, token_end);

					// todo : allmost all language definitions use lower case to specify keywords, so shouldn't this use ::tolower ?
					if (!mLanguageDefinition.mCaseSensitive)
						std::transform(id.begin(), id.end(), id.begin(), ::toupper);

					if (mLanguageDefinition.mKeywords.count(id) != 0)
						token_color = ColorSchemeIdx::Keyword;
					else if (mLanguageDefinition.mIdentifiers.count(id) != 0)
						token_color = ColorSchemeIdx::KnownIdentifier;
					else if (mLanguageDefinition.mPreprocIdentifiers.count(id) != 0)
						token_color = ColorSchemeIdx::PreprocIdentifier;
				}

				for (size_t j = 0; j < token_length; ++j) line[(token_begin - bufferBegin) + j].mColorIndex = token_color;

				first = token_end;
			}
		}
	}
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
			col = (col / mTabSize) * mTabSize + mTabSize;
		else
			col++;
		i += UTF8CharLength(c);
	}
	return col;
}

int Editor::GetCurrentLineMaxColumn() const { return GetLineMaxColumn(mState.mCursorPosition.mLine); }


uint8_t Editor::GetTabCountsUptoCursor(const Coordinates& coords)const
{
	uint8_t tabCounts = 0;
	int i = 0;

	int max=GetCharacterIndex(coords);
	const auto& line=mLines[coords.mLine];
	for (; i < max;) 
	{
		if (line[i].mChar == '\t')
			tabCounts++;

		i+=UTF8CharLength(line[i].mChar);
	}

	return tabCounts;
}


size_t Editor::GetCharacterIndex(const Coordinates& aCoordinates) const
{
	if (aCoordinates.mLine >= mLines.size())
		return -1;
	auto& line = mLines[aCoordinates.mLine];
	int c = 0;
	int i = 0;
	for (; i < line.size() && c < aCoordinates.mColumn;)
	{
		if (line[i].mChar == '\t')
			c += mTabSize;
		else
			++c;
		i += UTF8CharLength(line[i].mChar);
	}
	return i;
}


void Editor::UpdateBounds()
{
	GL_WARN("UPDATING BOUNDS");
	mEditorPosition = mEditorWindow->Pos;
	GL_INFO("EditorPosition: x:{} y:{}",mEditorPosition.x,mEditorPosition.y);

	mEditorSize = ImVec2(mEditorWindow->ContentRegionRect.Max.x, mLines.size() * (mLineSpacing + mCharacterSize.y) +0.5*mLineSpacing);
	mEditorBounds = ImRect(mEditorPosition, ImVec2(mEditorPosition.x + mEditorSize.x, mEditorPosition.y + mEditorSize.y));

	reCalculateBounds = false;
}

bool Editor::IsCursorVisible(){
	int min=int(floor(mMinLineVisible));
	int count=(int)floor(mEditorWindow->Size.y/mLineHeight);

	if(mState.mCursorPosition.mLine < min) return false;
	if(mState.mCursorPosition.mLine > (min+count)) return false;

	return true;
}


float Editor::GetSelectionPosFromCoords(const Coordinates& coords)const{
	float offset{0.0f};
	if(coords==mState.mSelectionStart) offset=-1.0f;
	return mLinePosition.x - offset + (coords.mColumn * mCharacterSize.x);
}


void Editor::HighlightCurrentWordInBuffer() const {
	bool isNormalMode=mSelectionMode==SelectionMode::Normal;
	int minLine=int(mMinLineVisible);
	int count=int(mEditorWindow->Size.y/mLineHeight);

	for(const Coordinates& coord:mSearchState.mFoundPositions){
		if(coord.mLine==mState.mSelectionStart.mLine && coord.mColumn==mState.mSelectionStart.mColumn) continue;
		if(mSearchState.mIsGlobal && (coord.mLine < minLine || coord.mLine > minLine+count)) break;

		float offset=(mSelectionMode==SelectionMode::Normal) ? (mLineHeight+1.0f-0.5f*mLineSpacing) : 1.0f;
		float linePosY = (mEditorPosition.y  + (coord.mLine-floor(mMinLineVisible)) * mLineHeight)+offset;

		ImVec2 start{mLinePosition.x+coord.mColumn*mCharacterSize.x-!isNormalMode,linePosY};
		ImVec2 end{start.x+mSearchState.mWord.size()*mCharacterSize.x+(!isNormalMode*2),linePosY+(isNormalMode ? 0 : mLineHeight)};
		ImDrawList* drawlist=ImGui::GetCurrentWindow()->DrawList;

		if(isNormalMode)
			drawlist->AddLine(start,end, mGruvboxPalletDark[(size_t)Pallet::HighlightOne]);
		else
			drawlist->AddRect(start,end, mGruvboxPalletDark[(size_t)Pallet::HighlightOne]);

	}
}

// FIX: Needs a find function to search for a substring in buffer
void Editor::FindAllOccurancesOfWord(std::string word){

	mSearchState.mIsGlobal=true;
	mSearchState.mFoundPositions.clear();

	// int start = std::min(0,(int)mLines.size()-1);
	// int lineCount = (mEditorWindow->Size.y) / mLineHeight;
	// int end = mLines.size();

	// while(start<end){
    //     const Line& line = mLines[start];
    //     size_t wordIdx = 0;

    //     while ((wordIdx = line.find(word, wordIdx)) != std::string::npos) {

    //         size_t startIndex = wordIdx;
    //         size_t endIndex = wordIdx + word.length() - 1;

    //         if(
    //         	(startIndex>0 && isalnum(line[startIndex-1].mChar)) || 
    //         	(endIndex < line.size()-2 && isalnum(line[endIndex+1].mChar))
    //         )
    //         {
    //         	wordIdx=endIndex+1;
    //         	continue;
    //         }

    //         GL_TRACE("Line {} : Found '{}' at [{},{}] ",start+1,word,startIndex,endIndex);

    //         mSearchState.mFoundPositions.push_back({start,GetCharacterColumn(start,startIndex)});
    //         wordIdx = endIndex + 1;
    //     }

	// 	start++;
	// }

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



void Editor::SearchWordInCurrentVisibleBuffer(){

	OpenGL::ScopedTimer timer("WordSearch");
	mSearchState.reset();


	int start = std::min(int(mMinLineVisible),(int)mLines.size()-1);
	int lineCount = (mEditorWindow->Size.y) / mLineHeight;
	int end = std::min(start + lineCount + 1,(int)mLines.size()-1);

	const std::string currentWord=std::move(GetWordAt(mState.mCursorPosition));
	if(currentWord.empty())return;
	mSearchState.mWord=currentWord;


	GL_WARN("Searching: {}",currentWord);
	//Just for testing FindWordStart & FindWordEnd
	// const auto& scoord=FindWordStart(mState.mCursorPosition);
	// const auto& ecoord=FindWordEnd(mState.mCursorPosition);
	// GL_INFO("COORDS:{}-->{}",scoord.mColumn,ecoord.mColumn);


	// while(start<=end){
    //     const std::string& line = mLines[start];
    //     size_t wordIdx = 0;

    //     while ((wordIdx = line.find(currentWord, wordIdx)) != std::string::npos) {

    //         size_t startIndex = wordIdx;
    //         size_t endIndex = wordIdx + currentWord.length() - 1;

    //         if(
    //         	(startIndex>0 && isalnum(line[startIndex-1])) || 
    //         	(endIndex < line.size()-2 && isalnum(line[endIndex+1]))
    //         )
    //         {
    //         	wordIdx=endIndex+1;
    //         	continue;
    //         }

    //         GL_TRACE("Line {} : Found '{}' at [{},{}] ",start+1,currentWord,startIndex,endIndex);

    //         mSearchState.mFoundPositions.push_back({start,GetCharacterColumn(start,startIndex)});
    //         wordIdx = endIndex + 1;
    //     }

	// 	start++;
	// }

}

// FIX: Unicode support
std::pair<int,int> Editor::GetIndexOfWordAtCursor(const Coordinates& coords)const{

	int idx = GetCharacterIndex(coords);
	int start_idx{idx};
	int end_idx{idx};
	bool a = true, b = true;
	while ((a || b)) {
		char chr{0};
		if (start_idx == 0 || start_idx < 0) {
			a = false;
			start_idx = 0;
		} else chr = mLines[coords.mLine][start_idx - 1].mChar;


		if (a && (isalnum(chr) || chr == '_')) start_idx--;
		else a = false;

		if (end_idx >= mLines[coords.mLine].size()) {
			b = false;
			end_idx = mLines[coords.mLine].size();
		} else {
			chr = mLines[coords.mLine][end_idx].mChar;
			if (b && (isalnum(chr) || chr == '_')) end_idx++;
			else b = false;
		}
	}
	return {start_idx,end_idx};	
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



void Editor::ScrollToLineNumber(int lineNo,bool animate){

	lineNo=std::max(1,lineNo);
	if(lineNo > mLines.size()) lineNo=mLines.size();

	mState.mCursorPosition.mLine=lineNo-1;


	int lineLength=GetCurrentLineMaxColumn();
	if( lineLength < mState.mCursorPosition.mColumn) 
		mState.mCursorPosition.mColumn=lineLength;


	int totalLines=0;
	totalLines=lineNo-(int)floor(mMinLineVisible);

	float lineCount=floor((mEditorWindow->Size.y) / mLineHeight)*0.5f;
	totalLines-=lineCount;

	mScrollAmount=totalLines*mLineHeight;

	if(!animate){
		ImGui::SetScrollY(ImGui::GetScrollY()+mScrollAmount);
		return;
	}


	//Handling quick change in nextl
	if((ImGui::GetTime()-mLastClick)<0.5f){
		ImGui::SetScrollY(ImGui::GetScrollY()+mScrollAmount);
	}else{
		mInitialScrollY=ImGui::GetScrollY();
		mScrollAnimation.start();
	}

	mLastClick=(float)ImGui::GetTime();

	//=================NOTE===================
	//Enabling the below code updates the selectionStart 
	// thus disabling the selection using arrow keys
	// I don't remember why I added hence commenting it out

	// if(mSelectionMode!=SelectionMode::Normal){
	// 	mState.mSelectionStart=mState.mSelectionEnd=mState.mCursorPosition;
	// }
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

std::string Editor::GetText(const Coordinates& aStart, const Coordinates& aEnd) const
{
	std::string result;

	auto lstart = aStart.mLine;
	auto lend = aEnd.mLine;
	auto istart = GetCharacterIndex(aStart);
	auto iend = GetCharacterIndex(aEnd);

	size_t s = 0;

	for (size_t i = lstart; i < lend; i++) s += mLines[i].size();

	result.reserve(s + s / 8);

	while (istart < iend || lstart < lend) {
		if (lstart >= (int)mLines.size())
			break;

		auto& line = mLines[lstart];
		if (istart < (int)line.size()) {
			result += line[istart].mChar;
			istart++;
		} else {
			istart = 0;
			++lstart;
			result += '\n';
		}
	}

	return result;
}


std::string Editor::GetSelectedText() const { return std::move(GetText(mState.mSelectionStart, mState.mSelectionEnd)); }

void Editor::Copy()
{
	if (HasSelection()) {
		ImGui::SetClipboardText(GetSelectedText().c_str());
	} else {
		if (!mLines.empty()) {
			std::string str;
			auto& line = mLines[SanitizeCoordinates(mState.mCursorPosition).mLine];
			for (auto& g : line) str.push_back(g.mChar);

			ImGui::SetClipboardText(str.c_str());
		}
	}
}


void Editor::Paste(){
	std::string text{ImGui::GetClipboardText()};
	if(text.size()>0){
		// UndoRecord uRecord;

		if(HasSelection()){
			// uRecord.mRemovedText=GetSelectedText();
			// uRecord.mRemovedStart=mState.mSelectionStart;
			// uRecord.mRemovedEnd=mState.mSelectionEnd;
			DeleteSelection();
		}

		// uRecord.mAddedText=text;
		// uRecord.mAddedStart=mState.mCursorPosition;

		InsertTextAt(mState.mCursorPosition, text.c_str());
		mState.mSelectionStart=mState.mSelectionEnd=mState.mCursorPosition;
		// uRecord.mAddedEnd=mState.mCursorPosition;
		// uRecord.mAfterState=mState;
		// mUndoManager.AddUndo(uRecord);
	}
}

bool Editor::HasSelection() const
{
	return mState.mSelectionEnd != mState.mSelectionStart;
}



void Editor::DeleteSelection() {
	if(!HasSelection()) return;
	if(mState.mSelectionStart>mState.mSelectionEnd) std::swap(mState.mSelectionStart,mState.mSelectionEnd);
	DeleteRange(mState.mSelectionStart,mState.mSelectionEnd);
	mState.mCursorPosition=mState.mSelectionEnd=mState.mSelectionStart;
	Colorize(mState.mSelectionStart.mLine, 1);
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
	if (mState.mCursorPosition != aPosition) {
		mState.mCursorPosition = aPosition;
		// mCursorPositionChanged = true;
		// EnsureCursorVisible();
	}
}

Editor::Line& Editor::InsertLine(int aIndex)
{
	auto& result = *mLines.insert(mLines.begin() + aIndex, Line());
	return result;
}

Coordinates Editor::GetActualCursorCoordinates() const { return SanitizeCoordinates(mState.mCursorPosition); }

void Editor::InsertText(const std::string& aValue) { InsertText(aValue.c_str()); }

void Editor::InsertText(const char* aValue)
{
	if (aValue == nullptr)
		return;

	auto pos = GetActualCursorCoordinates();
	auto start = std::min(pos, mState.mSelectionStart);
	int totalLines = pos.mLine - start.mLine;

	totalLines += InsertTextAt(pos, aValue);

	SetCursorPosition(pos);
	Colorize(start.mLine - 1, totalLines + 2);
}

int Editor::InsertTextAt(Coordinates& aWhere, const char* aValue)
{
	assert(!mReadOnly);

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
	char c = line[cindex].mChar;
	if (cindex > 0 && (!isalnum(c) || c != '_')) {
		c = line[--cindex].mChar;
		if (cindex == 0)
			return at;
		if (!isalnum(c) && c != '_')
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
	// Copy();
	// Backspace();
}

void Editor::SaveFile()
{
	std::string copyStr;
	int startLine = 0;
	while (startLine < mLines.size()) {
		for (auto& glyph : mLines[startLine]) copyStr += glyph.mChar;
		copyStr += '\n';
		startLine++;
	}

	std::ofstream file(mFilePath, std::ios::trunc);
	if (!file.is_open()) {
		GL_INFO("ERROR SAVING");
		return;
	}

	file << copyStr;
	file.close();
	GL_INFO("Saving...");
	// this->isFileSaving=true;
	StatusBarManager::ShowNotification("Saved", mFilePath.c_str(), StatusBarManager::NotificationType::Success);
}

void Editor::SelectAll(){
	GL_INFO("SELECT ALL");
	mState.mSelectionEnd=mState.mCursorPosition=Coordinates(mLines.size()-1,0);
	mState.mSelectionStart=Coordinates(0,0);
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
	mState.mSelectionStart=mState.mSelectionEnd=mState.mCursorPosition;
	mSelectionMode=SelectionMode::Normal;
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
	auto width = ImGui::GetWindowWidth();

	auto top = 1 + (int)ceil(scrollY / mCharacterSize.y);
	auto bottom = (int)ceil((scrollY + height) / mCharacterSize.y);

	auto left = (int)ceil(scrollX / mCharacterSize.x);
	auto right = (int)ceil((scrollX + width) / mCharacterSize.x);

	auto pos = mState.mCursorPosition;
	// auto len = TextDistanceToLineStart(pos);

	if (pos.mLine < top)
		ImGui::SetScrollY(std::max(0.0f, (pos.mLine - 1) * mCharacterSize.y));
	if (pos.mLine > bottom - 4)
		ImGui::SetScrollY(std::max(0.0f, (pos.mLine + 4) * mCharacterSize.y - height));
	// if (len + mLineBarWidth+mPaddingLeft < left + 4)
	// 	ImGui::SetScrollX(std::max(0.0f, len + mLineBarWidth+mPaddingLeft - 4));
	// if (len + mLineBarWidth+mPaddingLeft > right - 4)
	// 	ImGui::SetScrollX(std::max(0.0f, len + mLineBarWidth+mPaddingLeft + 4 - width));
}

float Editor::TextDistanceFromLineStart(const Coordinates& aFrom) const
{
	auto& line = mLines[aFrom.mLine];
	float distance = 0.0f;
	float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;
	int colIndex = GetCharacterIndex(aFrom);
	for (size_t it = 0u; it < line.size() && it < colIndex;) {
		if (line[it].mChar == '\t') {
			distance = (1.0f + std::floor((1.0f + distance) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
			++it;
		} else {
			auto d = UTF8CharLength(line[it].mChar);
			char tempCString[7];
			int i = 0;
			for (; i < 6 && d-- > 0 && it < (int)line.size(); i++, it++) tempCString[i] = line[it].mChar;

			tempCString[i] = '\0';
			distance += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, tempCString, nullptr, nullptr).x;
		}
	}

	return distance;
}



