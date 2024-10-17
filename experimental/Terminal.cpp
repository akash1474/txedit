#include "DataTypes.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "pch.h"
#include <handleapi.h>
#include <ioapiset.h>
#include <processthreadsapi.h>
#include "Terminal.h"
#include <processthreadsapi.h>
#include <consoleapi2.h>  // For ConPTY APIs
#include "regex"


// TODO:
// Separate read output to different lines and implement selection copy + rightclick copy

Terminal::Terminal() : mIsCommandRunning(false), mScrollToBottom(false),mTerminalThread(false) {
    mCurrentDirectory = GetCurrentDirectory();
    ZeroMemory(&mProcessInfo, sizeof(PROCESS_INFORMATION));
    StartShell();
}

Terminal::~Terminal() {
    CloseShell();
}

std::string RemoveANSISequences(const std::string& str) {
    // Regular expression to match ANSI escape sequences
    std::regex ansiRegex(R"(\x1B\[[0-9;]*[a-zA-Z]|\x1B\].*?\x07)");

    // Replace matched sequences with an empty string
    return std::regex_replace(str, ansiRegex, "");
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
    if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)){
        if(ImGui::IsKeyPressed(ImGuiKey_ModCtrl,true) && ImGui::IsKeyPressed(ImGuiKey_C,true)) this->SendInterrupt();
    }

    ImGui::End();
}

std::string Terminal::GetCurrentDirectory() {
    char buffer[MAX_PATH];
    ::GetCurrentDirectoryA(MAX_PATH, buffer);
    return std::string(buffer);
}

void Terminal::StartShell() {
	GL_WARN("Terminal::StartShell()");

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    // Create the pipes for input/output
    if (!CreatePipe(&mHStdOutRead, &mHStdOutWrite, &sa, 0)) {
        std::cerr << "Failed to create output pipe.\n";
        return;
    }

    if (!CreatePipe(&mHStdInRead, &mHStdInWrite, &sa, 0)) {
        std::cerr << "Failed to create input pipe.\n";
        return;
    }

    // Create the Pseudo Console
    COORD consoleSize = {80, 25};
    HRESULT hr = CreatePseudoConsole(consoleSize, mHStdInRead, mHStdOutWrite, 0, &hPC);
    if (FAILED(hr)) {
        std::cerr << "Failed to create pseudo console. Error: " << hr << std::endl;
        return;
    }

        STARTUPINFOEXW startupInfo{sizeof(STARTUPINFOEXW)};
        SIZE_T bytesRequired = 0;
        if (InitializeProcThreadAttributeList(nullptr, 1, 0, &bytesRequired))
            throw std::runtime_error("InitializeProcThreadAttributeList wasn't expected to succeed at that time.");

        const auto threadAttributeList = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(calloc(bytesRequired, 1));
        startupInfo.lpAttributeList = threadAttributeList;
        startupInfo.StartupInfo.dwFlags=STARTF_USESTDHANDLES;
        
        if (!InitializeProcThreadAttributeList(threadAttributeList, 1, 0, &bytesRequired)) GL_CRITICAL("Failed");

        if (!UpdateProcThreadAttribute(
                threadAttributeList,
                0,
                PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                hPC,
                sizeof(HPCON),
                nullptr,
                nullptr)
        )GL_CRITICAL("Failed: UpdateProcThreadAttribute");

        // Create the process (cmd.exe in this case)
        wchar_t commandLine[] = L"cmd.exe /k chcp 65001";
        BOOL result = CreateProcessW(
            nullptr, commandLine, nullptr, nullptr, FALSE,
            EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr,
            &startupInfo.StartupInfo, &mProcessInfo
        );

        if (!result) {
            std::cerr << "Failed to create process.\n";
            return;
        }

        // Clean up
        CloseHandle(mProcessInfo.hProcess);
        CloseHandle(mProcessInfo.hThread);

    // Start a thread to read the shell output continuously
    std::thread([this]() { ShellReader(); }).detach();
}

// Read from the shell continuously and update the output
void Terminal::ShellReader() {
    char buffer[4096];
    DWORD bytesRead;


    while (!mTerminalThread) {
        BOOL rResult = ReadFile(mHStdOutRead, buffer, sizeof(buffer) - sizeof(char), &bytesRead,0);
        if (!rResult) {
            CheckReadFileErrors();
            CloseShell();
            StartShell();
            break;
        }
        if (bytesRead > 0) {
            buffer[bytesRead/sizeof(char)] = '\0';
            // std::string utf8Output = ToUTF8(buffer);
            
            std::lock_guard<std::mutex> lock(mOutputMutex);
            mOutputBuffer.push_back(std::string("\n»  ")+buffer);
            mScrollToBottom = true;
        }
    }
}


