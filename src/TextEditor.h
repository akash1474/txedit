#pragma once
#include "Coordinates.h"
#include "DataTypes.h"
#include "HighlightType.h"
#include "LanguageConfig.h"
#include "TokenType.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "string"
#include "tree_sitter/api.h"
#include "vector"
#include <cstdint>
#include <mutex>
#include <stdint.h>
#include <thread>


#include "Animation.h"
#include "UndoManager.h"

// Use this when you decide to use dll for each language


inline static bool IsUTFSequence(char c) {
    return (c & 0xC0) == 0x80;
}

// https://en.wikipedia.org/wiki/UTF-8
// We assume that the char is a standalone character (<128) or a leading byte of an UTF-8 code sequence (non-10xxxxxx code)
inline static int UTF8CharLength(uint8_t c)
{
	if ((c & 0xFE) == 0xFC)
		return 6;
	if ((c & 0xFC) == 0xF8)
		return 5;
	if ((c & 0xF8) == 0xF0)
		return 4;
	else if ((c & 0xF0) == 0xE0)
		return 3;
	else if ((c & 0xE0) == 0xC0)
		return 2;
	return 1;
}

inline size_t GetUTF8StringLength(const std::string& str) {
    size_t length = 0;
    size_t i = 0;

    while (i < str.size()) {
        unsigned char c = static_cast<unsigned char>(str[i]);

        size_t char_len = 1;
        if ((c & 0x80) == 0x00) {
            char_len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            char_len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_len = 4;
        } else {
            throw std::runtime_error("Invalid UTF-8 sequence encountered.");
        }

        if (i + char_len > str.size()) {
            throw std::runtime_error("Incomplete UTF-8 character at the end of the string.");
        }

        i += char_len;
        ++length;
    }

    return length;
}



class Editor
{
	UndoManager mUndoManager;
	std::vector<ImU32> mGruvboxPalletDark;
public:
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



	typedef std::string String;
	// typedef std::unordered_map<int, bool> ErrorMarkers;
	typedef uint8_t Char;

	struct Glyph
	{
		Char mChar;
		TxTokenType mColorIndex = TxTokenType::TxDefault;
		Glyph(Char aChar, TxTokenType aColorIndex) : mChar(aChar), mColorIndex(aColorIndex){}
	};

	typedef std::vector<Glyph> Line;
	typedef std::vector<Line> Lines;

	struct LanguageDefinition {};

	struct BracketMatch {
		bool mHasMatch = false;
		Coordinates mStartBracket;
		Coordinates mEndBracket;
	};

	enum class SelectionMode { Normal, Word, Line };


	//TreeSitter Experimental
	// TSQuery* mQuery=nullptr;
	// ErrorMarkers mErrorMarkers;

	TSInputEdit mTSInputEdit;
	void DebugDisplayNearByText();
	std::string GetFullText();

	void ApplySyntaxHighlighting(const std::string &sourceCode);

	uint32_t GetLineLengthInBytes(int aLineIdx);
	void PrintTree(const TSNode &node, const std::string &source_code,std::string& output, int indent = 0);

	std::string GetNearbyLinesString(int aLineNo,int aLineCount=5);

	Coordinates GetCoordinatesFromOffset(uint32_t offset);


    std::mutex mutex_;
    std::condition_variable cv_;
    bool terminate_ = false;
    bool needsUpdate_ = false;
    std::thread workerThread_;
    std::atomic<bool> debounceFlag_{false};
	void ReparseEntireTree();
	void DebouncedReparse();
	void CloseDebounceThread();
	void WorkerThread();

private:
	TxEdit::HighlightType mHighlightType{TxEdit::HighlightType::None};

	ImU32 GetGlyphColor(const Glyph& aGlyph) const;

	LanguageConfig* mLanguageConfig=nullptr;
	bool mIsSyntaxHighlightingSupportForFile=false;


	Animation mScrollAnimation;
	float mScrollAmount{0.0f};
	float mInitialScrollY{0.0f};
	bool mTextChanged;
	bool mScrollToTop;


	bool isFileLoaded = false;




	// Cursor & Selection
	SelectionMode mSelectionMode{SelectionMode::Normal};
	void SortCursorsFromTopToBottom();
	void MergeCursorsIfNeeded();
	void ClearCursors();

public:
	EditorState mState;
	Cursor& GetCurrentCursor();


private:
	std::string mLineBuffer;
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


	Lines mLines;
	int mMinLineVisible{0};
	double mLastClick{-1.0f};
	bool mBufferModified{0};


