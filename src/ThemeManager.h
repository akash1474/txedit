#pragma once
#include "pch.h"
#include "TokenType.h"

class ThemeManager{
	ThemeManager(){};
	std::unordered_map<std::string, ImU32> mCaptureToColor;
	std::unordered_map<std::string, TxTokenType> mCaptureToTokenType;
	std::vector<ImU32> mTokenToColor;
public:
	~ThemeManager(){}

	static ThemeManager& Get(){
		static ThemeManager instance;
		return instance;
	}

	static std::unordered_map<std::string, ImU32>& GetCaptureToColorMap(){return Get().mCaptureToColor;};
	static std::unordered_map<std::string,TxTokenType>& GetCaptureToTokenMap(){return Get().mCaptureToTokenType;}
	static std::vector<ImU32>& GetTokenToColorMap(){return Get().mTokenToColor;}
	static void Init();
	static std::unordered_map<std::string, ImU32> LoadGruvboxColors(const std::string& filename);
	static ImU32 HexToImU32(const std::string& hex);
};