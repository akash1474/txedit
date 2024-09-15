#include "Coordinates.h"
#include "FontAwesome6.h"
#include "Lexer.h"
#include "Timer.h"
#include "UndoManager.h"
#include "imgui_internal.h"
#include "pch.h"
#include "TextEditor.h"

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


Editor::Editor()
{
	InitPallet();
	InitFileExtensions();
	// #undef IM_TABSIZE
	// #define IM_TABSIZE mTabWidth
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
	this->ResetState();
	size_t size{0};
	std::ifstream t(filepath);
	if(t.good()) mFilePath=filepath;
	fileType=GetFileType();

	std::string file_data{0};
	t.seekg(0, std::ios::end);
	size = t.tellg();
	file_data.resize(size, ' ');
	t.seekg(0);
	t.read(&file_data[0], size);
	this->SetBuffer(file_data);
	isFileLoaded=true;
	reCalculateBounds=true;
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
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

	ImGui::SetNextWindowClass(&window_class);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("#editor_container",0,winFlags | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
		ImGui::PopStyleVar();




	    ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
	    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags | ImGuiDockNodeFlags_NoResize);

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

std::string Editor::GetText(const Coordinates & aStart, const Coordinates & aEnd) const
{
	std::string result;

	auto lstart = aStart.mLine;
	auto lend = aEnd.mLine;
	auto istart = GetCharacterIndex(aStart);
	auto iend = GetCharacterIndex(aEnd);

	//Single Line/Word Selection
	if(lstart==lend) return mLines[lstart].substr(istart,iend-istart);

	//Multiline
	result+=mLines[lstart].substr(istart);result+='\n';
	for(int i=lstart+1;i<lend;i++) result+=mLines[i]+'\n';
	result+=mLines[lend].substr(0,iend);

	return result;
}

std::string Editor::GetSelectedText() const{
	return std::move(GetText(mState.mSelectionStart,mState.mSelectionEnd));
}



bool Editor::Draw()
{
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

		mTitleBarHeight = 0;
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

	if( ImGui::IsMouseDown(0) && mSelectionMode==SelectionMode::Word && (ImGui::GetMousePos().y>(mEditorPosition.y+mEditorWindow->Size.y))){
		ImGui::SetScrollY(ImGui::GetScrollY()+mLineHeight);
		if(mState.mSelectionEnd.mLine < mLines.size()-1){
			mState.mSelectionEnd.mLine++;
			mState.mCursorPosition.mLine++;
		}
	}

	if(ImGui::IsMouseDown(0) && mSelectionMode==SelectionMode::Word && (ImGui::GetMousePos().y<mEditorPosition.y)){
		ImGui::SetScrollY(ImGui::GetScrollY()-mLineHeight);
		if(mState.mSelectionEnd.mLine > 0){
			mState.mSelectionEnd.mLine--;
			mState.mCursorPosition.mLine--;
		}
	}

	// // BackGrounds
	// mEditorWindow->DrawList->AddRectFilled({mEditorPosition.x + mLineBarWidth, mEditorPosition.y}, mEditorBounds.Max,mGruvboxPalletDark[(size_t)Pallet::Background]); // Code




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


	mMinLineVisible = fmax(0.0f,ImGui::GetScrollY() / mLineHeight) ;
	mLinePosition.y = (mEditorPosition.y+mTitleBarHeight  + (mState.mCursorPosition.mLine-floor(mMinLineVisible)) * mLineHeight);
	mLinePosition.x = mEditorPosition.x + mLineBarWidth + mPaddingLeft-ImGui::GetScrollX();



	//Highlight Selections
	if (mSelectionMode == SelectionMode::Word || mSelectionMode==SelectionMode::Line) {
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
						int posY=(mEditorPosition.y+mTitleBarHeight + (cursor.mCursorPosition.mLine-floor(mMinLineVisible)) * mLineHeight);
						ImVec2 start(GetSelectionPosFromCoords(cursor.mSelectionStart), posY);
						ImVec2 end(GetSelectionPosFromCoords(cursor.mSelectionEnd), posY + mLineHeight);

						mEditorWindow->DrawList->AddRectFilled(start, end, mGruvboxPalletDark[(size_t)Pallet::Highlight]);
					}
				}
			}

		}else if ((selectionStart.mLine+1)==selectionEnd.mLine){ //Rendering selection two lines

			float prevLinePositonY=mLinePosition.y;
			if(mState.mCursorDirectionChanged){
				mLinePosition.y = (mEditorPosition.y+mTitleBarHeight + (selectionEnd.mLine-floor(mMinLineVisible)) * mLineHeight);
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
				mLinePosition.y = (mEditorPosition.y+mTitleBarHeight + (selectionEnd.mLine-floor(mMinLineVisible)) * mLineHeight);
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


	int start = std::min(int(mMinLineVisible),(int)mLines.size());
	int lineCount = (mEditorWindow->Size.y) / mLineHeight;
	int end = std::min(start + lineCount + 1,(int)mLines.size());


	int lineNo = 0;
	int i_prev=0;
	const std::vector<Lexer::Token>& tokens=lex.GetTokens();

	//Rendering Lines and Vertical Indentation Lines
	while (start != end) {

		// float x=mLinePosition.x;
		// for(int i=0;i<tokens.size();i++){
		// 	if(tokens[i].location.mLine==start){
		// 		ImU32 color= tokens[i].type==Lexer::HeaderName ? mGruvboxPalletDark[(size_t)Pallet::String] : mGruvboxPalletDark[(size_t)Pallet::Text];

		// 		float linePosY = mEditorPosition.y+mLineSpacing + (lineNo * mLineHeight) + mTitleBarHeight;
		// 		mEditorWindow->DrawList->AddText({x+(tokens[i].location.mColumn*mCharacterSize.x), linePosY}, color, tokens[i].text);
		// 	}else{
		// 		break;
		// 	}
		// }	
		float linePosY = mEditorPosition.y + (lineNo * mLineHeight) + mTitleBarHeight+(0.5*mLineSpacing);
		mEditorWindow->DrawList->AddText({mLinePosition.x, linePosY}, mGruvboxPalletDark[(size_t)Pallet::Text], mLines[start].c_str());

		//Indentation Lines
		if(mLines[start].empty()){
			int i=i_prev;
			while(i>-1){
				ImVec2 indentStart{mLinePosition.x+(i*mTabWidth*mCharacterSize.x), linePosY-(0.5f*mLineSpacing)};
				mEditorWindow->DrawList->AddLine(indentStart, {indentStart.x,indentStart.y+mLineHeight}, mGruvboxPalletDark[(size_t)Pallet::Indentation]);
				i--;
			}
		}else{
			int i=0;
			while(mLines[start].size() > i && mLines[start][i]=='\t'){
				ImVec2 indentStart{mLinePosition.x+(i*mTabWidth*mCharacterSize.x), linePosY-(0.5f*mLineSpacing)};
				mEditorWindow->DrawList->AddLine(indentStart, {indentStart.x,indentStart.y+mLineHeight}, mGruvboxPalletDark[(size_t)Pallet::Indentation]);
				i++;
			}
			i_prev=--i;
		}
		start++;
		lineNo++;
	}




	// Cursor
	if(mCursors.empty()){
		ImVec2 cursorPosition(mLinePosition.x - 1.0f + (mState.mCursorPosition.mColumn * mCharacterSize.x), mLinePosition.y);
		mEditorWindow->DrawList->AddRectFilled(cursorPosition, {cursorPosition.x + 2.0f, cursorPosition.y + mLineHeight},ImColor(255, 255, 255, 255));
	}else{

		for(const EditorState& cursor:mCursors){
			int lineY = (mEditorPosition.y+mTitleBarHeight + (cursor.mCursorPosition.mLine-floor(mMinLineVisible)) * mLineHeight);
			ImVec2 cursorPosition(mLinePosition.x - 1.0f + (cursor.mCursorPosition.mColumn * mCharacterSize.x), lineY);
			mEditorWindow->DrawList->AddRectFilled(cursorPosition, {cursorPosition.x + 2.0f, cursorPosition.y + mLineHeight},ImColor(255, 255, 255, 255));
		}

	}




	start = std::min(int(mMinLineVisible),(int)mLines.size());
	lineCount = (mEditorWindow->Size.y) / mLineHeight;
	end = std::min(start + lineCount + 1,(int)mLines.size());



	bool isTrue=mSearchState.isValid() && mSelectionMode!=SelectionMode::Line;
	if(isTrue)
		HighlightCurrentWordInBuffer();

	//Line Number Background
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition}, {mEditorPosition.x + mLineBarWidth, mEditorSize.y}, mGruvboxPalletDark[(size_t)Pallet::Background]); // LineNo
	// Highlight Current Lin
	mEditorWindow->DrawList->AddRectFilled({mEditorPosition.x,mLinePosition.y},{mEditorPosition.x+mLineBarWidth, mLinePosition.y + mLineHeight},mGruvboxPalletDark[(size_t)Pallet::Highlight]); // Code
	mLineHeight = mLineSpacing + mCharacterSize.y;



	//Rendering for selected word
	if(isTrue){
		bool isNormalMode=mSelectionMode==SelectionMode::Normal;
		for(const auto& coord:mSearchState.mFoundPositions){
			float linePosY = (mEditorPosition.y+mTitleBarHeight  + (coord.mLine-floor(mMinLineVisible)) * mLineHeight);
			ImVec2 start{mEditorPosition.x,linePosY};
			ImVec2 end{mEditorPosition.x+4.0f,linePosY+(isNormalMode ? 0 : mLineHeight)};

			mEditorWindow->DrawList->AddRectFilled(start,end, mGruvboxPalletDark[(size_t)Pallet::Text]);
		}
	}


	//Highlighting Brackets
	if(mBracketsCoordinates.hasMatched){
		for(Coordinates& coord:mBracketsCoordinates.coords){
			if (coord.mColumn >= mLines[coord.mLine].size()) continue;
			int column=GetCharacterColumn(coord.mLine,coord.mColumn);

			float linePosY = (mEditorPosition.y+mTitleBarHeight  + (coord.mLine-floor(mMinLineVisible)) * mLineHeight);
			int tabs=GetTabCountsUptoCursor(coord)*(mTabWidth-1);

			ImVec2 start{mLinePosition.x+column*mCharacterSize.x,linePosY};
			mEditorWindow->DrawList->AddRect(start,{start.x+mCharacterSize.x+1,start.y+mLineHeight}, mGruvboxPalletDark[(size_t)Pallet::HighlightOne]);
		}
	}


	//Horizonal scroll Shadow
	if(ImGui::GetScrollX()>0.0f){
		ImVec2 pos_start{mEditorPosition.x+mLineBarWidth,0.0f};
		mEditorWindow->DrawList->AddRectFilledMultiColor(pos_start,{pos_start.x+10.0f,mEditorWindow->Size.y}, ImColor(19,21,21,130),ImColor(19,21,21,0),ImColor(19,21,21,0),ImColor(19,21,21,130));
	}


	//Rendering Line Number
	lineNo = 0;
	while (start != end) {
		float linePosY =mEditorPosition.y + (lineNo * mLineHeight) + mTitleBarHeight+ 0.5f*mLineSpacing;
		float linePosX=mEditorPosition.x + mLineBarPadding + (mLineBarMaxCountWidth-GetNumberWidth(start+1))*mCharacterSize.x;

		mEditorWindow->DrawList->AddText({linePosX, linePosY}, (start==mState.mCursorPosition.mLine) ? mGruvboxPalletDark[(size_t)Pallet::Text] : mGruvboxPalletDark[(size_t)Pallet::Comment], std::to_string(start + 1).c_str());

		start++;
		lineNo++;
	}


	HandleKeyboardInputs();
	HandleMouseInputs();

	return true;
}


