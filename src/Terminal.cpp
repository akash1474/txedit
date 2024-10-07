#include "DataTypes.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "pch.h"
#include <handleapi.h>
#include <ioapiset.h>
#include "Terminal.h"


// TODO:
// Separate read output to different lines and implement selection copy + rightclick copy

Terminal::Terminal() : mIsCommandRunning(false), mScrollToBottom(false),mTerminalThread(false) {
    mCurrentDirectory = GetCurrentDirectory();
    StartShell();
}

Terminal::~Terminal() {
    CloseShell();
}

void Terminal::Render() {
    ImGui::Begin("Terminal");
    static size_t prevSize=0;
    static int bufferUpdated=0; //using a bool doesn't do the job to scroll to bottom
    if(mOutputBuffer.size()!=prevSize){
        {
            std::lock_guard<std::mutex> lock(mOutputMutex);
            for(int i=prevSize;i<mOutputBuffer.size();i++) 
                mBuffer+=mOutputBuffer[i];
        }
        prevSize=mOutputBuffer.size();
        bufferUpdated=1;
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
    ImGui::InputTextMultiline("##toutput", (char*)mBuffer.c_str(), mBuffer.size(),{-1,-ImGui::GetFrameHeightWithSpacing()*1.2f},ImGuiInputTextFlags_ReadOnly|ImGuiInputTextFlags_HideCursor);
    ImGui::PopStyleColor();
    static bool isFocused=false;
    if(ImGui::IsItemHovered() && !ImGui::IsMouseDragging(0) && ImGui::IsMouseReleased(0)) isFocused=true;
    else isFocused=false;
    
    if(bufferUpdated){
        ImGuiContext& g = *GImGui;
        static const char* child_window_name = NULL;
        ImFormatStringToTempBuffer(&child_window_name, NULL, "%s/%s_%08X", g.CurrentWindow->Name, "##toutput", ImGui::GetID("##toutput"));
        ImGuiWindow* child_window = ImGui::FindWindowByName(child_window_name);
        ImGui::SetScrollY(child_window, child_window->ScrollMax.y);
        if(bufferUpdated>4) bufferUpdated=0;
        else bufferUpdated++;
    }

    ImGui::TextColored({0.196f,0.808f,0.659f,1.0f},"Command:>>"); ImGui::SameLine();
    if (ImGui::InputTextMultiline("##Command", mCommandBuffer, IM_ARRAYSIZE(mCommandBuffer),{-1,-1}, ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CtrlEnterForNewLine)) {
        RunCommand();
    }
    if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)  && isFocused){
    	ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

std::string Terminal::GetCurrentDirectory() {
    char buffer[MAX_PATH];
    ::GetCurrentDirectoryA(MAX_PATH, buffer);
    return std::string(buffer);
}

void Terminal::StartShell() {
	GL_WARN("STARTING SHELL");

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hStdOutWrite;
    HANDLE hStdInRead;

    // Create pipes for standard input and output
    CreatePipe(&mHStdOutRead, &hStdOutWrite, &saAttr, 0);
    SetHandleInformation(mHStdOutRead, HANDLE_FLAG_INHERIT, 0);
    CreatePipe(&hStdInRead, &mHStdInWrite, &saAttr, 0);
    SetHandleInformation(mHStdInWrite, HANDLE_FLAG_INHERIT, 0);

    // Set up the process information for cmd.exe
    STARTUPINFOA startInfo;
    ZeroMemory(&startInfo, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);
    startInfo.hStdError = hStdOutWrite;
    startInfo.hStdOutput = hStdOutWrite;
    startInfo.hStdInput = hStdInRead;
    startInfo.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&mProcessInfo, sizeof(mProcessInfo));

    // Create the cmd.exe process
    if (!CreateProcessA(
        NULL,
        (LPSTR)"cmd.exe",  // Command line
        NULL,           // Process security attributes
        NULL,           // Primary thread security attributes
        TRUE,           // Handles are inherited
        0,              // Creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &startInfo,     // Pointer to STARTUPINFO structure
        &mProcessInfo))     // Pointer to PROCESS_INFORMATION structure
    {
        std::cerr << "Error: Unable to create shell process." << std::endl;
        return;
    }

    CloseHandle(hStdOutWrite);
    CloseHandle(hStdInRead);

    // Start a thread to read the shell output continuously
    std::thread([this]() { ShellReader(); }).detach();
}

// Read from the shell continuously and update the output
void Terminal::ShellReader() {
    char buffer[4096];
    DWORD bytesRead;
    OVERLAPPED readOverlapped = {};
    readOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    while (!mTerminalThread) {
        GL_INFO("RUNNING");
    	BOOL rResult=ReadFile(mHStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, &readOverlapped);
    	if(!rResult){
            if (GetLastError() == ERROR_IO_PENDING) {
                WaitForSingleObject(&readOverlapped.hEvent, INFINITE);
                GetOverlappedResult(mHStdOutRead, &readOverlapped, &bytesRead, FALSE);
            }else{
                CheckReadFileErrors();
                CloseShell();
                StartShell();
                break;
            }
    	}
        buffer[bytesRead] = '\0';
        std::lock_guard<std::mutex> lock(mOutputMutex);
        mOutputBuffer.push_back(buffer);
        // bool isFirst=false;
        // std::string temp;
        // static bool isFinished=false;
        // for(int i=0;i<bytesRead;i++){
        //     if(buffer[i]=='\r'){
        //         isFinished=true;
        //         continue;
        //     }
        //     if(buffer[i]=='\n'){
        //         if(temp.empty()) continue;
        //         if(isFirst){
        //             if(!mOutputBuffer.empty()) mOutputBuffer.back()+=temp;
        //             else mOutputBuffer.push_back(temp);
        //             isFirst=false;
        //             isFinished=true;
        //         }
        //         else{
        //             mOutputBuffer.push_back(temp);
        //             isFinished=false;
        //         }
        //         temp.clear();
        //     }else temp+=buffer[i];
        // }
        // if(temp.size()){
        //     if(!isFinished && !mOutputBuffer.empty()) mOutputBuffer.back()+=temp;
        //     else mOutputBuffer.push_back(temp);
        //     isFinished=true;
        //     temp.clear();
        // }
        mScrollToBottom = true;
    }
}


