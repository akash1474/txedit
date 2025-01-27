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

enum class DirectoryEvent{
	FileModified,
	FileAdded,
	FileRemoved,
	FileRenamedOldName,
	FileRenamedNewName
};


class FileNavigation{
	struct Entity{
		std::string filename;
		std::string path;
		bool is_directory=false;
		bool is_explored=false;
	};

	bool mIsOpen=true;
	bool mAreIconsLoaded=false;
	std::vector<std::string> mFolders;
	std::unordered_map<std::string, IconData> mIconDatabase;
	DirectoryMonitor mDirectoryMonitor;
	Entity* mCurrentEntity{0};
	bool mHoveringThisFrame{0};

	std::unordered_map<std::string,std::vector<Entity>> mDirectoryData;
	static void ShowContextMenu(std::string& path,bool isFolder=false);
	static void RenderFolderItems(std::string path,bool isRoot=false);

	FileNavigation();

public:
	FileNavigation(const FileNavigation&)=delete;

	static FileNavigation& Get(){
		static FileNavigation instance;
		return instance;
	}

	~FileNavigation();

	static void Init();

	static void Render();
	static bool AreIconsLoaded(){return Get().mAreIconsLoaded;}

	static void ScanDirectory(const std::string& aDirectoryPath);
	static bool CustomSelectable(std::string& aFileName,bool aIsSelected=false);

	static void AddFolder(std::string aPath);
	static std::vector<std::string>& GetFolders(){ return Get().mFolders;}

	static void ToggleSideBar(){Get().mIsOpen=!Get().mIsOpen;}
	static const bool IsOpen(){return Get().mIsOpen;}

	static void HandleEvent(DirectoryEvent aEvent,std::wstring& aPayLoad);
	static void LoadIconData(const std::string& aJsonPath);
	static std::pair<const std::string,IconData>* GetIconForExtension(const std::string& aExtension);

	static void MarkFileAsOpen(const std::string& aOpenedFilePath);

	// Returns file type name to be displayed in status bar
	static std::string GetFileTypeNameFromFilePath(const std::string& aFilePath);
};
