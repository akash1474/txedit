#pragma once
#include "Coordinates.h"
#include "vector"

struct Cursor{
	Coordinates mSelectionStart;
	Coordinates mSelectionEnd;
	Coordinates mCursorPosition;
	bool mCursorDirectionChanged=false;
};

struct EditorState {
	std::vector<Cursor> mCursors;
	int mCurrentCursorIdx=0;
};

