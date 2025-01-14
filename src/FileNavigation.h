#pragma once
#include "ImageTexture.h"
#include "string"
#include "vector"
#include "unordered_map"
#include "TextEditor.h"
#include "DirectoryMonitor.h"
#include <rpcdcep.h>

// Structure to store icon data
struct IconData {
    std::string name;
    std::vector<std::string> extensions;
    ImageTexture texture;
};


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
	std::unordered_map<std::string, IconData> mIconDatabase;
	// DirectoryMonitor mDirectoryMonitor;

	std::unordered_map<std::string,std::vector<Entity>> mDirectoryData;
	void ShowContextMenu(std::string& path,bool isFolder=false);
	void RenderFolderItems(std::string path,bool isRoot=false);

public:

	FileNavigation();
	~FileNavigation();

	void SetTextEditor(Editor* editorPtr){ this->mTextEditor=editorPtr;}
	void Render();

	void ScanDirectory(const std::string& aDirectoryPath);
	bool CustomSelectable(std::string& aFileName,bool aIsSelected=false);

	void AddFolder(std::string aPath);
	void ToggleSideBar(){mIsOpen=!mIsOpen;}
	const bool IsOpen()const{return mIsOpen;}
	std::vector<std::string>& GetFolders(){ return mFolders;}


	std::unordered_map<std::string, IconData> LoadIconData(const std::string& aJsonPath);
	std::pair<const std::string,IconData>* GetIconForExtension(const std::string& aExtension);

};