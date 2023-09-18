#include "imgui.h"
#include "imgui_internal.h"
#include "string"
#include "vector"
#include <map>
#include <stdint.h>
#include <unordered_map>

class Editor
{
	enum class Pallet { Background = 0, BackgroundDark, Text, String, Comment,Highlight, Max };
	std::vector<ImU32> mGruvboxPalletDark;
	// Google

	void InitPallet()
	{
		mGruvboxPalletDark.resize((size_t)Pallet::Max);
		mGruvboxPalletDark[(size_t)Pallet::Background] = ImColor(29,32,33, 255);     // 235
		mGruvboxPalletDark[(size_t)Pallet::BackgroundDark] = ImColor(29, 32, 33, 255); // 235
		mGruvboxPalletDark[(size_t)Pallet::Text] = ImColor(235, 219, 178, 255);        // 223
		mGruvboxPalletDark[(size_t)Pallet::String] = ImColor(152, 151, 26, 255);       // 106
		mGruvboxPalletDark[(size_t)Pallet::Comment] = ImColor(146,131,116,255);        // 237
		mGruvboxPalletDark[(size_t)Pallet::Highlight] = ImColor(54,51,50,255);        // 237
	}

	struct Coordinates {
		int mLine, mColumn;
		Coordinates() : mLine(0), mColumn(0) {}
		Coordinates(int aLine, int aColumn) : mLine(aLine), mColumn(aColumn)
		{
			assert(aLine >= 0);
			assert(aColumn >= 0);
		}
		static Coordinates Invalid()
		{
			static Coordinates invalid(-1, -1);
			return invalid;
		}

		bool operator==(const Coordinates& o) const { return mLine == o.mLine && mColumn == o.mColumn; }

		bool operator!=(const Coordinates& o) const { return mLine != o.mLine || mColumn != o.mColumn; }

		bool operator<(const Coordinates& o) const
		{
			if (mLine != o.mLine) return mLine < o.mLine;
			return mColumn < o.mColumn;
		}

		bool operator>(const Coordinates& o) const
		{
			if (mLine != o.mLine) return mLine > o.mLine;
			return mColumn > o.mColumn;
		}

		bool operator<=(const Coordinates& o) const
		{
			if (mLine != o.mLine) return mLine < o.mLine;
			return mColumn <= o.mColumn;
		}

		bool operator>=(const Coordinates& o) const
		{
			if (mLine != o.mLine) return mLine > o.mLine;
			return mColumn >= o.mColumn;
		}
	};

	enum class SelectionMode { Normal, Word, Line };

	struct EditorState {
		Coordinates mSelectionStart;
		Coordinates mSelectionEnd;
		Coordinates mCursorPosition;
		bool mCursorDirectionChanged=false;
	};




	SelectionMode mSelectionMode{SelectionMode::Normal};
	float mLineFloatPart{0.0f};
	EditorState mState;

	//##### LINE BAR #######
	float mLineBarWidth{0.0f}; //init -> num_count+2*mLineBarPadding
	float mLineBarPadding{15.0f};
	float mLineBarMaxCountWidth{0};
	inline uint8_t GetNumberWidth(int number){
	    uint8_t length = 0;
	    do {length++; number /= 10; } while (number != 0);
	    return length;
	}



	//###### EDITOR SPACE ######
	float mLineSpacing = 10.f;
	bool mReadOnly = false;
	int mLineHeight{0};
	float mPaddingLeft{5.0f};
	uint8_t mTabWidth{4};
	uint8_t mCurrLineTabCounts{0};
	float mTitleBarHeight{0.0f};


	std::vector<std::string> mLines;
	int mCurrentLineIndex{0};
	size_t mCurrLineLength{0};
	float mMinLineVisible{0.0f};

	double mLastClick{-1.0f};


	ImVec2 mCharacterSize;
	ImVec2 mEditorPosition;
	ImVec2 mEditorSize;
	ImRect mEditorBounds;
	ImVec2 mLinePosition;
	ImGuiWindow* mEditorWindow{0};

	uint32_t GetCurrentLineIndex();
	void MoveUp(bool ctrl = false, bool shift = false);
	void SwapLines(bool up = true);
	void MoveDown(bool ctrl = false, bool shift = false);
	void MoveLeft(bool ctrl = false, bool shift = false);
	void MoveRight(bool ctrl = false, bool shift = false);
	void Copy();
	void Paste();
	void Cut();
	Coordinates MapScreenPosToCoordinates(const ImVec2& mousePosition);
	float GetSelectionPosFromCoords(const Coordinates& coords)const;

  public:
	bool reCalculateBounds = true;
	void SetBuffer(const std::string& buffer);
	bool render();
	void setLineSpacing(float value) { this->mLineSpacing = value; }
	void HandleKeyboardInputs();
	void HandleMouseInputs();
	void UpdateBounds();
	bool IsReadOnly() const { return mReadOnly; }
	void InsertCharacter(char newChar);
	void Backspace();
	void InsertLine();
	inline uint8_t GetTabWidth() { return this->mTabWidth; }

	void UpdateTabCountsUptoCursor();

	size_t GetCurrentLineLength(int currLineIndex = -1);
	Editor::EditorState* GetEditorState(){return &mState; }


	Editor();
	~Editor();
};