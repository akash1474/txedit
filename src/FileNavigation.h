#pragma once
#include "string"
#include "vector"
#include "map"
#include "unordered_map"
#include "TextEditor.h"

class FileNavigation{
	struct Entity{
		std::string filename;
		std::string path;
		bool is_directory=false;
		bool is_explored=false;
	};

	Editor* mTextEditor=nullptr;
	bool mIsOpen=true;
	std::vector<std::string> mFolders;

	std::unordered_map<std::string,std::vector<Entity>> mDirectoryData;
	void ShowContextMenu(std::string& path,bool isFolder=true);
	void RenderFolderItems(std::string path,bool isRoot=false);

public:

	FileNavigation(){};
	~FileNavigation(){ mDirectoryData.clear(); }

	void SetTextEditor(Editor* editorPtr){ this->mTextEditor=editorPtr;}
	void Render();

	void UpdateDirectory(std::string directory);

	void AddFolder(std::string path){ mFolders.push_back(path);}
	void ToggleSideBar(){mIsOpen=!mIsOpen;}
	const bool IsOpen()const{return mIsOpen;}
	std::vector<std::string>& GetFolders(){ return mFolders;}

};