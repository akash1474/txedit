#include "imgui.h"
#include "vector"
#include "string"
#include "imgui.h"
#include "imgui_internal.h"
#include <stdint.h>


class Editor{
	struct Coordinates
	{
		int mLine, mColumn;
		Coordinates() : mLine(0), mColumn(0) {}
		Coordinates(int aLine, int aColumn) : mLine(aLine), mColumn(aColumn)
		{
			assert(aLine >= 0);
			assert(aColumn >= 0);
		}
		static Coordinates Invalid() { static Coordinates invalid(-1, -1); return invalid; }

		bool operator ==(const Coordinates& o) const
		{
			return
				mLine == o.mLine &&
				mColumn == o.mColumn;
		}

		bool operator !=(const Coordinates& o) const
		{
			return
				mLine != o.mLine ||
				mColumn != o.mColumn;
		}

		bool operator <(const Coordinates& o) const
		{
			if (mLine != o.mLine)
				return mLine < o.mLine;
			return mColumn < o.mColumn;
		}

		bool operator >(const Coordinates& o) const
		{
			if (mLine != o.mLine)
				return mLine > o.mLine;
			return mColumn > o.mColumn;
		}

		bool operator <=(const Coordinates& o) const
		{
			if (mLine != o.mLine)
				return mLine < o.mLine;
			return mColumn <= o.mColumn;
		}

		bool operator >=(const Coordinates& o) const
		{
			if (mLine != o.mLine)
				return mLine > o.mLine;
			return mColumn >= o.mColumn;
		}
	};

	enum class SelectionMode
	{
		Normal,
		Word,
		Line
	};

	struct EditorState
	{
		Coordinates mSelectionStart;
		Coordinates mSelectionEnd;
		Coordinates mCursorPosition;
	};


	SelectionMode mSelectionMode{0};
	EditorState mState;


	float mLineSpacing=12.f;
	bool mReadOnly=false;
	int mLineHeight{0};
	uint8_t mTabWidth{3};
	uint8_t mCurrLineTabCounts{0};
	float mTitleBarHeight{0.0f};


	std::vector<std::string> mLines;
	int mCurrentLineIndex{0};
	size_t mCurrLineLength{0};
	float mMinLineVisible{0.0f};
	float mCurrentLineNo{0.0f};

	double mLastClick{-1.0f};


	ImVec2 mCharacterSize;
	ImVec2 mEditorPosition;
	ImVec2 mEditorSize;
	ImRect mEditorBounds;
	ImVec2 mLinePosition;
	ImGuiWindow* mEditorWindow{0};

	inline uint32_t GetCurrentLineIndex();

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
	size_t GetCurrentLineLengthUptoCursor();

	Editor();
	~Editor();
};