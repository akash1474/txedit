#pragma once
#include "vector"
#include "string"
#include "imgui.h"
#include "TextEditor.h"

struct FileTab{
	std::string filepath;
	std::string filename;
	bool isTemp=false;
	bool isActive=false;
	bool isSaved=false;
	bool isOpen=true;
	std::string id;
	Editor* editor;
	FileTab(std::string path,std::string file,bool temp,bool active,bool save,std::string idx)
		:filepath(path),filename(file),isTemp(temp),isActive(active),isSaved(save),id(idx){}
};

class TabsManager{
	std::vector<FileTab> mTabs;
	ImGuiID mDockSpaceId;


public:
	static TabsManager& Get(){
		static TabsManager instance;
		return instance;
	}

	~TabsManager();

	static Editor* GetCurrentActiveTextEditor();
	static FileTab* GetCurrentActiveTab();

	static void SetNewTabsDockSpaceId(ImGuiID aMainDockSpaceId);
	static bool OpenFile(std::string filepath,bool isTemp=true);
	//Call when creating a new file
	static bool OpenNewEmptyFile();
	static void Render();

	static void SaveFile();

	static void DisableSearchForAllTabs();

private:
	TabsManager();
};