#pragma once
#include "string"
#include "unordered_map"

enum class HighlightType{
	None=0,
    CPP,
    C,
    Python,
    Lua,
    Java,
    JavaScript
};

namespace TxEdit{

	static inline std::unordered_map<std::string,HighlightType> extensionToHighlightTypeMap={
		{"cpp",HighlightType::CPP},
		{"py",HighlightType::Python},
		{"lua",HighlightType::Lua},
		{"c",HighlightType::C},
		{"h",HighlightType::CPP},
		{"hpp",HighlightType::CPP},
	};
}
