#include "string"
#include "vector"
#include "mutex"
#include "Windows.h"
#include <thread>

class Terminal {
public:
    Terminal();
    ~Terminal();

    void Render();

private:
    char mCommandBuffer[512];    // Buffer to hold input commands
    std::vector<std::string> mOutputBuffer; // Stores output of commands
    std::mutex mOutputMutex;          // To synchronize output access
    bool mIsCommandRunning;           // Flag to indicate a command is running
    bool mScrollToBottom;             // Flag to auto-scroll output
    std::string mCurrentDirectory;    // Store the current directory path
    std::string mBuffer;
    std::thread mThread;
    bool mTerminalThread;
    // Shell process variables
    PROCESS_INFORMATION mProcessInfo;
    HANDLE mHStdInWrite, mHStdOutRead;


    std::string GetCurrentDirectory();

    void StartShell();
    void CloseShell();
    void ShellReader();

    void CheckWriteFileErrors();
    void CheckReadFileErrors();

    // Function to send a command to the shell
    void RunCommand();
};