std::array<Coordinates,2> Editor::GetMatchingBracketsCoordinates(){
	OpenGL::ScopedTimer timer("BracketMatching");
	std::array<Coordinates,2> coords;
	bool hasBracketsMatched=true;
	mBracketsCoordinates.hasMatched=false;
	char match=-1;
	GL_INFO("CHAR:{}",mLines[mState.mCursorPosition.mLine][GetCharacterIndex(mState.mCursorPosition)]);

	const Coordinates cStart=FindMatchingBracket(mState.mCursorPosition, false,match);
	if(cStart.mLine==INT_MAX) hasBracketsMatched=false;
	if(!hasBracketsMatched) return coords;
	coords[0]=cStart;
	GL_INFO("BracketMatch Start:[{},{}]",coords[0].mLine,coords[0].mColumn);

	const Coordinates cEnd=FindMatchingBracket(mState.mCursorPosition, true,match);
	if(cEnd.mLine==INT_MAX) hasBracketsMatched=false;

	if(hasBracketsMatched){
		coords[1]=cEnd;
		mBracketsCoordinates.hasMatched=true;
		GL_INFO("BracketMatch End:[{},{}]",coords[1].mLine,coords[1].mColumn);
	}

	return coords;
}

Coordinates Editor::FindMatchingBracket(const Coordinates& coords,bool searchForward,char& match){
	Coordinates coord{INT_MAX,INT_MAX};

	int cLine=coords.mLine;
	int cColumn=std::max(0,(int)GetCharacterIndex(coords)-1);

	bool isFound=false;
	int ignore=0;
	char x=-1;
	while(
		searchForward ? 
		(cLine < mLines.size()) :  
		(cLine>=0))
	{
		const auto& line=mLines[cLine];
		if(line.empty()){
			if(searchForward){cColumn=0; cLine++; } 
			else{cColumn=mLines[cLine-1].size()-1; cLine--; }
			if(cLine<0 || cLine==mLines.size()) break;
			continue;
		}

		while(
			searchForward ? 
			(cColumn < line.size()) : 
			(cColumn>=0))
		{
			if(line[cColumn]==x){ x=-1; searchForward ? cColumn++ : cColumn--;   continue; }
			if(line[cColumn]=='\'' || line[cColumn]=='"'){x=line[cColumn]; searchForward ? cColumn++ : cColumn--; continue; }
			if(x!=-1) {searchForward ? cColumn++ : cColumn--;continue;}
			if(searchForward){
				switch(line[cColumn]){
					case '(': ignore++; break;
					case '[': ignore++; break;
					case '{': ignore++; break;
					case ')':{if(ignore>0) ignore--; break; }
					case ']':{if(ignore>0) ignore--; break; }
					case '}':{if(ignore>0) ignore--; break; }
				}
				if(match>0 && ignore==0 && match==line[cColumn]) isFound=true;
			}else{
				switch(line[cColumn]) {
					case ')': ignore++;break;
					case ']': ignore++;break;
					case '}': ignore++;break;
					case '(':{if(!ignore){match = ')'; isFound=true;} if(ignore>0) ignore--; break; }
					case '[':{if(!ignore){match = ']'; isFound=true;} if(ignore>0) ignore--; break; }
					case '{':{if(!ignore){match = '}'; isFound=true;} if(ignore>0) ignore--; break; }
				}
			}
			if(isFound)	{
				coord.mLine=cLine;
				coord.mColumn=cColumn;
				return coord;
			}

			searchForward ? cColumn++ : cColumn--;
		}

		if(cLine==0 || cLine==mLines.size()-1) break;
		if(searchForward){cColumn=0; cLine++; } 
		else{
			cColumn = mLines[cLine - 1].empty() ? 0 : mLines[cLine - 1].size() - 1;
			cLine--;
		}
		
	}
	return coord;
}


