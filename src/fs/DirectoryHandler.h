#include "string"
#include "vector"

namespace DirectoryHandler{
	bool CreateFile(std::string filePath);
	bool RenameFile(const std::string& filePath,const std::string& newName);
	bool DeleteFile(const std::string& filePath);
	bool DeleteFolder(const std::string& folderPath);
	bool CreateFolder(const std::string& folderPath);
	bool RenameFolder(const std::string& folderPath,const std::string newName);
	void OpenExplorer(const std::string& path);
	void StartTerminal(const std::string& path);
};