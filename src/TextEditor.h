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
#include "unordered_set"
#include "regex"


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


static bool IsUTFSequence(char c)
{
	return (c & 0xC0) == 0x80;
}

// https://en.wikipedia.org/wiki/UTF-8
// We assume that the char is a standalone character (<128) or a leading byte of an UTF-8 code sequence (non-10xxxxxx code)
static int UTF8CharLength(uint8_t c)
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


class Editor
{
	UndoManager mUndoManager;
	enum class PaletteIndex {
		Default,
		Keyword,
		Number,
		String,
		CharLiteral,
		Punctuation,
		Preprocessor,
		Identifier,
		KnownIdentifier,
		PreprocIdentifier,
		Comment,
		MultiLineComment,
		Background,
		BackgroundDark,
		Cursor,
		Selection,
		ErrorMarker,
		Breakpoint,
		LineNumber,
		CurrentLineFill,
		CurrentLineFillInactive,
		CurrentLineEdge,
		Max
	};
	std::vector<ImU32> mGruvboxPalletDark;

	// void InitPallet()
	// {
	// 	mGruvboxPalletDark.resize((int)PaletteIndex::Max);
	// 	mGruvboxPalletDark[(int)PaletteIndex::Background] = ImColor(29, 32, 33, 255);     // 235
	// 	mGruvboxPalletDark[(int)PaletteIndex::BackgroundDark] = ImColor(29, 32, 33, 255); // 235
	// 	mGruvboxPalletDark[(int)PaletteIndex::Text] = ImColor(235, 219, 178, 255);        // 223
	// 	mGruvboxPalletDark[(int)PaletteIndex::String] = ImColor(152, 151, 26, 255);       // 106
	// 	mGruvboxPalletDark[(int)PaletteIndex::Comment] = ImColor(146, 131, 116, 255);     // 237
	// 	mGruvboxPalletDark[(int)PaletteIndex::Indentation] = ImColor(60, 56, 54, 255);    // 237
	// 	mGruvboxPalletDark[(int)PaletteIndex::Highlight] = ImColor(54, 51, 50, 255);      // 237
	// 	mGruvboxPalletDark[(int)PaletteIndex::HighlightOne] = ImColor(150, 150, 150);     // 248 // Backup ImColor(102,92,84)
	// 	mGruvboxPalletDark[(int)PaletteIndex::YellowLight] = ImColor(250, 189, 47, 255);  // 237
	// 	mGruvboxPalletDark[(int)PaletteIndex::YellowDark] = ImColor(215, 153, 33, 255);   // 237
	// 	mGruvboxPalletDark[(int)PaletteIndex::AquaLight] = ImColor(142, 192, 124, 255);   // 237
	// 	mGruvboxPalletDark[(int)PaletteIndex::AquaDark] = ImColor(104, 157, 106, 255);    // 237
	// }


	Animation mScrollAnimation;
	float mScrollAmount{0.0f};
	float mInitialScrollY{0.0f};


	bool isFileLoaded = false;
	Lexer lex;


	struct BracketCoordinates {
		std::array<Coordinates, 2> coords;
		bool hasMatched = false;
	};


	/*
	- `SelectionMode::Word`: Only one word selection
	- `SelectionMode::Line`: Only one line selection
	- `SelectionMode::Normal`: Any form of selection
	- `SelectionMode::None`: No Selection. if this is set then `HasSelection()->false`
	*/
	enum class SelectionMode { Normal, Word, Line,None };
	SelectionMode mSelectionMode{SelectionMode::None};
	BracketCoordinates mBracketsCoordinates;
	std::vector<EditorState> mCursors;
	std::string mFileContents;


	struct Identifier {
		Coordinates mLocation;
		std::string mDeclaration;
	};

	typedef std::string String;
	typedef std::unordered_map<std::string, Identifier> Identifiers;
	typedef std::unordered_set<std::string> Keywords;
	typedef std::map<int, std::string> ErrorMarkers;
	typedef std::unordered_set<int> Breakpoints;
	typedef std::array<ImU32, (unsigned)PaletteIndex::Max> Palette;
	typedef uint8_t Char;

	struct Glyph {
		Char mChar;
		PaletteIndex mColorIndex = PaletteIndex::Default;
		bool mComment : 1;
		bool mMultiLineComment : 1;
		bool mPreprocessor : 1;

		Glyph(Char aChar, PaletteIndex aColorIndex)
		    : mChar(aChar), mColorIndex(aColorIndex), mComment(false), mMultiLineComment(false), mPreprocessor(false)
		{
		}
	};

	typedef std::vector<Glyph> Line;
	typedef std::vector<Line> Lines;

	struct LanguageDefinition {
		typedef std::pair<std::string, PaletteIndex> TokenRegexString;
		typedef std::vector<TokenRegexString> TokenRegexStrings;
		typedef bool (*TokenizeCallback)(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end,
		                                 PaletteIndex& paletteIndex);

		std::string mName;
		Keywords mKeywords;
		Identifiers mIdentifiers;
		Identifiers mPreprocIdentifiers;
		std::string mCommentStart, mCommentEnd, mSingleLineComment;
		char mPreprocChar;
		bool mAutoIndentation;

		TokenizeCallback mTokenize;

		TokenRegexStrings mTokenRegexStrings;

		bool mCaseSensitive;

		LanguageDefinition() : mPreprocChar('#'), mAutoIndentation(true), mTokenize(nullptr), mCaseSensitive(true) {}

