#include "imgui.h"
#include "vector"
#include "string"
#include "imgui.h"
#include "imgui_internal.h"


class Editor{
	float mLineSpacing=9.f;
	bool mReadOnly=false;
	std::vector<std::string> mLines;
	ImVec2 mCharacterSize;
	ImVec2 mEditorPosition;
	ImVec2 mEditorSize;
	ImVec2 mCursorPosition;
	ImRect mEditorBounds;
	ImVec2 mLinePosition;
	ImGuiWindow* mEditorWindow{0};
	int mLineHeight{0};
public:
	void SetBuffer(const std::string& buffer);
	bool render();
	void setLineSpacing(float value){this->mLineSpacing=value;}
	void HandleKeyboardInputs();
	bool IsReadOnly() const { return mReadOnly; }
	Editor();
	~Editor();
};