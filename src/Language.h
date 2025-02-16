#pragma once
#include "TreesitterLanguage.h"
#include "string"
#include "unordered_map"
#include "filesystem"

#include "tree_sitter/api.h"
typedef TSLanguage* (*TSLanguageFunc)(); // Define function pointer type


namespace TxEdit{

	enum class Language{
		None=0,
	    CPP,
	    C,
	    Python,
	    Lua,
	    Java,
	    // JavaScript,
	    Json
	};

	static inline std::unordered_map<std::string,Language> extensionToLanguageMap={
		{"cpp",Language::CPP},
		{"py",Language::Python},
		{"lua",Language::Lua},
		{"java",Language::Java},
		{"c",Language::C},
		{"h",Language::CPP},
		{"hpp",Language::CPP},
		{"json",Language::Json},
	};

	// Maps Language to folder name under `data/languages/{foldername}`
	static inline std::unordered_map<Language,std::string> languageToDirName={
		{Language::CPP,"cpp"},
		{Language::Python,"python"},
		{Language::Lua,"lua"},
		{Language::Java,"java"},
		{Language::C,"c"},
		{Language::Json,"json"}
	};

	// Maps Language to folder name under `data/languages/{foldername}`
	static inline std::unordered_map<Language,TSLanguageFunc> languageToTreeSitterLanguage={
		{Language::CPP,tree_sitter_cpp},
		{Language::Python,tree_sitter_python},
		{Language::Lua,tree_sitter_lua},
		{Language::Java,tree_sitter_java},
		{Language::C,tree_sitter_c},
		{Language::Json,tree_sitter_json}
	};



	static inline Language GetHighlightType(const std::string& aFilePath){
		std::string extension=std::filesystem::path(aFilePath).extension().string();
		if(extension[0]=='.')
			extension=extension.substr(1);

		if(extensionToLanguageMap.find(extension)!=extensionToLanguageMap.end())
		{
			return extensionToLanguageMap[extension];
		}

		return  Language::None;
	}


	static inline const std::string mDataDirectory="./data";
	static inline const std::string mLanguageDirectory=mDataDirectory+"/languages";

	static inline const std::string GetLanguageDataDirectoryPath(Language aLanguage){
		return std::move(mLanguageDirectory+"/"+languageToDirName[aLanguage]);
	}
}
