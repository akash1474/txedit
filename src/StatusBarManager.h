#include "string"
#include "Coordinates.h"

class StatusBarManager{
	bool mIsInputPanelOpen=false;

public:
	void Render();
	void ShowNotification(const char* string);
	void SetFileType(const char* filetype);
	void SetCursorCoordinate(const Coordinates& cursorPosition);
};
