#pragma once

#include <windows.h>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <set>

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

    // Add these functions to register files being modified by your application
    static void RegisterFileModification(const std::wstring& filePath);
    static void UnregisterFileModification(const std::wstring& filePath);
    bool IsFileModifiedByOwnProcess(const std::wstring& filePath);

private:
    bool mStopMonitoring;


    std::thread mMonitoringThread;
    std::vector<DirectoryWatch> mWatchList;

    static std::mutex mFilesLock;
    static std::set<std::wstring> mFilesBeingModified;
    
    // Called on another thread from DirectoryMonitor::Start();
    void Monitor();
};
