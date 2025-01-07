#include "imgui.h"
#include "pch.h"
#include "TextEditor.h"


void Editor::HandleMouseInputs()
{
	ImGuiIO& io = ImGui::GetIO();

	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowHovered()) {
		if (!shift && !alt) {

			auto click = ImGui::IsMouseClicked(0);
			auto doubleClick = ImGui::IsMouseDoubleClicked(0);

			auto t = ImGui::GetTime();
			auto tripleClick = click && !doubleClick && (mLastClick != -1.0f && (t - mLastClick) < io.MouseDoubleClickTime);

			//Left mouse button triple click
			if (tripleClick) {
				if(mSearchState.isValid()) mSearchState.reset();
				if (!ctrl) {
					GL_INFO("TRIPLE CLICK");
					mSelectionMode=SelectionMode::Line;
					mState.mSelectionStart.mColumn=0;
					mState.mSelectionEnd.mColumn=GetCurrentLineMaxColumn();
					mState.mCursorPosition.mColumn=mState.mSelectionEnd.mColumn;
				}
				mLastClick = -1.0f;
}

			// Left mouse button double click
			else if (doubleClick) {

				if(!ctrl) SelectWordUnderCursor();
				mLastClick = (float)ImGui::GetTime();

			}
			else if(click && ctrl){
				// if(mSearchState.isValid())
				// 	mSearchState.reset();

				if(mCursors.size()==0) mCursors.push_back(mState);
				SetCursorPosition(ScreenPosToCoordinates(ImGui::GetMousePos()));
				mCursors.push_back(mState);

				//Sorting in ascending order
				SortCursorsFromTopToBottom();


				GL_WARN("CTRL CLICK");
			}

			// Left mouse button click
			else if (click) {
				GL_INFO("MOUSE CLICK");
				if(!mCursors.empty()) mCursors.clear();


				mState.mSelectionStart=mState.mSelectionEnd=mState.mCursorPosition=ScreenPosToCoordinates(ImGui::GetMousePos());
				mSelectionMode = SelectionMode::Normal;

				SearchWordInCurrentVisibleBuffer();

				mState.mCursorDirectionChanged=false;
				mLastClick = (float)ImGui::GetTime();


				FindBracketMatch(mState.mCursorPosition);
			}

			//Mouse Click And Dragging
			else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0)) {
				if((ImGui::GetMousePos().y-mEditorPosition.y) <0.0f) return;

				io.WantCaptureMouse = true;
				if(mSearchState.isValid()) mSearchState.reset();

				mState.mCursorPosition=mState.mSelectionEnd=ScreenPosToCoordinates(ImGui::GetMousePos());
				mSelectionMode=SelectionMode::Word;

				if (mState.mSelectionStart > mState.mSelectionEnd) mState.mCursorDirectionChanged=true;
			}
		}
	}
}

void Editor::SelectWordUnderCursor(){
		if (mSelectionMode == SelectionMode::Line) mSelectionMode = SelectionMode::Normal;
		else
			mSelectionMode = SelectionMode::Word;

	#ifdef GL_DEBUG
		int idx = GetCharacterIndex(mState.mCursorPosition);
	#endif


		auto [start_idx,end_idx] = GetIndexOfWordAtCursor(mState.mCursorPosition);
		if(start_idx==end_idx) return;
		int tabCount=GetTabCountsUptoCursor(mState.mCursorPosition);


	#ifdef GL_DEBUG
		GL_WARN("SELECTION IDX:{} START:{} END:{}", idx, start_idx, end_idx);
		GL_INFO("TAB COUNT:{}", tabCount);
	#endif

		mState.mSelectionStart = Coordinates(mState.mCursorPosition.mLine, start_idx + (tabCount * (mTabSize - 1)));
		mState.mSelectionEnd = Coordinates(mState.mCursorPosition.mLine, end_idx + (tabCount * (mTabSize - 1)));

		mState.mCursorPosition = mState.mSelectionEnd;
}

void Editor::SortCursorsFromTopToBottom()
{
	if(mCursors.size()<2) return;
	if(mCursors[mCursors.size()-2].mCursorPosition > mCursors.back().mCursorPosition){
		std::sort(mCursors.begin(), mCursors.end(),[](const auto& left,const auto& right){
			return left.mCursorPosition < right.mCursorPosition;
		});
		mState=mCursors[0];
	}
	for(const auto& el:mCursors)
		GL_INFO("[R:{}  C:{}]",el.mCursorPosition.mLine,el.mCursorPosition.mColumn);
}