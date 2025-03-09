#pragma once
#include "vector"
#include "string"
#include "imgui.h"
#include "TextEditor.h"
#include "Trie.h"

struct FileTab{
	std::string filepath;
	std::string filename;
	bool isTemp=false;
	bool isActive=false;
	bool isSaved=false;
	bool isOpen=true;
	//Use for focusing the editor window when we reopen the same file and the file is already open
	ImGuiWindow* winPtr=nullptr;
	std::string id;
	Editor* editor;
	FileTab(std::string path,std::string file,bool temp,bool active,bool save,std::string idx)
		:filepath(path),filename(file),isTemp(temp),isActive(active),isSaved(save),id(idx){}
};

class TabsManager{
	std::vector<FileTab> mTabs;
	ImGuiID mDockSpaceId;
	int mLineNumberToScroll=-1;
	Trie::Node* mTokenSuggestionsRoot{0};


public:
	static TabsManager& Get(){
		static TabsManager instance;
		return instance;
	}

	~TabsManager();

	static Trie::Node* GetTrieRootNode();

	static Editor* GetCurrentActiveTextEditor();
	static FileTab* GetCurrentActiveTab();
	static FileTab* GetTabWithFileName(const std::string& aName);

	static void SetNewTabsDockSpaceId(ImGuiID aMainDockSpaceId);
	static FileTab* OpenFile(std::string filepath,bool isTemp=true);
	static void OpenFileWithAtLineNumber(const std::string& aFilePath,int aLineNumber,int aStartIndex,int aEndIndex);
	//Call when creating a new file
	static bool OpenNewEmptyFile();
	static void Render();

	static void SaveFile();

	static void DisableSearchForAllTabs();

private:
	TabsManager();
};