void Editor::SetBuffer(const std::string& text)
{
	mLines.clear();
	std::string currLine="";

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



void Editor::RenderStatusBar(){
	ImGuiIO& io=ImGui::GetIO();
	//[TODO]
}




size_t Editor::GetLineMaxColumn(int currLineIndex)const {
	if(mLines[currLineIndex].empty()) return 0;

	int tabCounts = 0;

	int max = mLines[currLineIndex].size();
	for (int i = 0; i < max; i++) if(mLines[currLineIndex][i] == '\t') tabCounts++;

	return max - tabCounts + (tabCounts * mTabWidth);
}

size_t Editor::GetCurrentLineMaxColumn()const{
	return GetLineMaxColumn(mState.mCursorPosition.mLine);
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


uint32_t Editor::GetCharacterIndex(const Coordinates& coords)const
{
	uint8_t tabCounts=GetTabCountsUptoCursor(coords);
	//(mTabWidth-1) as each tab replaced by mTabWidth containing one '/t' for each tabcount
	int val = coords.mColumn - (tabCounts * (mTabWidth - 1));
	//Hack for fixing 0 idx while getting the char idx of '\t' located at 0th idx;
	if(tabCounts==1 && val<0 && coords.mColumn>0) return 1;
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
		float linePosY = (mEditorPosition.y+mTitleBarHeight  + (coord.mLine-floor(mMinLineVisible)) * mLineHeight)+offset;

		ImVec2 start{mLinePosition.x+coord.mColumn*mCharacterSize.x-!isNormalMode,linePosY};
		ImVec2 end{start.x+mSearchState.mWord.size()*mCharacterSize.x+(!isNormalMode*2),linePosY+(isNormalMode ? 0 : mLineHeight)};
		ImDrawList* drawlist=ImGui::GetCurrentWindow()->DrawList;

		if(isNormalMode)
			drawlist->AddLine(start,end, mGruvboxPalletDark[(size_t)Pallet::HighlightOne]);
		else
			drawlist->AddRect(start,end, mGruvboxPalletDark[(size_t)Pallet::HighlightOne]);

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

            mSearchState.mFoundPositions.push_back({start,GetCharacterColumn(start,startIndex)});
            wordIdx = endIndex + 1;
        }

		start++;
	}

}

