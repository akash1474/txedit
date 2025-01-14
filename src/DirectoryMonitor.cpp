#include "pch.h"
#include <chrono>
#include <thread>
#include <winnt.h>
#include "DirectoryMonitor.h"

DirectoryMonitor::DirectoryMonitor(){}

DirectoryMonitor::~DirectoryMonitor() 
{
    Stop();
}

void DirectoryMonitor::Start() 
{
    mMonitoringThread = std::thread(&DirectoryMonitor::Monitor, this);
}

void DirectoryMonitor::Stop() 
{
    mStopMonitoring = true;
    if (mMonitoringThread.joinable()) 
    {
        mMonitoringThread.join();
    }
}

void DirectoryMonitor::AddDirectoryToWatchList(const std::wstring& aDirectoryPath){
    
    if(
        std::find_if(mWatchList.begin(),mWatchList.end(),
        [&](const DirectoryWatch& aWatch)
        {
            return aWatch.mDirectoryPath==aDirectoryPath;
        })!=mWatchList.end())
    {
        return;
    }

    HANDLE hDir = CreateFileW(
        aDirectoryPath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (hDir == INVALID_HANDLE_VALUE) {
        GL_CRITICAL("Failed to open directory. Error: {}",GetLastError());
        return;
    }

    GL_INFO("DirectoryMonitor::Added {}",ToUTF8(aDirectoryPath));
    mWatchList.emplace_back(hDir,aDirectoryPath);
}


void DirectoryMonitor::Monitor() 
{

    char buffer[1024];
    OVERLAPPED overlapped = {};
    HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    overlapped.hEvent = hEvent;

    while (true) {
        for(auto& aDirWatch:mWatchList){
            ResetEvent(hEvent);
            MonitorDirectory(hEvent,aDirWatch,overlapped,buffer);
        }
    }

    CloseHandle(hEvent);
    for(auto& aDirWatch:mWatchList)
        CloseHandle(aDirWatch.mHandle);
}

void DirectoryMonitor::MonitorDirectory(HANDLE hEvent,DirectoryWatch& aDirWatch,OVERLAPPED overlapped, char* buffer){
        DWORD bytesReturned;
        if (!ReadDirectoryChangesW(
                aDirWatch.mHandle,
                buffer,
                sizeof(buffer),
                TRUE, // Monitor the entire directory tree
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE,
                &bytesReturned,
                &overlapped,
                nullptr)) {
            std::cerr << "Failed to read directory changes. Error: " << GetLastError() << "\n";
            return;
        }

        // Wait for the event to be signaled (non-blocking via a timeout)
        DWORD waitStatus = WaitForSingleObject(hEvent, 1000); // 1-second timeout
        if (waitStatus == WAIT_OBJECT_0) {
            FILE_NOTIFY_INFORMATION* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);

            do {
                std::wstring fileName(info->FileName, info->FileNameLength / sizeof(WCHAR));
                switch (info->Action) {
                case FILE_ACTION_MODIFIED:
                    std::wcout << L"File modified: " << fileName << "\n";
                    break;
                case FILE_ACTION_ADDED:
                    std::wcout << L"File added: " << fileName << "\n";
                    break;
                case FILE_ACTION_REMOVED:
                    std::wcout << L"File deleted: " << fileName << "\n";
                    break;
                case FILE_ACTION_RENAMED_OLD_NAME:
                    std::wcout << L"File renamed (old name): " << fileName << "\n";
                    break;
                case FILE_ACTION_RENAMED_NEW_NAME:
                    std::wcout << L"File renamed (new name): " << fileName << "\n";
                    break;
                }

                if (info->NextEntryOffset == 0) {
                    break;
                }

                info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                    reinterpret_cast<BYTE*>(info) + info->NextEntryOffset);

            } while (true);
        } else if (waitStatus == WAIT_TIMEOUT) {
            std::wcout << L"No changes detected in the past second.\n";
        } else {
            std::cerr << "Failed to wait for directory change. Error: " << GetLastError() << "\n";
            return;
        }

}