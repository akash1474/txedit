#pragma once
#include <unordered_map>
#include "HighlightType.h"
#include "LanguageConfig.h"
#include "Log.h"
#include "TreesitterLanguage.h"

class LanguageConfigManager{
	std::unordered_map<TxEdit::HighlightType, LanguageConfig> mLoadedLanguages;
public:
	LanguageConfigManager(){}
	~LanguageConfigManager(){}


	static LanguageConfigManager& Get(){
		static LanguageConfigManager instance;
		return instance;
	}




// private:
	static LanguageConfig* GetLanguageConfig(TxEdit::HighlightType aType){
		if(aType==TxEdit::HighlightType::None)
			return nullptr;

		if(Get().mLoadedLanguages.find(aType)!=Get().mLoadedLanguages.end())
			return &Get().mLoadedLanguages[aType];

		std::string queryString;
		if(!LoadLanguageQuery(aType, queryString))
			return nullptr;

		LanguageConfig& config=Get().mLoadedLanguages[aType];
		config.pQueryString=queryString;

		switch(aType){
			case TxEdit::HighlightType::None:
				break;
			case TxEdit::HighlightType::CPP:
				config.tsLanguage=tree_sitter_cpp;
				break;
			case TxEdit::HighlightType::C:
				config.tsLanguage=tree_sitter_c;
				break;
			case TxEdit::HighlightType::Python:
				config.tsLanguage=tree_sitter_python;
				break;
			case TxEdit::HighlightType::Lua:
				config.tsLanguage=tree_sitter_lua;
				break;
			case TxEdit::HighlightType::Java:
				config.tsLanguage=tree_sitter_java;
				break;
			// case TxEdit::HighlightType::JavaScript:
			// 	config.tsLanguage=tree_sitter_javascript;
			// 	break;
			case TxEdit::HighlightType::Json:
				config.tsLanguage=tree_sitter_json;
				break;
		}

		return &config;
	}
	static bool LoadLanguageQuery(TxEdit::HighlightType aType,std::string& outQuery) {
	    std::string path;
	    switch(aType){
	    case TxEdit::HighlightType::C:
	        path="./data/languages/c/highlight.scm";
	        break;
	    case TxEdit::HighlightType::CPP:
	        path="./data/languages/cpp/highlight.scm";
	        break;
	    case TxEdit::HighlightType::Python:
	        path="./data/languages/python/highlight.scm";
	        break;
	    case TxEdit::HighlightType::Lua:
	        path="./data/languages/lua/highlight.scm";
	        break;
	    case TxEdit::HighlightType::Java:
	        path="./data/languages/java/highlight.scm";
	        break;
	    case TxEdit::HighlightType::Json:
	        path="./data/languages/json/highlight.scm";
	        break;
	    // case TxEdit::HighlightType::JavaScript:
	    //     path="./data/languages/javascript/highlight.scm";
	    //     break;
		case TxEdit::HighlightType::None:
			break;
		}

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
	    return true;
	}

};