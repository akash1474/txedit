#include "pch.h"
#include "imgui.h"
#include "TextEditor.h"

void Editor::HandleKeyboardInputs()
{
	ImGuiIO& io = ImGui::GetIO();

	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowFocused()) {

		if (ImGui::IsWindowHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
		
		// if (ImGui::IsKeyPressed(ImGuiKey_Space) || ImGui::IsKeyPressed(ImGuiKey_RightArrow) || ImGui::IsKeyPressed(ImGuiKey_LeftArrow) ||
		//     ImGui::IsKeyPressed(ImGuiKey_UpArrow) ||
		//     ImGui::IsKeyPressed(ImGuiKey_UpArrow) && !mScrollAnimation.hasStarted && !IsCursorVisible())
		// 	ScrollToLineNumber(mState.mCursorPosition.mLine + 1);

		io.WantCaptureKeyboard = true;
		io.WantTextInput = true;

		if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Z))
			mUndoManager.Undo(5, this);
		else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Y))
			mUndoManager.Redo(5, this);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			MoveUp();
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			MoveDown();
		else if (ctrl && shift && !alt && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			SwapLines(true);
		else if (ctrl && shift && !alt && ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			SwapLines(false);
		else if (!alt && ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
			MoveLeft(ctrl, shift);
		else if (!alt && ImGui::IsKeyPressed(ImGuiKey_RightArrow))
			MoveRight(ctrl, shift);
		else if (!alt && ctrl && ImGui::IsKeyPressed(ImGuiKey_D))
			HandleCtrlD();
		else if (!alt && !ctrl && !shift && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			auto& aCursor=GetCurrentCursor();

			if(HasSelection(aCursor))
			{
				DisableSelection();
				return;
			}

			if(mState.mCursors.size()>1){
				mState.mCursors[0]=aCursor;
				mState.mCursors.erase(mState.mCursors.begin()+1);
				mState.mCurrentCursorIdx=0;
			}
		}
		// else if (!alt && ImGui::IsKeyPressed(ImGuiKey_PageDown))
		// 	MoveDown(GetPageSize() - 4, shift);
		else if (!alt && ctrl && ImGui::IsKeyPressed(ImGuiKey_Home))
			MoveTop(shift);
		else if (ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_End))
			MoveBottom(shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_Home))
			MoveHome(shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_End))
			MoveEnd(shift);
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Delete))
			Delete();
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Backspace))
			Backspace();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_C))
			Copy();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_V))
			Paste();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_X))
			Cut();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_A))
			SelectAll();
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Enter))
		{
			if(HasSuggestions())
			{
		        for(auto& cursor:mState.mCursors)
		        	ApplySuggestion(mSuggestions[iCurrentSuggestion],cursor);

		        ClearSuggestions();
			}
			else
				InsertLineBreak();
		}
		else if (!IsReadOnly() && !ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_Tab))
			InsertTab(shift);

		if (!mReadOnly && !io.InputQueueCharacters.empty()) {

			// if (mSearchState.isValid())
			// 	mSearchState.reset();
			Cursor& aCursor=GetCurrentCursor();

			if (HasSelection(aCursor))
				Backspace();

			if (HasSelection(aCursor))
				DisableSelection();

			auto c = io.InputQueueCharacters[0];

			GL_INFO("{}", (char)c);
			if (c != 0 && (c == '\n' || c >= 32))
				InsertCharacter(c);

			io.InputQueueCharacters.resize(0);
		}

	}
}