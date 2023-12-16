#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include "string"
#include "vector"
#include <map>
#include <stdint.h>
#include <tuple>
#include <array>
#include <unordered_map>


#include "Animation.h"
#include "Lexer.h"


class Editor
{
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
		HighlightOne,
		Max 
	};
	std::vector<ImU32> mGruvboxPalletDark;

	void InitPallet()
	{
		mGruvboxPalletDark.resize((size_t)Pallet::Max);
		mGruvboxPalletDark[(size_t)Pallet::Background] = ImColor(29,32,33, 255);     // 235
		mGruvboxPalletDark[(size_t)Pallet::BackgroundDark] = ImColor(29, 32, 33, 255); // 235
		mGruvboxPalletDark[(size_t)Pallet::Text] = ImColor(235, 219, 178, 255);        // 223
		mGruvboxPalletDark[(size_t)Pallet::String] = ImColor(152, 151, 26, 255);       // 106
		mGruvboxPalletDark[(size_t)Pallet::Comment] = ImColor(146,131,116,255);        // 237
		mGruvboxPalletDark[(size_t)Pallet::Indentation] = ImColor(60,56,54,255);        // 237
		mGruvboxPalletDark[(size_t)Pallet::Highlight] = ImColor(54,51,50,255);        // 237
		mGruvboxPalletDark[(size_t)Pallet::HighlightOne] = ImColor(102,92,84);   // 248
		mGruvboxPalletDark[(size_t)Pallet::YellowLight] = ImColor(250,189,47,255);        // 237
		mGruvboxPalletDark[(size_t)Pallet::YellowDark] = ImColor(215,153,33,255);        // 237
	}




	Animation mScrollAnimation;
	float mScrollAmount{0.0f};
	float mInitialScrollY{0.0f};


	bool isFileLoaded=false;
	Lexer lex;

	enum class SelectionMode { Normal, Word, Line };

	struct EditorState {
		Coordinates mSelectionStart;
		Coordinates mSelectionEnd;
		Coordinates mCursorPosition;
		bool mCursorDirectionChanged=false;
	};


	struct BracketCoordinates{
		std::array<Coordinates,2> coords;
		bool hasMatched=false;
	};

	std::vector<EditorState> mCursors;
	BracketCoordinates mBracketsCoordinates;




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
	float mTitleBarHeight{0.0f};


	std::vector<std::string> mLines;
	float mMinLineVisible{0.0f};

	double mLastClick{-1.0f};


	ImVec2 mCharacterSize;
	ImVec2 mEditorPosition;
	ImVec2 mEditorSize;
	ImRect mEditorBounds;
	ImVec2 mLinePosition;
	std::string mFilePath;
	ImGuiWindow* mEditorWindow{0};


	void SwapLines(bool up = true);

	void MoveUp(bool ctrl = false, bool shift = false);
	void MoveDown(bool ctrl = false, bool shift = false);
	void MoveLeft(bool ctrl = false, bool shift = false);
	void MoveRight(bool ctrl = false, bool shift = false);

	void Copy();
	void Paste();
	void Cut();
	void Delete();
	void SaveFile();
	void SelectAll();

	Coordinates MapScreenPosToCoordinates(const ImVec2& mousePosition);
	float GetSelectionPosFromCoords(const Coordinates& coords)const;

	int GetColumnNumberFromIndex(int idx,int lineIdx);
	std::pair<int,int> GetIndexOfWordAtCursor(const Coordinates& coords)const;


	struct SearchState{
		std::string mWord;
		std::vector<Coordinates> mFoundPositions;
		bool mIsGlobal=false;
		int mIdx=0;
		bool isValid()const {return mWord.length()>0;}
		void reset(){
			mWord.clear();
			mFoundPositions.clear();
			mIsGlobal=false;
			mIdx=0;
		}
	};

	SearchState mSearchState;

	void SearchWordInCurrentVisibleBuffer();
	void HighlightCurrentWordInBuffer() const;

	void FindAllOccurancesOfWord(std::string word);
	void HandleDoubleClick();


	uint8_t GetTabCountsUptoCursor(const Coordinates& coords)const;
	uint32_t GetCurrentLineIndex(const Coordinates& cursorPosition)const;


	std::array<Coordinates,2> GetMatchingBracketsCoordinates();

	bool IsCursorVisible();
	void RenderStatusBar();
	void DeleteCharacter(EditorState& cursor,int cidx=-1);
	void ResetState();
	void InitFileExtensions();
	std::map<std::string, std::string> FileExtensions;


  public:
	bool reCalculateBounds = true;
	bool isFileSaving=false;
	float maxLineWidth{0.0f}; //max horizontal scroll;
	std::string fileType;
	void SetBuffer(const std::string& buffer);


	std::string GetCurrentFilePath()const{return mFilePath;};
	void Render();
	bool Draw();
	void LoadFile(const char* filepath);
	void setLineSpacing(float value) { this->mLineSpacing = value; }
	std::string GetFileType();
	void HandleKeyboardInputs();
	void HandleMouseInputs();
	void UpdateBounds();
	bool IsReadOnly() const { return mReadOnly; }
	void InsertCharacter(char newChar);
	void Backspace();
	void InsertLine();
	int GetSelectionMode()const{return (int)mSelectionMode;};
	void ScrollToLineNumber(int lineNo,bool animate=true);
	void RecalculateBounds(){this->reCalculateBounds=true;}

	inline uint8_t GetTabWidth() { return this->mTabWidth; }


	size_t GetCurrentLineLength(int currLineIndex = -1);
	Editor::EditorState* GetEditorState(){return &mState; }


	Editor();
	~Editor();
};