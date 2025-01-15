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
	DirectoryMonitor mDirectoryMonitor;

	std::unordered_map<std::string,std::vector<Entity>> mDirectoryData;
	static void ShowContextMenu(std::string& path,bool isFolder=false);
	static void RenderFolderItems(std::string path,bool isRoot=false);

	FileNavigation();

public:
	static FileNavigation& Get(){
		static FileNavigation instance;
		return instance;
	}

	~FileNavigation();

	static void SetTextEditor(Editor* editorPtr){ Get().mTextEditor=editorPtr;}
	static void Render();

	static void ScanDirectory(const std::string& aDirectoryPath);
	static bool CustomSelectable(std::string& aFileName,bool aIsSelected=false);

	static void AddFolder(std::string aPath);
	static void ToggleSideBar(){Get().mIsOpen=!Get().mIsOpen;}
	static const bool IsOpen(){return Get().mIsOpen;}
	static std::vector<std::string>& GetFolders(){ return Get().mFolders;}


	std::unordered_map<std::string, IconData> LoadIconData(const std::string& aJsonPath);
	std::pair<const std::string,IconData>* GetIconForExtension(const std::string& aExtension);

};