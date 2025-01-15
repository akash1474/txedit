#pragma once

#include <windows.h>
#include <string>
#include <thread>
#include <vector>

struct DirectoryWatch{
    HANDLE mHandle;
    std::wstring mDirectoryPath;
    DirectoryWatch(HANDLE aHandle,std::wstring aDir):mHandle(aHandle),mDirectoryPath(aDir){}
};

class DirectoryMonitor {
public:
    DirectoryMonitor();
    ~DirectoryMonitor();
    void Start();
    void Stop();
    void AddDirectoryToWatchList(const std::wstring& aDirectoryPath);
    void MonitorDirectory(HANDLE& hEvent,DirectoryWatch& aDirWatch,OVERLAPPED& overlapped, char* buffer);

private:
    bool mStopMonitoring;
    std::thread mMonitoringThread;
    std::vector<DirectoryWatch> mWatchList;

    // Called on another thread from DirectoryMonitor::Start();
    void Monitor();
};
