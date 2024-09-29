#include <windows.h>
#include <iostream>
#include <string>
#include <winerror.h>
#include "Winbase.h"
#include "list"
#include "unordered_map"
#include "unordered_set"

struct Node {
    int count;
    std::unordered_set<std::string> keys;
};

class AllOne {
public:
    void inc(std::string key) {
        if (const auto it = keyToIterator.find(key); it == keyToIterator.end())
            addNewKey(key);
        else
            incrementExistingKey(it, key);
    }

    void dec(std::string key) {
        const auto it = keyToIterator.find(key);
        decrementExistingKey(it, key);
    }

    std::string getMaxKey() {
        return nodeList.empty() ? "" : *nodeList.back().keys.begin();
    }

    std::string getMinKey() {
        return nodeList.empty() ? "" : *nodeList.front().keys.begin();
    }

private:
    std::list<Node> nodeList; // list of nodes sorted by count
    std::unordered_map<std::string, std::list<Node>::iterator> keyToIterator;

    void addNewKey(const std::string& key) {
        if (nodeList.empty() || nodeList.front().count > 1)
            nodeList.push_front({1, {key}});
        else // nodeList.front().count == 1
            nodeList.front().keys.insert(key);
        keyToIterator[key] = nodeList.begin();
    }

    // Increments the count of the key by 1.
    void incrementExistingKey(
        std::unordered_map<std::string, std::list<Node>::iterator>::iterator it,const std::string& key) {
        const auto listIt = it->second;

        auto nextIt = next(listIt);
        const int newCount = listIt->count + 1;
        if (nextIt == nodeList.end() || nextIt->count > newCount)
            nextIt = nodeList.insert(nextIt, {newCount, {key}});
        else // Node with count + 1 exists.
            nextIt->keys.insert(key);

        keyToIterator[key] = nextIt;
        remove(listIt, key);
    }

    // Decrements the count of the key by 1.
    void decrementExistingKey(
        std::unordered_map<std::string, std::list<Node>::iterator>::iterator it,
        const std::string& key) {
        const auto listIt = it->second;
        if (listIt->count == 1) {
            keyToIterator.erase(it);
            remove(listIt, key);
            return;
        }

        auto prevIt = prev(listIt);
        const int newCount = listIt->count - 1;
        if (listIt == nodeList.begin() || prevIt->count < newCount)
            prevIt = nodeList.insert(listIt, {newCount, {key}});
        else // Node with count - 1 exists.
            prevIt->keys.insert(key);

        keyToIterator[key] = prevIt;
        remove(listIt, key);
    }

    // Removes the key from the node list.
    void remove(std::list<Node>::iterator it, const std::string& key) {
        it->keys.erase(key);
        if (it->keys.empty()) nodeList.erase(it);
    }
};


void printError(std::wstring type=L""){
    DWORD error = GetLastError();
    if (error != 0) {
        LPVOID lpMsgBuf;
        FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPWSTR)&lpMsgBuf,
            0,
            NULL
        );
        std::wcout << type << L"Error: " << (LPWSTR)lpMsgBuf << std::endl;
        LocalFree(lpMsgBuf); // Free the buffer allocated by FormatMessage
    }
}

void CheckWriteFileErrors() {
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
            // If the error code isn't handled explicitly, print it out
            LPVOID lpMsgBuf;
            DWORD dw = GetLastError();
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&lpMsgBuf,
                0, NULL);

            std::cerr << "ErrorWrite: " << (char*)lpMsgBuf << std::endl;
            LocalFree(lpMsgBuf);  // Free the buffer.
            break;
    }
}

void CheckReadFileErrors() {
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
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&lpMsgBuf,
                0, NULL);

            std::cerr << "ErrorRead: " << (char*)lpMsgBuf << std::endl;
            LocalFree(lpMsgBuf);  // Free the buffer.
            break;
    }
}

bool CheckPipe(HANDLE hPipe) {
    DWORD bytesAvailable = 0;
    BOOL result = PeekNamedPipe(hPipe, NULL, 0, NULL, &bytesAvailable, NULL);
    std::cout << "ReadAvaliable:"<< bytesAvailable << std::endl;

    return bytesAvailable>0;
}



