#pragma once
#include <string>
#include <vector>



struct MatchResult {
    std::string filename;
    double score;

    bool operator<(const MatchResult& other) const {
        return score > other.score;  // higher scores first
    }
};

class QuickFileSearch
{
	int mSelectedIndex{0};
    bool mIsOpen{0};
	char mSearchBuffer[256] = "";

    std::vector<MatchResult> mMatchedResults;
	std::vector<std::string> mFiles;

	std::vector<MatchResult> mRecentlyOpenedMatchedResults;

	std::string mRootDirectory{0};
    QuickFileSearch(){};

	static double CalculateFuzzyScore(const std::string& filename, const std::string& aQuery);
	static std::vector<std::string> GetFilesInDirectory(const std::string& directoryPath,const std::string& rootPath, int maxDepth = 1, int currentDepth = 0);
	static std::vector<MatchResult> ExecuteFuzzySearch(const std::vector<std::string>& aFileNames, const std::string& aQuery);
public:
    ~QuickFileSearch();
    static QuickFileSearch& Get(){
    	static QuickFileSearch instance;
    	return instance;
    }

	static void Render();
	static void EventListener();
	static void CloseQuickSearch();

};