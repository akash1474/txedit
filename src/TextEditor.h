#pragma once
#include "Coordinates.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "string"
#include "tree_sitter/api.h"
#include "vector"
#include <map>
#include <stdint.h>
#include <tuple>
#include <array>
#include <unordered_map>


#include "Animation.h"
#include "Lexer.h"
#include "UndoManager.h"


#ifndef TREE_SITTER_CPP_H_
	#define TREE_SITTER_CPP_H_

typedef struct TSLanguage TSLanguage;

	#ifdef __cplusplus
extern "C" {
	#endif

	const TSLanguage* tree_sitter_cpp(void);

	#ifdef __cplusplus
}
	#endif

#endif // TREE_SITTER_CPP_H_


class Editor
{
	UndoManager mUndoManager;
	enum class Pallet {
		Background = 0,
		BackgroundDark,
		Text,
		String,
		Comment,
		Indentation,
		Highlight,
		YellowLight,
		YellowDark,
		AquaLight,
		AquaDark,
		HighlightOne,
		Max
	};
	std::vector<ImU32> mGruvboxPalletDark;

	void InitPallet()
	{
		mGruvboxPalletDark.resize((size_t)Pallet::Max);
		mGruvboxPalletDark[(size_t)Pallet::Background] = ImColor(29, 32, 33, 255);     // 235
		mGruvboxPalletDark[(size_t)Pallet::BackgroundDark] = ImColor(29, 32, 33, 255); // 235
		mGruvboxPalletDark[(size_t)Pallet::Text] = ImColor(235, 219, 178, 255);        // 223
		mGruvboxPalletDark[(size_t)Pallet::String] = ImColor(152, 151, 26, 255);       // 106
		mGruvboxPalletDark[(size_t)Pallet::Comment] = ImColor(146, 131, 116, 255);     // 237
		mGruvboxPalletDark[(size_t)Pallet::Indentation] = ImColor(60, 56, 54, 255);    // 237
		mGruvboxPalletDark[(size_t)Pallet::Highlight] = ImColor(54, 51, 50, 255);      // 237
		mGruvboxPalletDark[(size_t)Pallet::HighlightOne] = ImColor(150, 150, 150);     // 248 // Backup ImColor(102,92,84)
		mGruvboxPalletDark[(size_t)Pallet::YellowLight] = ImColor(250, 189, 47, 255);  // 237
		mGruvboxPalletDark[(size_t)Pallet::YellowDark] = ImColor(215, 153, 33, 255);   // 237
		mGruvboxPalletDark[(size_t)Pallet::AquaLight] = ImColor(142, 192, 124, 255);   // 237
		mGruvboxPalletDark[(size_t)Pallet::AquaDark] = ImColor(104, 157, 106, 255);    // 237
	}


	Animation mScrollAnimation;
	float mScrollAmount{0.0f};
	float mInitialScrollY{0.0f};


	bool isFileLoaded = false;
	Lexer lex;


	struct BracketCoordinates {
		std::array<Coordinates, 2> coords;
		bool hasMatched = false;
	};


	// Cursor & Selection
	enum class SelectionMode { Normal, Word, Line };
	SelectionMode mSelectionMode{SelectionMode::Normal};
	BracketCoordinates mBracketsCoordinates;
	std::vector<EditorState> mCursors;
	std::string mFileContents;

public:
	EditorState mState;


private:
	// ##### LINE BAR #######
	float mLineBarWidth{0.0f}; // init -> num_count+2*mLineBarPadding
	float mLineBarPadding{15.0f};
	float mLineBarMaxCountWidth{0};
	inline uint8_t GetNumberWidth(int number)
	{
		uint8_t length = 0;
		do {
			length++;
			number /= 10;
		} while (number != 0);
		return length;
	}


	std::vector<std::string> mLines;
	float mMinLineVisible{0.0f};
	double mLastClick{-1.0f};


	// Editor Properties
	ImVec2 mCharacterSize;
	ImVec2 mEditorPosition;
	ImVec2 mEditorSize;
	ImRect mEditorBounds;
	ImVec2 mLinePosition;
	std::string mFilePath;
	ImGuiWindow* mEditorWindow{0};

	int mLineHeight{0};
	float mPaddingLeft{5.0f};
	uint8_t mTabWidth{4};
	float mTitleBarHeight{0.0f};
	float mLineSpacing = 10.f;
	bool mReadOnly = false;