std::string Editor::GetWordAt(const Coordinates& coords)const{
	auto [start_idx,end_idx]=GetIndexOfWordAtCursor(mState.mCursorPosition);
	if(start_idx==end_idx) return "";
	return mLines[mState.mCursorPosition.mLine].substr(start_idx,end_idx-start_idx);
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

            mSearchState.mFoundPositions.push_back({start,GetCharacterColumn(start,startIndex)});
            wordIdx = endIndex + 1;
        }

		start++;
	}

}


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
		} else chr = mLines[coords.mLine][start_idx - 1];


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



Coordinates Editor::MapScreenPosToCoordinates(const ImVec2& mousePosition){
	Coordinates coords;

	float currentLineNo=(ImGui::GetScrollY() + (mousePosition.y-mEditorPosition.y) - mTitleBarHeight) / mLineHeight;
	coords.mLine = std::max(0,(int)floor(currentLineNo - (mMinLineVisible - floor(mMinLineVisible))));	
	if(coords.mLine > mLines.size()-1) coords.mLine=mLines.size()-1;

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

	mState.mCursorPosition.mLine = coords.mLine;

	int lineLength=GetCurrentLineMaxColumn();
	if(coords.mColumn > lineLength) coords.mColumn=lineLength;

	return coords;
}


int Editor::GetCharacterColumn(int aLine,int aIndex)const{
	if(aLine>=mLines.size()) return 0;
	if(aIndex<0) return 0;
	if(aIndex>mLines[aLine].size()-1) return GetLineMaxColumn(aLine);

	int column{0};
	for(int i=0;i<aIndex;i++){
		if(mLines[aLine][i]=='\t') column+=mTabWidth;
		else column++;
	}

	return column;
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
	if(mSelectionMode!=SelectionMode::Normal){
		mState.mSelectionStart=mState.mSelectionEnd=mState.mCursorPosition;
	}
}



