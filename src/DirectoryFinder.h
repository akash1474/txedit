#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include "imgui.h"


#define INPUT_BUFFER_SIZE 256
#define MAX_PATH_LENGTH 512

struct DFResult
{
	std::string displayText;
	int lineNumber;
	int startCharIndex;
	int endCharIndex;
};

struct DFResultFile
{
	std::string filePath;
	std::string fileName;
	std::vector<DFResult> results;
};

class DirectoryFinder
{
	static DirectoryFinder& Get(){
		static DirectoryFinder instance;
		return instance;
	}

private:
	DirectoryFinder(){};
	bool mOpenedFromExplorer =false;
	bool mIsWindowOpen=false;

	bool mRegexEnabled = false;
	bool mCaseSensitiveEnabled = false;
	
	char mDirectoryPath[MAX_PATH_LENGTH] = "\0";
	char mToFind[INPUT_BUFFER_SIZE] = "\0";

	std::vector<DFResultFile> mResultFiles;
	std::vector<DFResult*> mResultsInFile;

	std::thread* mFinderThread = nullptr;
	std::mutex mFinderMutex;
	ImGuiID mDockspaceId;	

public:
	static void SetDockspaceId(ImGuiID aRightDockspaceId){Get().mDockspaceId=aRightDockspaceId;}
	static void Find();
	static void Setup(const std::string& aFolderPath,bool aOpenedFromExplorer = false);
	static bool Render();
};