#include "pch.h"
#include "Log.h"
#include "ConPTY.h"
#include <consoleapi2.h>  // For ConPTY APIs
#include <processthreadsapi.h>
#include <ioapiset.h>



ConPTY::ConPTY(){
    ZeroMemory(&mProcessInfo, sizeof(PROCESS_INFORMATION));
}

ConPTY::~ConPTY(){
    CloseShell();
}


bool ConPTY::Initialize(){
	GL_WARN("ConPTY::Initialize()");

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    // Create the pipes for input/output
    if (!CreatePipe(&mHStdOutRead, &mHStdOutWrite, &sa, 0)) {
    	GL_CRITICAL("ConPTY::Failed to create output Pipe");
        return false;
    }

    if (!CreatePipe(&mHStdInRead, &mHStdInWrite, &sa, 0)) {
    	GL_CRITICAL("ConPTY::Failed to create input Pipe");
        return false;
    }

    COORD consoleSize = {100, 250};
    HRESULT hr = CreatePseudoConsole(consoleSize, mHStdInRead, mHStdOutWrite, 0, &hPC);
    if (FAILED(hr)) {
    	GL_CRITICAL("ConPTY::SudoConsole Creation Error!");
        return false;
    }

    return true;
}


bool ConPTY::CreateAndStartProcess(const std::wstring& cmd){
    GL_WARN("ConPTY::CreateAndStartProcess()");
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
    ) GL_CRITICAL("Failed: UpdateProcThreadAttribute");

    // Create the process (cmd.exe in this case)
    BOOL result = CreateProcessW(
        nullptr, (wchar_t*)cmd.data(), nullptr, nullptr, FALSE,
        EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr,
        &startupInfo.StartupInfo, &mProcessInfo
    );

    if (!result) {
    	GL_CRITICAL("ConPTY::Failed to CreateProcessW");
        return false;
    }

    // Clean up
    CloseHandle(mProcessInfo.hProcess);
    CloseHandle(mProcessInfo.hThread);

	return true;
}

std::string ConPTY::ReadOutput(){
	mBytesRead=0;
    BOOL rResult = ReadFile(mHStdOutRead, mReadBuffer, sizeof(mReadBuffer) - sizeof(char), &mBytesRead,0);
    if (!rResult) {
        CheckReadFileErrors();
        CloseShell();
    }
    if (mBytesRead > 0) {
        GL_INFO("ConPTY::BytesRead:{}",mBytesRead);
        mReadBuffer[mBytesRead/sizeof(char)] = '\0';
         return std::string(mReadBuffer);
     }
     return "";
}

void ConPTY::WriteInput(const std::string& command){
    DWORD bytesWritten;
    if(!WriteFile(mHStdInWrite, command.c_str(),command.size(), &bytesWritten, NULL)){
    	GL_ERROR("Terminal::FAILED TO WRITE CMD");
    	CheckWriteFileErrors();
    	CloseShell();
    }
    GL_INFO("Terminal::BytesWritten:{}",bytesWritten);
}


void ConPTY::CloseShell(){
	if (mProcessInfo.hProcess) {
        TerminateProcess(mProcessInfo.hProcess, 0);
        CloseHandle(mProcessInfo.hProcess);
        CloseHandle(mProcessInfo.hThread);
    }
    if (mHStdInWrite) CloseHandle(mHStdInWrite);
    if (mHStdOutRead) CloseHandle(mHStdOutRead);
    if (hPC) ClosePseudoConsole(hPC);
}



void ConPTY::SendInterrupt(){
    GL_WARN("ConPTY::Interrupt Signal Sent");
    WriteInput("\x03");
}


void ConPTY::CheckWriteFileErrors() {
	DWORD error = GetLastError();

	switch (error) {
	    case ERROR_ACCESS_DENIED:
	        std::cerr << "ConPTY::ErrorWrite: Access denied." << std::endl;
	        break;
	    case ERROR_BROKEN_PIPE:
	        std::cerr << "ConPTY::ErrorWrite: The pipe has been ended (broken pipe)." << std::endl;
	        break;
	    case ERROR_HANDLE_EOF:
	        std::cerr << "ConPTY::ErrorWrite: Reached the end of the file." << std::endl;
	        break;
	    case ERROR_INVALID_HANDLE:
	        std::cerr << "ConPTY::ErrorWrite: Invalid handle." << std::endl;
	        break;
	    case ERROR_IO_PENDING:
	        std::cerr << "ConPTY::ErrorWrite: I/O operation is in progress." << std::endl;
	        break;
	    case ERROR_NO_DATA:
	        std::cerr << "ConPTY::ErrorWrite: The pipe is being closed." << std::endl;
	        break;
	    case ERROR_NOT_ENOUGH_MEMORY:
	        std::cerr << "ConPTY::ErrorWrite: Not enough memory to complete the operation." << std::endl;
	        break;
	    case ERROR_OPERATION_ABORTED:
	        std::cerr << "ConPTY::ErrorWrite: The I/O operation has been aborted." << std::endl;
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

	        std::cerr << "ConPTY::ErrorWrite: " << (char*)lpMsgBuf << std::endl;
	        LocalFree(lpMsgBuf);  // Free the buffer.
	        break;
	}
}
void ConPTY::CheckReadFileErrors() {
    DWORD error = GetLastError();

    switch (error) {
        case ERROR_ACCESS_DENIED:
            std::cerr << "ConPTY::ErrorRead: Access denied." << std::endl;
            break;
        case ERROR_BROKEN_PIPE:
            std::cerr << "ConPTY::ErrorRead: The pipe has been ended (broken pipe)." << std::endl;
            break;
        case ERROR_HANDLE_EOF:
            std::cerr << "ConPTY::ErrorRead: Reached the end of the file." << std::endl;
            break;
        case ERROR_INVALID_HANDLE:
            std::cerr << "ConPTY::ErrorRead: Invalid handle." << std::endl;
            break;
        case ERROR_IO_PENDING:
            std::cerr << "ConPTY::ErrorRead: I/O operation is in progress." << std::endl;
            break;
        case ERROR_MORE_DATA:
            std::cerr << "ConPTY::ErrorRead: More data is available than the buffer can hold." << std::endl;
            break;
        case ERROR_NO_DATA:
            std::cerr << "ConPTY::ErrorRead: The pipe is being closed." << std::endl;
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            std::cerr << "ConPTY::ErrorRead: Not enough memory to complete the operation." << std::endl;
            break;
        case ERROR_OPERATION_ABORTED:
            std::cerr << "ConPTY::ErrorRead: The I/O operation has been aborted." << std::endl;
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

            std::cerr << "ConPTY::ErrorRead: " << (char*)lpMsgBuf << std::endl;
            LocalFree(lpMsgBuf);  // Free the buffer.
            break;
    }
}