void Editor::Copy(){
	if(mSelectionMode==SelectionMode::Normal) return;

	Coordinates selectionStart=mState.mSelectionStart;
	Coordinates selectionEnd=mState.mSelectionEnd;

	if(selectionStart>selectionEnd) std::swap(selectionStart,selectionEnd);

	if(selectionStart.mLine==selectionEnd.mLine)
	{
		uint32_t start=GetCharacterIndex(selectionStart);
		uint32_t end=GetCharacterIndex(selectionEnd);
		uint8_t word_len = end-start;

		std::string selection = mLines[mState.mCursorPosition.mLine].substr(start,word_len);
		ImGui::SetClipboardText(selection.c_str());
	}

	else{

		std::string copyStr;

		//start
		uint8_t start=GetCharacterIndex(selectionStart);
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
		end=GetCharacterIndex(selectionEnd);
		word_len=end-start;

		copyStr+=mLines[selectionEnd.mLine].substr(start,word_len);

		ImGui::SetClipboardText(copyStr.c_str());
	}
}


void Editor::Paste(){
	std::string text{ImGui::GetClipboardText()};
	if(text.size()>0){
		UndoRecord uRecord;

		if(HasSelection()){
			uRecord.mRemovedText=GetSelectedText();
			uRecord.mRemovedStart=mState.mSelectionStart;
			uRecord.mRemovedEnd=mState.mSelectionEnd;
			DeleteSelection();
		}

		uRecord.mAddedText=text;
		uRecord.mAddedStart=mState.mCursorPosition;

		InsertTextAt(mState.mCursorPosition, text);
		uRecord.mAddedEnd=mState.mCursorPosition;
		uRecord.mAfterState=mState;
		mUndoManager.AddUndo(uRecord);
	}
}

