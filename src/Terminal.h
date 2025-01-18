#pragma once
#include "Coordinates.h"
#include "imgui_internal.h"
#include "string"
#include "vector"
#include "mutex"
#include "Windows.h"
#include <list>
#include <thread>
#include <wincontypes.h>
#include "ConPTY.h"
#include "future"

class Terminal
{
public:
	Terminal();
	~Terminal();

	void Render();

	struct Cursor{
		Coordinates mSelectionStart;
		Coordinates mSelectionEnd;
		Coordinates mCursorPosition;
		bool mCursorDirectionChanged=false;
	};

private:
	std::mutex mOutputMutex; // To synchronize output access
	bool mIsCommandRunning;  // Flag to indicate a command is running
	bool mScrollToBottom;    // Flag to auto-scroll output
	std::string mBuffer;
	std::future<void> mReaderThread;
	bool mReaderThreadIsRunning;
	ConPTY mConPTY;
	double mPrevTime = 0;


	void StartShell();
	void CloseShell();
	void ShellReader();

	// Function to send a command to the shell
	void RunCommand(std::string& command);


private:
	// Command History
	std::list<std::string> mHistory;
	std::list<std::string>::iterator mHistoryIterator;
	void PushHistory(std::string& cmd);
	void PushHistoryCommand(bool direction);


private:
	enum class SelectionMode { Normal, Word, Line };
	SelectionMode mSelectionMode{SelectionMode::Normal};

	// Text Renderer
	Cursor mState;
	std::vector<std::string> mLines;
	ImGuiWindow* mWindow = nullptr;
	ImVec2 mPosition;
	ImVec2 mCharacterSize;
	ImVec2 mLinePosition;
	ImRect mBounds;
	ImVec2 mSize;
	float mLastClick;
	float mMinLineVisible;
	float mLineSpacing{5.0f};
	float mTitleBarHeight{0};
    int mLineHeight{0};
	bool mRecalculateBounds = false;
    uint8_t mTabWidth{4};
	Coordinates mReadOnlyCoords;
	std::string mCommand;

	float mPaddingLeft = 5.0f;

	void Draw();
	void UpdateBounds();
	float GetSelectionPosFromCoords(const Coordinates& coords) const;
	size_t GetLineMaxColumn(int currLineIndex) const;
	void AppendText(const std::string& str);
	Coordinates GetCommandInsertPosition();
	int GetCharacterIndex(const Coordinates& aCoordinates) const;
	std::pair<int, int> GetIndexOfWordAtCursor(const Coordinates& coords) const;
	Coordinates MapScreenPosToCoordinates(const ImVec2& mousePosition);
	void HandleInputs();
	void ExitSelectionMode() { mSelectionMode = SelectionMode::Normal; };
	int GetCharacterColumn(int aLine, int aIndex) const;
};