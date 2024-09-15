#pragma once
#include "Coordinates.h"

struct EditorState {
	Coordinates mSelectionStart;
	Coordinates mSelectionEnd;
	Coordinates mCursorPosition;
	bool mCursorDirectionChanged=false;
};