bool Editor::HasSelection()const{
	return mSelectionMode==SelectionMode::Word || mSelectionMode==SelectionMode::Line;
}


void Editor::DeleteSelection() {
	DeleteRange(mState.mSelectionStart,mState.mSelectionEnd);
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

void Editor::InsertTextAt(Coordinates& aWhere, std::string& data)
{
	if(mSearchState.isValid()) mSearchState.reset();
	if(mSelectionMode==SelectionMode::Word) Backspace();
	int idx=GetCharacterIndex(aWhere);

	size_t foundIndex=data.find('\n');
	bool isMultiLineText=foundIndex!=std::string::npos;

	if(!isMultiLineText){
		mLines[aWhere.mLine].insert(idx,data);
		mState.mCursorPosition.mColumn+=data.size();
		return;
	}


	//Inserting into currentline
	std::string segment=data.substr(0,foundIndex);

	size_t word_len=mLines[aWhere.mLine].size()-idx;
	std::string end_segment=mLines[aWhere.mLine].substr(idx,word_len);
	mLines[aWhere.mLine].erase(idx,word_len);

	mLines[aWhere.mLine].insert(idx,segment);

	std::string line;
	int lineIndex=aWhere.mLine+1;
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

	mState.mCursorPosition.mLine=lineIndex;
	mState.mCursorPosition.mColumn=GetCurrentLineMaxColumn();

	mLines[mState.mCursorPosition.mLine].append(end_segment);

}


Coordinates Editor::FindWordStart(const Coordinates& aFrom) const{
	Coordinates at=aFrom;
	if(at.mLine>(int)mLines.size()) return at;

	auto& line=mLines[at.mLine];
	int cindex=GetCharacterIndex(at);

	if(cindex >= line.size()) return at;

	if(cindex > 0 && isspace(line[cindex])) cindex--;
	if(cindex > 0 && isspace(line[cindex])) return at;

	while( cindex > 0 && (isalnum(line[cindex]) || line[cindex]=='_')) cindex--;

	if((!isalnum(line[cindex]) && line[cindex]!='_')) cindex++;
	return Coordinates(at.mLine,GetCharacterColumn(at.mLine,cindex));
}


Coordinates Editor::FindWordEnd(const Coordinates& aFrom) const{
	int idx=GetCharacterIndex(aFrom);

	if(mLines[aFrom.mLine][idx]==' ') idx++;
	if(idx < mLines[aFrom.mLine].size() && mLines[aFrom.mLine][idx]==' ') return aFrom;
	while(idx < mLines[aFrom.mLine].size() && (isalnum(mLines[aFrom.mLine][idx]) || mLines[aFrom.mLine][idx]=='_')) idx++;
	return {aFrom.mLine,GetCharacterColumn(aFrom.mLine, idx-1)};
}


void Editor::Cut(){
	Copy();
	Backspace();
}

void Editor::SaveFile(){
	std::string copyStr;
	int startLine=0;
	while(startLine < mLines.size()){
		copyStr+=mLines[startLine];
		copyStr+='\n';
		startLine++;
	}

	std::ofstream file(mFilePath,std::ios::trunc);
	if(!file.is_open()){
		GL_INFO("ERROR SAVING");
		return;
	}

	file << copyStr;
	file.close();
	GL_INFO("Saving...");
	// this->isFileSaving=true;
	StatusBarManager::ShowNotification("Saved", mFilePath.c_str(),StatusBarManager::NotificationType::Success);

}

void Editor::SelectAll(){
	GL_INFO("SELECT ALL");
	mState.mSelectionEnd=mState.mCursorPosition=Coordinates(mLines.size()-1,0);
	mState.mSelectionStart=Coordinates(0,0);
	mSelectionMode=SelectionMode::Word;
}


void Editor::Find(){
	GL_INFO("FIND");
	StatusBarManager::ShowInputPanel("Word:",[](const char* value){
		GL_INFO("CallbackFN:{}",value);
		// StatusBarManager::ShowNotification("Created:", file_path,StatusBarManager::NotificationType::Success);
	},nullptr,true,"Save");
}








