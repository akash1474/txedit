#include "imgui.h"
#include "pch.h"
#include "TextEditor.h"

void Editor::HandleKeyboardInputs()
{
	ImGuiIO& io = ImGui::GetIO();

	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowFocused()) {

		if (ImGui::IsWindowHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
		bool anyKeyPressed = false;
		for (int i = 0; i < IM_ARRAYSIZE(io.KeysDown); i++) {
		    if (io.KeysDown[i]) {
		        anyKeyPressed = true;
		        break;
		    }
		}
		if (anyKeyPressed && !mScrollAnimation.hasStarted && !IsCursorVisible()) ScrollToLineNumber(mState.mCursorPosition.mLine+1);

		io.WantCaptureKeyboard = true;
		io.WantTextInput = true;

		// if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
		// 	Undo();
		// else if (!IsReadOnly() && !ctrl && !shift && alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
		// 	Undo();
		// else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y)))
		// 	Redo();
		if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) 
			MoveUp();
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
		else if (!alt && ctrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_D))){

			bool condition=(mSelectionMode==SelectionMode::Word) && mSearchState.mFoundPositions.empty();

			if(mSelectionMode==SelectionMode::Normal || condition){

				if(mSelectionMode!=SelectionMode::Word) HandleDoubleClick();
				if(mState.mSelectionStart==mState.mSelectionEnd) return;
				
				int start_idx=GetCurrentLineIndex(mState.mSelectionStart);
				int end_idx=GetCurrentLineIndex(mState.mSelectionEnd);

				if(start_idx>end_idx) std::swap(start_idx,end_idx);

				mSearchState.reset();
				mSearchState.mWord=mLines[mState.mCursorPosition.mLine].substr(start_idx,end_idx-start_idx);
				GL_INFO("Search: {}",mSearchState.mWord);

				FindAllOccurancesOfWord(mSearchState.mWord);
				mCursors.push_back(mState);

				//Finding Index of Position same as currentLine to get next occurance 
				auto it=std::find_if(mSearchState.mFoundPositions.begin(),mSearchState.mFoundPositions.end(),[&](const auto& coord){
					return coord.mLine==mState.mCursorPosition.mLine;
				});

				if(it!=mSearchState.mFoundPositions.end())
					mSearchState.mIdx=std::min((int)mSearchState.mFoundPositions.size()-1,(int)std::distance(mSearchState.mFoundPositions.begin(),it)+1);
			}else{
				GL_INFO("Finding Next");
				const Coordinates& coord=mSearchState.mFoundPositions[mSearchState.mIdx];
				ScrollToLineNumber(coord.mLine+1);

				mState.mSelectionStart=mState.mSelectionEnd=coord;
				mState.mSelectionEnd.mColumn=coord.mColumn+mSearchState.mWord.size();
				GL_INFO("[{}  {} {}]",mState.mSelectionStart.mColumn,mState.mSelectionEnd.mColumn,mSearchState.mWord.size());

				mState.mCursorPosition=mState.mSelectionEnd;
				mCursors.push_back(mState);
				mSearchState.mIdx++;

				if(mSearchState.mIdx==mSearchState.mFoundPositions.size()) mSearchState.mIdx=0;
			}
		}else if(!alt && !ctrl && !shift && ImGui::IsKeyPressed(ImGuiKey_Escape)){
			if(mSelectionMode!=SelectionMode::Normal) mSelectionMode=SelectionMode::Normal;
			if(mCursors.size()) mCursors.clear();
		}
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
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
			Delete();
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
			Backspace();
		// else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
		// 	mOverwrite ^= true;
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			Copy();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
			Copy();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F)))
			Find();
		else if (!IsReadOnly() && !ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			Paste();
		else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_V)))
			Paste();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_X)))
			Cut();
		else if (!ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
			Cut();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
			SelectAll();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_S)))
			SaveFile();
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
			InsertLine();
		else if (!IsReadOnly() && !ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab))) {

			if(mSearchState.isValid()) mSearchState.reset();

			if (mSelectionMode == SelectionMode::Normal) {
				if(!mCursors.empty()){
					for(int i=0;i<mCursors.size();i++){
						int lineIdx=mCursors[i].mCursorPosition.mLine;
						int idx = GetCurrentLineIndex(mCursors[i].mCursorPosition);
						GL_INFO("IDX:", idx);

						mLines[lineIdx].insert(mLines[lineIdx].begin() + idx, 1, '\t');

						mCursors[i].mCursorPosition.mColumn += mTabWidth;

						for(int j=i+1;j<mCursors.size();j++){
							if(mCursors[j].mCursorPosition.mLine==mCursors[i].mCursorPosition.mLine)
								mCursors[j].mCursorPosition.mColumn+=mTabWidth;
						}

					}
				}else{
					int idx = GetCurrentLineIndex(mState.mCursorPosition);
					GL_INFO("IDX:", idx);

					mLines[mState.mCursorPosition.mLine].insert(mLines[mState.mCursorPosition.mLine].begin() + idx, 1, '\t');
					mState.mCursorPosition.mColumn += mTabWidth;
				}
				
				return;
			}

			if(mSelectionMode==SelectionMode::Line){

				int value=shift ? -1 : 1;

				if(shift){
					if(mLines[mState.mCursorPosition.mLine][0]!='\t') return;
					if(mState.mSelectionStart.mColumn==0)
						mState.mSelectionStart.mColumn+=mTabWidth;
					mLines[mState.mCursorPosition.mLine].erase(0,1);
				}
				else
					mLines[mState.mCursorPosition.mLine].insert(mLines[mState.mCursorPosition.mLine].begin(), 1, '\t');


				mState.mCursorPosition.mColumn += mTabWidth*value;
				mState.mSelectionStart.mColumn += mTabWidth*value;
				mState.mSelectionEnd.mColumn +=mTabWidth*value;
				return;
			}


			if(mSelectionMode==SelectionMode::Word && mState.mSelectionStart.mLine!=mState.mSelectionEnd.mLine){
				GL_INFO("INDENTING");


				int startLine=mState.mSelectionStart.mLine;
				int endLine=mState.mSelectionEnd.mLine;

				if(startLine>endLine) std::swap(startLine,endLine);


				int value=shift ? -1 : 1;
				while(startLine<=endLine){

					if(shift){
						if(mLines[startLine][0]=='\t') mLines[startLine].erase(0,1);
					}
					else
						mLines[startLine].insert(mLines[startLine].begin(), 1, '\t');


					startLine++;
				}
				mState.mSelectionStart.mColumn += mTabWidth*value;
				mState.mSelectionEnd.mColumn +=mTabWidth*value;
				mState.mCursorPosition.mColumn+=mTabWidth*value;

			}

		}

		if (!mReadOnly && !io.InputQueueCharacters.empty()) {

			if(mSearchState.isValid()) mSearchState.reset();
			// if (mSelectionMode == SelectionMode::Word) Backspace();

			// if(mSelectionMode!=SelectionMode::Normal) 
			// 	mSelectionMode=SelectionMode::Normal;

			auto c = io.InputQueueCharacters[0];

			GL_INFO("{}", (char)c);
			if (c != 0 && (c == '\n' || c >= 32)) InsertCharacter(c);

			io.InputQueueCharacters.resize(0);
		}

		if(anyKeyPressed && !ctrl) mBracketsCoordinates.coords=GetMatchingBracketsCoordinates();
	}
}