void Terminal::CloseShell() {
    mTerminalThread=true;
    // if(mThread.joinable()) mThread.join();

    CloseHandle(mHStdInWrite);
    mHStdInWrite = INVALID_HANDLE_VALUE;

    CloseHandle(mHStdOutRead);
    mHStdOutRead=INVALID_HANDLE_VALUE;

    TerminateProcess(mProcessInfo.hProcess, 0);

    CloseHandle(mProcessInfo.hProcess);
    mProcessInfo.hProcess=INVALID_HANDLE_VALUE;

    CloseHandle(mProcessInfo.hThread);
    mProcessInfo.hThread=INVALID_HANDLE_VALUE;
}

void Terminal::RunCommand() {
    if (mIsCommandRunning) return;  // Prevent executing multiple commands simultaneously

    std::string command = mCommandBuffer;
    if (command.empty()) return;

    mIsCommandRunning = true;
    std::fill(std::begin(mCommandBuffer), std::end(mCommandBuffer), 0);  // Clear command buffer


    // Send the command to cmd.exe
    GL_WARN("CMD:{}",command.c_str());
    command += "\n";
    DWORD bytesWritten;
    if(!WriteFile(mHStdInWrite, command.c_str(), command.size(), &bytesWritten, NULL)){
    	GL_ERROR("FAILED TO WRITE CMD");
    	CheckWriteFileErrors();
    	CloseShell();
    	// StartShell();
    }

    mIsCommandRunning=false;
    mScrollToBottom = true;
}

void Terminal::CheckWriteFileErrors() {
	DWORD error = GetLastError();

	switch (error) {
	    case ERROR_ACCESS_DENIED:
	        std::cerr << "ErrorWrite: Access denied." << std::endl;
	        break;
	    case ERROR_BROKEN_PIPE:
	        std::cerr << "ErrorWrite: The pipe has been ended (broken pipe)." << std::endl;
	        break;
	    case ERROR_HANDLE_EOF:
	        std::cerr << "ErrorWrite: Reached the end of the file." << std::endl;
	        break;
	    case ERROR_INVALID_HANDLE:
	        std::cerr << "ErrorWrite: Invalid handle." << std::endl;
	        break;
	    case ERROR_IO_PENDING:
	        std::cerr << "ErrorWrite: I/O operation is in progress." << std::endl;
	        break;
	    case ERROR_NO_DATA:
	        std::cerr << "ErrorWrite: The pipe is being closed." << std::endl;
	        break;
	    case ERROR_NOT_ENOUGH_MEMORY:
	        std::cerr << "ErrorWrite: Not enough memory to complete the operation." << std::endl;
	        break;
	    case ERROR_OPERATION_ABORTED:
	        std::cerr << "ErrorWrite: The I/O operation has been aborted." << std::endl;
	        break;
	    default:
	        LPVOID lpMsgBuf;
	        DWORD dw = GetLastError();
	        FormatMessageW(
	            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	            NULL,
	            dw,
	            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	            (LPWSTR)&lpMsgBuf,
	            0, NULL);

	        std::cerr << "ErrorWrite: " << (char*)lpMsgBuf << std::endl;
	        LocalFree(lpMsgBuf);  // Free the buffer.
	        break;
	}
}
void Terminal::CheckReadFileErrors() {
    DWORD error = GetLastError();

    switch (error) {
        case ERROR_ACCESS_DENIED:
            std::cerr << "ErrorRead: Access denied." << std::endl;
            break;
        case ERROR_BROKEN_PIPE:
            std::cerr << "ErrorRead: The pipe has been ended (broken pipe)." << std::endl;
            break;
        case ERROR_HANDLE_EOF:
            std::cerr << "ErrorRead: Reached the end of the file." << std::endl;
            break;
        case ERROR_INVALID_HANDLE:
            std::cerr << "ErrorRead: Invalid handle." << std::endl;
            break;
        case ERROR_IO_PENDING:
            std::cerr << "ErrorRead: I/O operation is in progress." << std::endl;
            break;
        case ERROR_MORE_DATA:
            std::cerr << "ErrorRead: More data is available than the buffer can hold." << std::endl;
            break;
        case ERROR_NO_DATA:
            std::cerr << "ErrorRead: The pipe is being closed." << std::endl;
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            std::cerr << "ErrorRead: Not enough memory to complete the operation." << std::endl;
            break;
        case ERROR_OPERATION_ABORTED:
            std::cerr << "ErrorRead: The I/O operation has been aborted." << std::endl;
            break;
        default:
            // If the error code isn't handled explicitly, print it out
            LPVOID lpMsgBuf;
            DWORD dw = GetLastError();
            FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPWSTR)&lpMsgBuf,
                0, NULL);

            std::cerr << "ErrorRead: " << (char*)lpMsgBuf << std::endl;
            LocalFree(lpMsgBuf);  // Free the buffer.
            break;
    }
}