		static const LanguageDefinition& CPlusPlus();
		static const LanguageDefinition& C();
		static const LanguageDefinition& Lua();
	};

public:
	EditorState mState;


private:
	// ##### LINE BAR #######

	int mColorRangeMin, mColorRangeMax;
	typedef std::vector<std::pair<std::regex, PaletteIndex>> RegexList;

	ImU32 GetGlyphColor(const Glyph& aGlyph) const;
	void Colorize(int aFromLine = 0, int aCount = -1);
	void ColorizeRange(int aFromLine = 0, int aToLine = 0);
	void ColorizeInternal();
	static const Palette& GetDarkPalette();

	void SetLanguageDefinition(const LanguageDefinition& aLanguageDef);
	void SetPalette(const Palette& aValue);


	Palette mPaletteBase;
	Palette mPalette;
	LanguageDefinition mLanguageDefinition;
	RegexList mRegexList;

	bool mCheckComments;
	bool mCursorPositionChanged;
	Breakpoints mBreakpoints;
	ErrorMarkers mErrorMarkers;
	ImVec2 mCharAdvance;
	std::string mLineBuffer;
	bool mTextChanged;
	bool mScrollToTop;
	bool mShowWhitespaces{true};
	bool mScrollToCursor;
	uint64_t mStartTime;
	uint64_t elapsed;


	bool IsOnWordBoundary(const Coordinates& aAt) const;
	Coordinates SanitizeCoordinates(const Coordinates& aValue) const;
	bool HasSelection() const;
	void SetSelectionStart(const Coordinates& aPosition);
	void SetSelectionEnd(const Coordinates& aPosition);
	void SetSelection(const Coordinates& aStart, const Coordinates& aEnd, SelectionMode aMode = SelectionMode::Normal);
	/*
		Clears the selection(Sets SelectionStart=SelectionEnd=CursorPosition)
	
	*/
	void ClearSelection();
	void SetCursorPosition(const Coordinates & aPosition);
	Coordinates GetActualCursorCoordinates() const;
	std::string GetCurrentLineText() const;
	void EnsureCursorVisible();
	Coordinates FindNextWord(Coordinates& aFrom)const;


	std::vector<Line> mLines;
	double mLastClick{-1.0f};

	// Left Padding before the lineNumber Rendering
	float mLineBarPadding{15.0f};
	float mLineBarWidth;

	// Editor Properties
	ImVec2 mCharacterSize;
	ImVec2 mEditorPosition;
	ImVec2 mEditorSize;
	ImRect mEditorBounds;
	std::string mFilePath;
	ImGuiWindow* mEditorWindow{0};
	Coordinates mInteractiveStart, mInteractiveEnd;

	float mLineHeight;
	float mPaddingLeft{10.0f}; // Padding after LineBar
	uint8_t mTabSize{4};
	float mTitleBarHeight;
	float mLineSpacing = 10.f;
	bool mReadOnly = false;


	// Movements
	void MoveUp(bool ctrl = false, bool shift = false);
	void MoveDown(bool ctrl = false, bool shift = false);
	void MoveLeft(bool ctrl = false, bool shift = false);
	void MoveRight(bool ctrl = false, bool shift = false);
	void SwapLines(bool up = true);

	void InsertText(const std::string& aValue);
	void InsertText(const char* aValue);

	// Utility
	void Copy();
	void Paste();
	void Cut();
	void Delete();
	void SaveFile();
	void SelectAll();

	Coordinates FindWordStart(const Coordinates& aFrom) const;
	Coordinates FindWordEnd(const Coordinates& aFrom) const;

	Coordinates ScreenPosToCoordinates(const ImVec2& mousePosition) const;
	float GetSelectionPosFromCoords(const Coordinates& coords) const;

	int GetCharacterColumn(int aLine, int aIndex) const;
	int GetCharacterIndex(const Coordinates& aCoordinates) const;
	std::pair<int, int> GetWordIndex(const Coordinates& coords) const;
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

	void CalculateBracketMatch();
	std::array<Coordinates, 2> GetMatchingBracketsCoordinates();
	Coordinates FindStartBracket(const Coordinates& coords);
	Coordinates FindEndBracket(const Coordinates& coords);
	Coordinates FindMatchingBracket(const Coordinates& coords, bool searchForward) const;

	bool IsCursorVisible();
	void RenderStatusBar();
	float TextDistanceFromLineStart(const Coordinates& aFrom) const;

	void DeleteCharacter(EditorState& cursor, int cidx = -1);
	void ResetState();
	void InitFileExtensions();
	std::map<std::string, std::string> FileExtensions;

	void Advance(Coordinates & aCoordinates) const;


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

	inline uint8_t GetTabWidth() { return this->mTabSize; }


	void InsertCharacter(char newChar);
	void InsertTab(bool isShiftPressed);
	int InsertTextAt(Coordinates& aWhere, const char* aValue);
	void Backspace();
	Line& InsertLine(int aIndex);
	void InsertLineBreak(EditorState& state, int idx);

	void DeleteRange(const Coordinates& aStart, const Coordinates& aEnd);
	void DeleteSelection();

	void RemoveLine(int aIndex);
	void RemoveLine(int aStart, int aEnd);

	int GetLineMaxColumn(int currLineIndex) const;
	int GetCurrentLineMaxColumn() const;

	struct Token {
	};

	// TreeSitter Experimental
	TSParser* mParser = nullptr;
	TSTree* mTree = nullptr;
	TSQuery* mQuery = nullptr;
	TSQueryCursor* mCursor = nullptr;

	void InitTreeSitter();

	Editor();
	~Editor();
};