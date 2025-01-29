#pragma once
#include "string"
#include "unordered_map"
#include "filesystem"



namespace TxEdit{

	enum class HighlightType{
		None=0,
	    CPP,
	    C,
	    Python,
	    Lua,
	    Java,
	    // JavaScript,
	    Json
	};

	static inline std::unordered_map<std::string,HighlightType> extensionToHighlightTypeMap={
		{"cpp",HighlightType::CPP},
		{"py",HighlightType::Python},
		{"lua",HighlightType::Lua},
		{"java",HighlightType::Java},
		{"c",HighlightType::C},
		{"h",HighlightType::CPP},
		{"hpp",HighlightType::CPP},
		{"json",HighlightType::Json},
	};

	static inline HighlightType GetHighlightType(const std::string& aFilePath){
		std::string extension=std::filesystem::path(aFilePath).extension().string();
		if(extension[0]=='.')
			extension=extension.substr(1);

		if(extensionToHighlightTypeMap.find(extension)!=extensionToHighlightTypeMap.end())
		{
			return extensionToHighlightTypeMap[extension];
		}

		return  HighlightType::None;
	}
}
