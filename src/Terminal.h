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
    std::string mBuffer,mDisplayBuffer;
    std::thread mThread;
    bool mTerminalThread;
    ConPTY mConPTY;
    size_t mPrevBufferSize=0;
    double mPrevTime=0;
    // Shell process variables


    void StartShell();
    void CloseShell();
    void ShellReader();

    // Function to send a command to the shell
    void RunCommand(std::string& command);
};