	// Editor Properties
	ImVec2 mCharacterSize;
	ImVec2 mEditorPosition;
	ImVec2 mEditorSize;
	ImVec2 mLinePosition;
	std::string mFilePath;
	ImGuiWindow* mEditorWindow{0};

	int mLineHeight{0};
	float mPaddingLeft{5.0f};
	int mTabSize{4};
	float mLineSpacing = 8.0f;
	bool mReadOnly = false;


	// Movements
	void MoveUp(bool ctrl = false, bool shift = false);
	void MoveDown(bool ctrl = false, bool shift = false);
	void MoveLeft(bool ctrl = false, bool shift = false);
	void MoveRight(bool ctrl = false, bool shift = false);
	void SwapLines(bool up = true);

	/*
		Returns pixel `ImVec2(x,y)` coordinates on screen at which `aCoords` line is located
		@return start postion of aCoord.mLine
	*/
	ImVec2 GetLinePosition(const Coordinates& aCoords);


	// Utility
	void Copy();
	void Paste();
	void Cut();
	void Delete();
	void SelectAll();

	Coordinates GetActualCursorCoordinates() const;
	void SetCursorPosition(const Coordinates& aPosition);


	Line& InsertLine(int aIndex);
	void InsertText(const std::string& aValue);
	void InsertText(const char* aValue);

	Coordinates SanitizeCoordinates(const Coordinates& aValue) const;

	Coordinates FindWordStart(const Coordinates& aFrom) const;
	Coordinates FindWordEnd(const Coordinates& aFrom) const;

	Coordinates ScreenPosToCoordinates(const ImVec2& aPosition)const;
	float GetSelectionPosFromCoords(const Coordinates& coords) const;

	std::string GetSelectedText(const Cursor& aCursor) const;
	std::string GetText(const Coordinates& aStart, const Coordinates& aEnd) const;

	void DisableSelection();

	int GetCharacterColumn(int aLine, int aIndex) const;
	std::pair<int, int> GetIndexOfWordAtCursor(const Coordinates& coords) const;
	std::string GetWordAt(const Coordinates& coords) const;

	// Search & Find
	struct SearchState {
		std::string mWord;
		size_t mWordLen;
		std::vector<Coordinates> mFoundPositions;
		bool mIsGlobal = false;
		int mIdx = 0;
		bool IsValid() const { return mWord.length() > 0 && mFoundPositions.size() > 0; }
		void SetSearchWord(const std::string& word){
			mWord=word;
			mWordLen=GetUTF8StringLength(word);
		}
		void Reset()
		{
			mWord.clear();
			mFoundPositions.clear();
			mIsGlobal = false;
			mIdx = 0;
			mWordLen=0;
		}
	};
	SearchState mSearchState;

	void PerformSearch(const std::string& aWord);

	void HighlightCurrentWordInBuffer();
	void FindAllOccurancesOfWord(std::string word,size_t aStartLineIdx,size_t aEndLineIdx);
	void FindAllOccurancesOfWordInVisibleBuffer();
	void SelectWordUnderCursor(Cursor& aCursor);
	void Find();

	void HandleCtrlD();


	// Status & UI


	uint8_t GetTabCountsUptoCursor(const Coordinates& aCoords) const;
	uint8_t GetTabCountsAtLineStart(const Coordinates& aCoords);
	size_t GetCharacterIndex(const Coordinates& coords) const;

	BracketMatch mBracketMatch;

	bool HasBracketMatch()const{return mBracketMatch.mHasMatch;}
	Coordinates FindStartBracket(const Coordinates& coords);
	Coordinates FindEndBracket(const Coordinates& coords);
	void FindBracketMatch(const Coordinates& aCoords);
	void HighlightBracket(const Coordinates& aCoords);


	void DeleteCharacter(Cursor& aCursor,bool aDeletePreviousCharacter,UndoRecord* uRecord=nullptr);
	void ResetState();

	struct Highlight{
		Coordinates aStart,aEnd;
		int iStart,iEnd;
		int lineNo;
		bool isPresent{0};
		float startTime;
	};

	Highlight mHighlight;
	std::vector<std::string> mSuggestions;
	size_t iCurrentSuggestion{0};

	void RenderHighlight(const Highlight& aHighlight);
	void RenderSuggestionBox(const std::vector<std::string>& suggestions, size_t& selectedIndex);
	void ApplySuggestion(const std::string& aString,Cursor& aCursor);

