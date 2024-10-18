#include "Coordinates.h"
#include "DataTypes.h"
#include "imgui_internal.h"
#include "string"
#include "vector"
#include "mutex"
#include "Windows.h"
#include <thread>
#include <wincontypes.h>
#include "ConPTY.h"



class Terminal {
public:
    Terminal();
    ~Terminal();

    void Render();

private:
    std::mutex mOutputMutex;          // To synchronize output access
    bool mIsCommandRunning;           // Flag to indicate a command is running
    bool mScrollToBottom;             // Flag to auto-scroll output
    std::string mBuffer;
    std::thread mThread;
    bool mTerminalThread;
    ConPTY mConPTY;
    double mPrevTime=0;
    


    void StartShell();
    void CloseShell();
    void ShellReader();

    // Function to send a command to the shell
    void RunCommand(std::string& command);


private:
    //Command History
    std::vector<std::string> mHistory;
    size_t mHistoryIdx{0};
    void PushHistory(const std::string& cmd);
    void PushHistoryCommand(bool direction);


private:

    enum class SelectionMode { Normal, Word, Line };
    SelectionMode mSelectionMode{SelectionMode::Normal};

    // Text Renderer
    EditorState mState;
    std::vector<std::string> mLines;
    ImGuiWindow* mWindow=nullptr;
    ImVec2 mPosition;
    ImVec2 mCharacterSize;
    ImVec2 mLinePosition;
    ImRect mBounds;
    ImVec2 mSize;
    float mLastClick;
    float mMinLineVisible;
    float mLineSpacing{5.0f};
    int mLineHeight{0};
    float mTitleBarHeight{0};
    uint8_t mTabWidth{4};
    bool mRecalculateBounds=false;
    Coordinates mReadOnlyCoords;
    std::string mCommand;

    float mPaddingLeft = 5.0f;

    void Draw();
    void UpdateBounds();
    float GetSelectionPosFromCoords(const Coordinates& coords)const;
    size_t GetLineMaxColumn(int currLineIndex)const;
    void AppendText(const std::string& str);
    Coordinates GetCommandInsertPosition();
    int GetCharacterIndex(const Coordinates& aCoordinates) const;
    std::pair<int,int> GetIndexOfWordAtCursor(const Coordinates& coords)const;
    Coordinates MapScreenPosToCoordinates(const ImVec2& mousePosition);
    void HandleInputs();
    void ExitSelectionMode(){mSelectionMode=SelectionMode::Normal;};
    int GetCharacterColumn(int aLine, int aIndex) const;

};