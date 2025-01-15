#include "pch.h"
#include <chrono>
#include <ratio>
#include <thread>
#include <winnt.h>
#include "DirectoryMonitor.h"
#include "FileNavigation.h"

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
    mWatchList.emplace_back(std::move(hDir),aDirectoryPath);
}


void DirectoryMonitor::Monitor() 
{

    char buffer[1024];
    OVERLAPPED overlapped = {};
    HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    overlapped.hEvent = hEvent;

    while (!mStopMonitoring) {
		if (mWatchList.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
        }
        for(auto& aDirWatch:mWatchList){
            ResetEvent(hEvent);
            MonitorDirectory(hEvent,aDirWatch,overlapped,buffer);
        }
    }

    CloseHandle(hEvent);
    for(auto& aDirWatch:mWatchList)
        CloseHandle(aDirWatch.mHandle);
}

void DirectoryMonitor::MonitorDirectory(HANDLE& hEvent,DirectoryWatch& aDirWatch,OVERLAPPED& overlapped, char* buffer){
    DWORD bytesReturned;
    char nBuffer[1024];
    if (!ReadDirectoryChangesW(
            aDirWatch.mHandle,
            nBuffer,
            sizeof(nBuffer),
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
        FILE_NOTIFY_INFORMATION* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(nBuffer);

        do {
            std::wstring modPath(info->FileName, info->FileNameLength / sizeof(WCHAR));
            std::wstring filePath=aDirWatch.mDirectoryPath+L"/"+std::filesystem::path(modPath).generic_wstring();

            switch (info->Action) {
            case FILE_ACTION_MODIFIED:
                FileNavigation::HandleEvent(DirectoryEvent::FileModified,filePath);
                break;
            case FILE_ACTION_ADDED:
                FileNavigation::HandleEvent(DirectoryEvent::FileAdded,filePath);
                break;
            case FILE_ACTION_REMOVED:
                FileNavigation::HandleEvent(DirectoryEvent::FileRemoved,filePath);
                break;
            case FILE_ACTION_RENAMED_OLD_NAME:
                FileNavigation::HandleEvent(DirectoryEvent::FileRenamedOldName,filePath);
                break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                FileNavigation::HandleEvent(DirectoryEvent::FileRenamedNewName,filePath);
                break;
            }

            if (info->NextEntryOffset == 0) {
                break;
            }

            info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<BYTE*>(info) + info->NextEntryOffset);

        } while (true);
    }
}