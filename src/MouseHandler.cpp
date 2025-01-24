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

	if (ImGui::IsWindowHovered()) 
	{
		if (!shift && !alt) 
		{

			auto click = ImGui::IsMouseClicked(0);
			auto doubleClick = ImGui::IsMouseDoubleClicked(0);

			auto t = ImGui::GetTime();
			auto tripleClick = click && !doubleClick && (mLastClick != -1.0f && (t - mLastClick) < io.MouseDoubleClickTime);

			//Left mouse button triple click
			if (tripleClick) 
			{
				DisableSearch();
				if (!ctrl) 
				{
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
			else if (doubleClick) 
			{

				if(!ctrl) SelectWordUnderCursor(mState.mCursors[mState.mCurrentCursorIdx]);
				mLastClick = (float)ImGui::GetTime();

				mSelectionMode=SelectionMode::Word;

				auto& aCursor=GetCurrentCursor();
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
			else if(click && ctrl)
			{
				DisableSearch();

				Cursor aState;
				aState.mCursorPosition=ScreenPosToCoordinates(ImGui::GetMousePos());
				mState.mCursors.push_back(aState);
				mState.mCurrentCursorIdx++;


				SortCursorsFromTopToBottom();


				GL_WARN("CTRL CLICK");
			}

			// Left mouse button click
			else if (click) 
			{
				GL_INFO("MOUSE CLICK");

				DisableSearch();
				ClearCursors();
				ClearSuggestions();

				Cursor& aState=GetCurrentCursor();

				aState.mSelectionStart=aState.mSelectionEnd=aState.mCursorPosition=ScreenPosToCoordinates(ImGui::GetMousePos());
				mSelectionMode = SelectionMode::Normal;


				aState.mCursorDirectionChanged=false;
				mLastClick = (float)ImGui::GetTime();


				FindBracketMatch(aState.mCursorPosition);
				FindAllOccurancesOfWordInVisibleBuffer();
			}

			//Mouse Click And Dragging
			else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0)) 
			{
				if((ImGui::GetMousePos().y-mEditorPosition.y) <0.0f) 
					return;

				io.WantCaptureMouse = true;
				DisableSearch();
				Cursor& aCursor=GetCurrentCursor();

				aCursor.mCursorPosition=aCursor.mSelectionEnd=ScreenPosToCoordinates(ImGui::GetMousePos());
				// mSelectionMode=SelectionMode::Word;

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


#ifdef GL_DEBUG
	GL_WARN("SELECTION IDX:{} START:{} END:{}", idx, start_idx, end_idx);
#endif

	aCursor.mSelectionStart = Coordinates(aCursor.mCursorPosition.mLine, GetCharacterColumn(aCursor.mCursorPosition.mLine,start_idx));
	aCursor.mSelectionEnd = Coordinates(aCursor.mCursorPosition.mLine, GetCharacterColumn(aCursor.mCursorPosition.mLine,end_idx));

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

void Editor::MergeCursorsIfNeeded(){
	// requires the cursors to be sorted from top to bottom
	std::unordered_set<int> cursorsToDelete;

	if (HasSelection(GetCurrentCursor()))
	{
		// merge cursors if they overlap
		for (int c = mState.mCurrentCursorIdx; c > 0; c--)// iterate backwards through pairs
		{
			int pc = c - 1; // pc for previous cursor

			bool pcContainsC = mState.mCursors[pc].mSelectionEnd >= mState.mCursors[c].mSelectionEnd;
			bool pcContainsStartOfC = mState.mCursors[pc].mSelectionEnd > mState.mCursors[c].mSelectionStart;

			if (pcContainsC)
			{
				cursorsToDelete.insert(c);
			}
			else if (pcContainsStartOfC)
			{
				Coordinates pcStart = mState.mCursors[pc].mSelectionStart;
				Coordinates cEnd = mState.mCursors[c].mSelectionEnd;
				mState.mCursors[pc].mSelectionEnd = cEnd;
				mState.mCursors[pc].mCursorPosition = cEnd;
				cursorsToDelete.insert(c);
			}
		}
	}
	else
	{
		// merge cursors if they are at the same position
		for (int c = mState.mCurrentCursorIdx; c > 0; c--)// iterate backwards through pairs
		{
			int pc = c - 1;
			if (mState.mCursors[pc].mCursorPosition == mState.mCursors[c].mCursorPosition)
				cursorsToDelete.insert(c);
		}
	}


	for (int c = mState.mCurrentCursorIdx; c > -1; c--)// iterate backwards through each of them
	{
		if (cursorsToDelete.find(c) != cursorsToDelete.end())
			mState.mCursors.erase(mState.mCursors.begin() + c);
	}
	mState.mCurrentCursorIdx -= cursorsToDelete.size();
}


void Editor::ClearCursors(){
	if(mState.mCursors.size()==1) return; 

	Cursor& currentCursor=GetCurrentCursor();
	mState.mCursors[0]=currentCursor;
	mState.mCurrentCursorIdx=0;

	mState.mCursors.erase(mState.mCursors.begin()+1,mState.mCursors.end());
}