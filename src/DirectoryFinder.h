#pragma once
#include <regex>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

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
	int mCreatedFromFolderView = -1;
	bool mIsWindowOpen=false;

	char mDirectoryPath[MAX_PATH_LENGTH] = "\0";
	bool mRegexEnabled = true;
	bool mCaseSensitiveEnabled = true;
	char mToFind[INPUT_BUFFER_SIZE] = "\0";

	std::vector<DFResultFile> mResultFiles;
	std::vector<DFResult*> mResultsInFile;

	std::thread* mFinderThread = nullptr;
	std::mutex mFinderMutex;

public:
	static void Find();
	static void Setup(const std::string& aFolderPath, int aOpenedFromExplorer = -1);
	static bool Render();
};