	// Movements
	void MoveUp(bool ctrl = false, bool shift = false);
	void MoveDown(bool ctrl = false, bool shift = false);
	void MoveLeft(bool ctrl = false, bool shift = false);
	void MoveRight(bool ctrl = false, bool shift = false);
	void SwapLines(bool up = true);


	// Utility
	void Copy();
	void Paste();
	void Cut();
	void Delete();
	void SaveFile();
	void SelectAll();

	Coordinates FindWordStart(const Coordinates& aFrom) const;
	Coordinates FindWordEnd(const Coordinates& aFrom) const;

	Coordinates MapScreenPosToCoordinates(const ImVec2& mousePosition);
	float GetSelectionPosFromCoords(const Coordinates& coords) const;

	int GetCharacterColumn(int aLine, int aIndex) const;
	std::pair<int, int> GetIndexOfWordAtCursor(const Coordinates& coords) const;
	std::string GetWordAt(const Coordinates& coords) const;

	// Search & Find
	struct SearchState {
		std::string mWord;
		std::vector<Coordinates> mFoundPositions;
		bool mIsGlobal = false;
		int mIdx = 0;
		bool isValid() const { return mWord.length() > 0; }
		void reset()
		{
			mWord.clear();
			mFoundPositions.clear();
			mIsGlobal = false;
			mIdx = 0;
		}
	};

	SearchState mSearchState;
	void SearchWordInCurrentVisibleBuffer();
	void HighlightCurrentWordInBuffer() const;
	void FindAllOccurancesOfWord(std::string word);
	void HandleDoubleClick();
	void Find();


	// Status & UI


	uint8_t GetTabCountsUptoCursor(const Coordinates& coords) const;
	uint32_t GetCharacterIndex(const Coordinates& coords) const;

	void CalculateBracketMatch();
	std::array<Coordinates, 2> GetMatchingBracketsCoordinates();
	Coordinates FindStartBracket(const Coordinates& coords);
	Coordinates FindEndBracket(const Coordinates& coords);
	Coordinates FindMatchingBracket(const Coordinates& coords, bool searchForward) const;

	bool IsCursorVisible();
	void RenderStatusBar();

	void DeleteCharacter(EditorState& cursor, int cidx = -1);
	void ResetState();
	void InitFileExtensions();
	std::map<std::string, std::string> FileExtensions;


public:
	bool reCalculateBounds = true;
	float maxLineWidth{0.0f}; // max horizontal scroll;
	std::string fileType;
	void SetBuffer(const std::string& buffer);
	void ClearEditor();

	EditorState* GetEditorState() { return &mState; }
	UndoManager* GetUndoMananger() { return &this->mUndoManager; }

	std::string GetCurrentFilePath() const { return mFilePath; };
	std::string GetFileType();
	void LoadFile(const char* filepath);
	int GetSelectionMode() const { return (int)mSelectionMode; };

	std::string GetSelectedText() const;
	std::string GetText(const Coordinates& aStart, const Coordinates& aEnd) const;

	void Render();
	bool Draw();
	void HandleKeyboardInputs();
	void HandleMouseInputs();
	void UpdateBounds();


	void SetLineSpacing(float value) { this->mLineSpacing = value; }
	void ScrollToLineNumber(int lineNo, bool animate = true);
	void RecalculateBounds() { this->reCalculateBounds = true; }
	bool IsReadOnly() const { return mReadOnly; }

	bool HasSelection() const;
	inline uint8_t GetTabWidth() { return this->mTabWidth; }


	void InsertCharacter(char newChar);
	void InsertTab(bool isShiftPressed);
	void InsertTextAt(Coordinates& aWhere, std::string& aValue);
	void Backspace();
	void InsertLine();
	void InsertLineBreak(EditorState& state, int idx);

	void DeleteRange(const Coordinates& aStart, const Coordinates& aEnd);
	void DeleteSelection();

	void RemoveLine(int aIndex);
	void RemoveLine(int aStart, int aEnd);

	size_t GetLineMaxColumn(int currLineIndex) const;
	size_t GetCurrentLineMaxColumn() const;

	struct Token{
		
	};

	//TreeSitter Experimental
	TSParser* mParser=nullptr;
	TSTree* mTree=nullptr;
	TSQuery* mQuery=nullptr;
	TSQueryCursor* mCursor=nullptr;

	void InitTreeSitter();

	Editor();
	~Editor();
};