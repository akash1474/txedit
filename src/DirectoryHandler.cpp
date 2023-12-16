#include "pch.h"
#include "shellapi.h"
#include "DirectoryHandler.h"
#include <filesystem>
#include <processthreadsapi.h>
#include <stdlib.h>
#include <winnt.h>


bool DirectoryHandler::CreateFile(const std::string& filePath){
	if(std::filesystem::exists(filePath)) return false;
	std::ofstream file(filePath);
	file.close();
	return true;
}



void DirectoryHandler::OpenExplorer(const std::string& path){
	std::string copy=path;
	std::replace(copy.begin(), copy.end(), '/', '\\');
	GL_INFO("OpenExplorer: {}",copy);
	if(!std::filesystem::is_directory(copy)){
		ShellExecute(NULL, L"open", L"explorer.exe", std::filesystem::path(copy).parent_path().wstring().c_str(), NULL, SW_SHOWNORMAL);
		return;
	}
	std::wstring wideString = StringToWString(copy);
	ShellExecute(NULL, L"open", L"explorer.exe",std::filesystem::path(copy).wstring().c_str(), NULL, SW_SHOWNORMAL);
}


void DirectoryHandler::StartTerminal(const std::string& path){
	std::string copy=path;
	std::replace(copy.begin(), copy.end(), '/', '\\');
	GL_INFO("OpenTerminal: {}",copy);

	if(!std::filesystem::is_directory(copy))
        copy=std::filesystem::path(copy).parent_path().generic_string();

    std::string command = "start cmd.exe /K cd /d " + copy;
	system(command.c_str());
}


bool DirectoryHandler::RenameFile(const std::string& filePath,const std::string& newName){
    try {
        std::filesystem::path oldPath(filePath);
        std::filesystem::path newPath = oldPath.parent_path() / newName;

        std::filesystem::rename(oldPath, newPath);

		GL_INFO("OLD:{} NEW:{}",oldPath.generic_string(),newPath.generic_string());
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        // std::cerr << "Error renaming folder: " << e.what() << std::endl;
        return false;
    }
    return false;
}


bool DirectoryHandler::DeleteFile(const std::string& filePath){
	return std::filesystem::remove(filePath);
}


bool DirectoryHandler::DeleteFolder(const std::string& folderPath){
	return std::filesystem::remove_all(folderPath);
}


bool DirectoryHandler::CreateFolder(const std::string& folderPath){
	if(std::filesystem::exists(folderPath)) return false;
	return std::filesystem::create_directory(folderPath);
}


bool DirectoryHandler::RenameFolder(const std::string& folderPath,const std::string newName){
    try {
        std::filesystem::path oldFolderPath(folderPath);
        std::filesystem::path newFolderPath = oldFolderPath.parent_path() / newName;

        std::filesystem::rename(oldFolderPath, newFolderPath);

        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        // std::cerr << "Error renaming folder: " << e.what() << std::endl;
        return false;
    }
}