bool Terminal::SendInterrupt(){
    GL_WARN("Interrupt");
    DWORD processId=mProcessInfo.dwProcessId;
    if (processId == 0) {
        std::cerr << "No process is currently running." << std::endl;
        return false;
    }

    // Attach to the console of the process
    if (!AttachConsole(processId)) {
        std::cerr << "Failed to attach to console. Error: " << GetLastError() << std::endl;
        return false;
    }

    // Generate a Ctrl+C event
    bool result = GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);

    // Detach from the console
    FreeConsole();

    if (!result) {
        std::cerr << "Failed to send Ctrl+C event. Error: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}


void Terminal::CloseShell() {
    mTerminalThread=true;
    // if(mThread.joinable()) mThread.join();

    if (mProcessInfo.hProcess) {
        TerminateProcess(mProcessInfo.hProcess, 0);
        CloseHandle(mProcessInfo.hProcess);
        CloseHandle(mProcessInfo.hThread);
    }
    if (mHStdInWrite) CloseHandle(mHStdInWrite);
    if (mHStdOutRead) CloseHandle(mHStdOutRead);
    if (hPC) ClosePseudoConsole(hPC);
}

void Terminal::RunCommand() {
    if (mIsCommandRunning) return;  // Prevent executing multiple commands simultaneously

    std::string command = mCommandBuffer;
    if (command.empty()) return;

    mIsCommandRunning = true;
    std::fill(std::begin(mCommandBuffer), std::end(mCommandBuffer), 0);  // Clear command buffer


    // Send the command to cmd.exe
    GL_WARN("Terminal::CMD:{}",command.c_str());
    command += "\r\n";
    // std::wstring wCommand=L"echo »\n";
    DWORD bytesWritten;
    if(!WriteFile(mHStdInWrite, command.c_str(),command.size(), &bytesWritten, NULL)){
    	GL_ERROR("Terminal::FAILED TO WRITE CMD");
    	CheckWriteFileErrors();
    	CloseShell();
    	// StartShell();
    }
    GL_INFO("Terminal::BytesWritten:{}",bytesWritten);

    mIsCommandRunning=false;
    mScrollToBottom = true;
}

void Terminal::CheckWriteFileErrors() {
	DWORD error = GetLastError();

	switch (error) {
	    case ERROR_ACCESS_DENIED:
	        std::cerr << "Terminal::ErrorWrite: Access denied." << std::endl;
	        break;
	    case ERROR_BROKEN_PIPE:
	        std::cerr << "Terminal::ErrorWrite: The pipe has been ended (broken pipe)." << std::endl;
	        break;
	    case ERROR_HANDLE_EOF:
	        std::cerr << "Terminal::ErrorWrite: Reached the end of the file." << std::endl;
	        break;
	    case ERROR_INVALID_HANDLE:
	        std::cerr << "Terminal::ErrorWrite: Invalid handle." << std::endl;
	        break;
	    case ERROR_IO_PENDING:
	        std::cerr << "Terminal::ErrorWrite: I/O operation is in progress." << std::endl;
	        break;
	    case ERROR_NO_DATA:
	        std::cerr << "Terminal::ErrorWrite: The pipe is being closed." << std::endl;
	        break;
	    case ERROR_NOT_ENOUGH_MEMORY:
	        std::cerr << "Terminal::ErrorWrite: Not enough memory to complete the operation." << std::endl;
	        break;
	    case ERROR_OPERATION_ABORTED:
	        std::cerr << "Terminal::ErrorWrite: The I/O operation has been aborted." << std::endl;
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

	        std::cerr << "Terminal::ErrorWrite: " << (char*)lpMsgBuf << std::endl;
	        LocalFree(lpMsgBuf);  // Free the buffer.
	        break;
	}
}
void Terminal::CheckReadFileErrors() {
    DWORD error = GetLastError();

    switch (error) {
        case ERROR_ACCESS_DENIED:
            std::cerr << "Terminal::ErrorRead: Access denied." << std::endl;
            break;
        case ERROR_BROKEN_PIPE:
            std::cerr << "Terminal::ErrorRead: The pipe has been ended (broken pipe)." << std::endl;
            break;
        case ERROR_HANDLE_EOF:
            std::cerr << "Terminal::ErrorRead: Reached the end of the file." << std::endl;
            break;
        case ERROR_INVALID_HANDLE:
            std::cerr << "Terminal::ErrorRead: Invalid handle." << std::endl;
            break;
        case ERROR_IO_PENDING:
            std::cerr << "Terminal::ErrorRead: I/O operation is in progress." << std::endl;
            break;
        case ERROR_MORE_DATA:
            std::cerr << "Terminal::ErrorRead: More data is available than the buffer can hold." << std::endl;
            break;
        case ERROR_NO_DATA:
            std::cerr << "Terminal::ErrorRead: The pipe is being closed." << std::endl;
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            std::cerr << "Terminal::ErrorRead: Not enough memory to complete the operation." << std::endl;
            break;
        case ERROR_OPERATION_ABORTED:
            std::cerr << "Terminal::ErrorRead: The I/O operation has been aborted." << std::endl;
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

            std::cerr << "Terminal::ErrorRead: " << (char*)lpMsgBuf << std::endl;
            LocalFree(lpMsgBuf);  // Free the buffer.
            break;
    }
}
