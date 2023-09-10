#include "imgui.h"
#include "vector"
#include "string"
#include "imgui.h"
#include "imgui_internal.h"
#include <stdint.h>


class Editor{
	enum class SelectionMode
	{
		Normal,
		Word,
		Line
	};


	float mLineSpacing=9.f;
	bool mReadOnly=false;
	int mLineHeight{0};
	uint8_t mTabWidth{3};
	uint8_t mCurrLineTabCounts{0};


	std::vector<std::string> mLines;
	int mCurrentLineIndex{0};
	size_t mCurrLineLength{0};
	float mMinLineVisible{0.0f};
	float mCurrentLineNo{0.0f};


	ImVec2 mCharacterSize;
	ImVec2 mEditorPosition;
	ImVec2 mEditorSize;
	ImVec2 mCursorPosition;
	ImRect mEditorBounds;
	ImVec2 mLinePosition;
	ImGuiWindow* mEditorWindow{0};
public:
	bool reCalculateBounds=true;
	void SetBuffer(const std::string& buffer);
	bool render();
	void setLineSpacing(float value){this->mLineSpacing=value;}
	void HandleKeyboardInputs();
	void HandleMouseInputs();
	void UpdateBounds();
	bool IsReadOnly() const { return mReadOnly; }
	void InsertCharacter(char newChar);
	void Backspace();
	void InsertLine();
	size_t GetCurrentLineLength(int currLineIndex=-1);

	Editor();
	~Editor();
};