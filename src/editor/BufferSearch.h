#include <string>
#include <vector>

#include "editor/TextEditor.h"

class SearchState {
private:
    std::string mWord;
    size_t mWordLen;
    std::vector<Coordinates> mFoundPositions;
    bool mIsGlobal;
    int mIdx;

public:
    SearchState() : mIsGlobal(false), mIdx(0), mWordLen(0) {}

    const std::string& getSearchWord() const { return mWord; }
    size_t getWordLength() const { return mWordLen; }
    const std::vector<Coordinates>& getFoundPositions() const { return mFoundPositions; }
    bool isGlobal() const { return mIsGlobal; }
    int getIdx() const { return mIdx; }

    void setSearchWord(const std::string& word) 
    {
        mWord = word;
        mWordLen = GetUTF8StringLength(word);
    }

    bool isValid() const 
    {
        return !mWord.empty() && !mFoundPositions.empty();
    }

    void reset() 
    {
        mWord.clear();
        mFoundPositions.clear();
        mIsGlobal = false;
        mIdx = 0;
        mWordLen = 0;
    }

    void disableSearch() 
    {
        if (this->isValid())
            this->reset();
    }

    bool hasSearchStarted(const std::string& aWord) const 
    {
        return mWord == aWord && this->isValid() && mIsGlobal;
    }
};