	std::string mFileTypeName;
public:
	inline bool IsHighlightPresent()const
	{
		return mHighlight.isPresent;
	}

	void CreateHighlight(int aLineNumber,int aStartIndex,int aEndIndex);
	size_t GetCursorCount(){return mState.mCursors.size();}

	std::string GetCurrentlyTypedWord();
	inline bool HasSuggestions()const{return !mSuggestions.empty();}
	void ClearSuggestions()
	{
		mSuggestions.clear();
		iCurrentSuggestion=0;
	}



	float maxLineWidth{0.0f}; // max horizontal scroll;
	void SetBuffer(const std::string& aBuffer);
	void ClearEditor();
	ImGuiWindow* GetImGuiWindowPtr(){return mEditorWindow;}

	EditorState* GetEditorState() { return &mState; }
	UndoManager* GetUndoMananger() { return &this->mUndoManager; }

	void ExecuteSearch(const std::string& aWord);
	void GotoNextMatch();
	void GotoPreviousMatch();
	void HighlightAllMatches();
	void DisableSearch(){
		if(mSearchState.IsValid())
			mSearchState.Reset();
	}
	bool HasSearchStarted(const std::string& aWord){return mSearchState.mWord==aWord && mSearchState.IsValid() && mSearchState.mIsGlobal;}

	std::string GetSelectedText();

	void EnsureCursorVisible();
	void UpdateSyntaxHighlighting(int aLineNo,int aLineCount=3);
	int InsertTextAt(Coordinates& aWhere, const char* aValue);
	std::string GetText();

	std::string GetCurrentFilePath() const { return mFilePath; };
	std::string GetFileTypeName();

	void LoadFile(const char* filepath);
	int GetSelectionMode() const { return (int)mSelectionMode; };

	bool Render(bool* aIsOpen,std::string& aUUID,bool& aIsTemp);
	bool Draw();
	void HandleKeyboardInputs();
	void HandleMouseInputs();

	inline bool IsBufferModified()const{return mBufferModified;}
	//Called inside Undo/Redo Functions
	inline void SetIsBufferModified(bool aIsModifiedValue){mBufferModified=aIsModifiedValue;}


	void SetLineSpacing(float value) { this->mLineSpacing = value; }
	// Only scroll to that line but doesn't update any positions
	void ScrollToLineNumber(int lineNo, bool animate = true);
	bool IsReadOnly() const { return mReadOnly; }

	bool HasSelection(const Cursor& aCursor) const;
	inline uint8_t GetTabWidth() { return this->mTabSize; }


	bool IsOpeningBracket(char aChar);
	void InsertAroundSelection(char aChar);
	void InsertCharacter(char newChar);
	void InsertTab(bool isShiftPressed);
	void Backspace();
	void InsertLine();
	void InsertLineBreak();

	void MoveHome(bool aShift =false);
	void MoveEnd(bool aShift =false);
	void MoveTop(bool aShift = false);
	void MoveBottom(bool aShift = false);

	float TextDistanceFromLineStart(const Coordinates& aFrom) const;
	void DeleteRange(const Coordinates& aStart, const Coordinates& aEnd);
	void DeleteSelection(Cursor& aCursor);

	void RemoveLine(int aIndex);
	void RemoveLine(int aStart, int aEnd);

	int GetLineMaxColumn(int currLineIndex) const;
	int GetCurrentLineMaxColumn();

	Editor();
	~Editor();
};




//converts a Unicode code point (c) into its corresponding UTF-8 encoded byte sequence and stores it in a buffer.
static inline int ImTextCharToUtf8(char* buf, int buf_size, unsigned int c)
{
	if (c < 0x80)
	{
		buf[0] = (char)c;
		return 1;
	}
	if (c < 0x800)
	{
		if (buf_size < 2) return 0;
		buf[0] = (char)(0xc0 + (c >> 6));
		buf[1] = (char)(0x80 + (c & 0x3f));
		return 2;
	}
	if (c >= 0xdc00 && c < 0xe000)
	{
		return 0;
	}
	if (c >= 0xd800 && c < 0xdc00)
	{
		if (buf_size < 4) return 0;
		buf[0] = (char)(0xf0 + (c >> 18));
		buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
		buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[3] = (char)(0x80 + ((c) & 0x3f));
		return 4;
	}
	//else if (c < 0x10000)
	{
		if (buf_size < 3) return 0;
		buf[0] = (char)(0xe0 + (c >> 12));
		buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[2] = (char)(0x80 + ((c) & 0x3f));
		return 3;
	}
}
