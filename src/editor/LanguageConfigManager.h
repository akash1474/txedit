#pragma once
#include <unordered_map>
#include "editor/Language.h"
#include "editor/LanguageConfig.h"
#include "core/Log.h"
#include "nlohmann/json.hpp"
#include "core/Timer.h"

class LanguageConfigManager{
	std::unordered_map<TxEdit::Language, LanguageConfig> mLoadedLanguages;
public:
	LanguageConfigManager(){}
	~LanguageConfigManager(){}


	static LanguageConfigManager& Get(){
		static LanguageConfigManager instance;
		return instance;
	}

// private:
	static LanguageConfig* GetLanguageConfig(TxEdit::Language aLanguage){
		OpenGL::ScopedTimer timer("LanguageConfigManager::GetLanguageConfig");
		if(aLanguage==TxEdit::Language::None)
			return nullptr;

		if(Get().mLoadedLanguages.find(aLanguage)!=Get().mLoadedLanguages.end())
			return &Get().mLoadedLanguages[aLanguage];

		std::string queryString;
		if(!LoadLanguageQuery(aLanguage, queryString))
			return nullptr;

		LanguageConfig& config=Get().mLoadedLanguages[aLanguage];
		config.pQueryString=queryString;
		config.tsLanguage=TxEdit::languageToTreeSitterLanguage[aLanguage];

		const std::string jsonFilePath=TxEdit::GetLanguageDataDirectoryPath(aLanguage)+"/config.json";
		std::ifstream file(jsonFilePath);
		if(!file.good()){
			GL_CRITICAL("LanguageConfigManager::GetLanguageConfig - FileError:{}",jsonFilePath);
			return nullptr;
		}

		nlohmann::json configInfo;
		configInfo << file;
		GL_INFO(configInfo["commentString"]);
		config.commentSymbol=configInfo["commentString"];

		return &config;
	}



	static bool LoadLanguageQuery(TxEdit::Language aLanguage,std::string& outQuery) {
		OpenGL::ScopedTimer timer("LanguageConfig::LoadLanguageQuery");
	    std::string path=TxEdit::GetLanguageDataDirectoryPath(aLanguage)+"/highlight.scm";

	    if(!std::filesystem::exists(path)){
	        GL_CRITICAL("Invalid Path:{}",path);
	        return false;
	    }

	    GL_INFO("QueryStringPath:{}",path);
	    std::ifstream file(path);
	    if(!file.good())
	        return false;

	    file.seekg(0, std::ios::end);
	    size_t fileSize = file.tellg();
	    file.seekg(0,std::ios::beg);
	    
	    std::string content(fileSize, ' ');
	    file.read(&content[0], fileSize);


	    outQuery=std::move(content);
	    GL_INFO("QueryString: Loaded");
	    return true;
	}

};