int main() {
    // Handles for pipes
    HANDLE hStdoutRead = NULL, hStdoutWrite = NULL;
    HANDLE hStdinRead = NULL, hStdinWrite = NULL;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;  // Pipe handles should be inheritable
    sa.lpSecurityDescriptor = NULL;

    // Create pipes for the child process's STDOUT
    if (!CreatePipe(&hStdoutRead, &hStdoutWrite, &sa, 0)) {
        std::cerr << "Error creating stdout pipe.\n";
        return 1;
    }

    // Create pipes for the child process's STDIN
    if (!CreatePipe(&hStdinRead, &hStdinWrite, &sa, 0)) {
        std::cerr << "Error creating stdin pipe.\n";
        return 1;
    }

    // Ensure the write handle to the child process's STDIN is not inherited
    SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0);

    // Create the child process (cmd.exe)
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = hStdoutWrite;
    si.hStdOutput = hStdoutWrite;
    si.hStdInput = hStdinRead;
    si.dwFlags |= STARTF_USESTDHANDLES;  // Use pipes for stdin, stdout, stderr

    // Start cmd.exe process
    if (!CreateProcess(NULL, (LPSTR)"cmd.exe", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "Error creating process.\n";
        return 1;
    }

    // Close unneeded handles
    CloseHandle(hStdoutWrite);  // No need to write to stdout pipe in parent
    CloseHandle(hStdinRead);    // No need to read from stdin pipe in parent

    // Buffer for reading and writing
    std::string command;
    DWORD bytesWritten, bytesRead;
    char buffer[4096];
    bool processCommand = true;

    OVERLAPPED writeOverlapped = {};
    writeOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    OVERLAPPED readOverlapped = {};
    readOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // Infinite loop to keep interacting with the child process
    while (true) {
        while(CheckPipe(hStdoutRead)){
            DWORD rResult = ReadFile(hStdoutRead, buffer, sizeof(buffer) - 1, &bytesRead,NULL);
            std::cout << "bytesRead: " << bytesRead << std::endl;

            if(!rResult) CheckReadFileErrors();
            
            if (rResult && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::cout << buffer << std::endl;
            }
        }

        DWORD exitCode;
        if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
            std::cerr << "Error getting the exit code of the process.\n";
        } else if (exitCode != STILL_ACTIVE) {
            std::cerr << "Child process has exited.\n";
            break;  // Exit the loop if the child process has terminated
        }

        // if(bytesRead==0){
            std::cout << "Enter command (or 'exit' to quit): ";
            std::getline(std::cin, command);
            command += "\n";

            BOOL wResult = WriteFile(hStdinWrite, command.c_str(), command.size(), &bytesWritten, &writeOverlapped);
            if(!wResult) CheckWriteFileErrors();
        // }
    }

    // Close handles and clean up after process has exited
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdinWrite);
    CloseHandle(hStdoutRead);
    CloseHandle(writeOverlapped.hEvent);
    CloseHandle(readOverlapped.hEvent);

    return 0;
}


// void Terminal::ShellReader() {
//     char buffer[256];
//     DWORD bytesRead;
//     OVERLAPPED readOverlapped = {};
//     readOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//     while (!mTerminalThread) {
//         GL_INFO("RUNNING");
//         BOOL rResult=ReadFile(mHStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, &readOverlapped);
//         if(!rResult){
//             if (GetLastError() == ERROR_IO_PENDING) {
//                 WaitForSingleObject(&readOverlapped.hEvent, INFINITE);
//                 GetOverlappedResult(mHStdOutRead, &readOverlapped, &bytesRead, FALSE);
//             }else{
//                 CheckReadFileErrors();
//                 CloseShell();
//                 StartShell();
//                 break;
//             }
//         }
//         buffer[bytesRead] = '\0';
//         std::lock_guard<std::mutex> lock(mOutputMutex);
//         mOutputBuffer.push_back(std::string(buffer));
//         mScrollToBottom = true;
//     }
// }