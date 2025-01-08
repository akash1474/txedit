#include "pch.h"
#include "DataTypes.h"
#include "imgui.h"
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
					auto& aState=mState.mCursors[mState.mCurrentCursorIdx];

					mSelectionMode=SelectionMode::Line;
					aState.mSelectionStart.mColumn=0;
					aState.mSelectionEnd.mColumn=GetCurrentLineMaxColumn();
					aState.mCursorPosition.mColumn=aState.mSelectionEnd.mColumn;
				}
				mLastClick = -1.0f;
}

			// Left mouse button double click
			else if (doubleClick) {

				if(!ctrl) SelectWordUnderCursor(mState.mCursors[mState.mCurrentCursorIdx]);
				mLastClick = (float)ImGui::GetTime();

			}
			else if(click && ctrl){
				// if(mSearchState.isValid())
				// 	mSearchState.reset();

				// if(mCursors.size()==0) mCursors.push_back(mState);
				Cursor aState;
				aState.mCursorPosition=ScreenPosToCoordinates(ImGui::GetMousePos());
				mState.mCursors.push_back(aState);
				mState.mCurrentCursorIdx++;


				//Sorting in ascending order
				SortCursorsFromTopToBottom();


				GL_WARN("CTRL CLICK");
			}

			// Left mouse button click
			else if (click) {
				GL_INFO("MOUSE CLICK");
				Cursor aState=mState.mCursors[mState.mCurrentCursorIdx];
				mState.mCursors.clear();

				aState.mSelectionStart=aState.mSelectionEnd=aState.mCursorPosition=ScreenPosToCoordinates(ImGui::GetMousePos());
				mSelectionMode = SelectionMode::Normal;

				// SearchWordInCurrentVisibleBuffer();

				aState.mCursorDirectionChanged=false;
				mLastClick = (float)ImGui::GetTime();


				mState.mCursors.push_back(aState);
				FindBracketMatch(aState.mCursorPosition);
			}

			//Mouse Click And Dragging
			else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0)) {
				if((ImGui::GetMousePos().y-mEditorPosition.y) <0.0f) return;

				io.WantCaptureMouse = true;
				if(mSearchState.isValid()) mSearchState.reset();
				Cursor& aCursor=GetCurrentCursor();

				aCursor.mCursorPosition=aCursor.mSelectionEnd=ScreenPosToCoordinates(ImGui::GetMousePos());
				mSelectionMode=SelectionMode::Word;

				if (aCursor.mSelectionStart > aCursor.mSelectionEnd) aCursor.mCursorDirectionChanged=true;
			}
		}
	}
}


Cursor& Editor::GetCurrentCursor(){
	if(mState.mCurrentCursorIdx >= mState.mCursors.size()) 
		mState.mCurrentCursorIdx=mState.mCursors.size()-1;

	return mState.mCursors[mState.mCurrentCursorIdx];
}

void Editor::SelectWordUnderCursor(Cursor& aCursor){
		if (mSelectionMode == SelectionMode::Line) mSelectionMode = SelectionMode::Normal;
		else
			mSelectionMode = SelectionMode::Word;

	#ifdef GL_DEBUG
		int idx = GetCharacterIndex(aCursor.mCursorPosition);
	#endif


		auto [start_idx,end_idx] = GetIndexOfWordAtCursor(aCursor.mCursorPosition);
		if(start_idx==end_idx) return;
		int tabCount=GetTabCountsUptoCursor(aCursor.mCursorPosition);


	#ifdef GL_DEBUG
		GL_WARN("SELECTION IDX:{} START:{} END:{}", idx, start_idx, end_idx);
		GL_INFO("TAB COUNT:{}", tabCount);
	#endif

		aCursor.mSelectionStart = Coordinates(aCursor.mCursorPosition.mLine, start_idx + (tabCount * (mTabSize - 1)));
		aCursor.mSelectionEnd = Coordinates(aCursor.mCursorPosition.mLine, end_idx + (tabCount * (mTabSize - 1)));

		aCursor.mCursorPosition = aCursor.mSelectionEnd;
}

void Editor::SortCursorsFromTopToBottom()
{
	if(mState.mCursors.size()<2) return;
	if(mState.mCursors[mState.mCursors.size()-2].mCursorPosition > mState.mCursors.back().mCursorPosition){

		Cursor aCursor=mState.mCursors[mState.mCurrentCursorIdx];

		std::sort(mState.mCursors.begin(), mState.mCursors.end(),[](const auto& left,const auto& right){
			return left.mCursorPosition < right.mCursorPosition;
		});

		for(int i=0;i<mState.mCursors.size();i++)
		{
			if(mState.mCursors[i].mCursorPosition==aCursor.mCursorPosition)
			{
				mState.mCurrentCursorIdx=i;
				break;
			}
		}
	}
	for(const auto& el:mState.mCursors)
		GL_INFO("[R:{}  C:{}]",el.mCursorPosition.mLine,el.mCursorPosition.mColumn);
}

void Editor::RemoveCursorsWithSameCoordinates(){
    mState.mCursors.erase(
		std::unique(mState.mCursors.begin(), mState.mCursors.end(), 
		[&](const auto& left, const auto& right) { return left.mCursorPosition == right.mCursorPosition; }), 
		mState.mCursors.end()
	);
}