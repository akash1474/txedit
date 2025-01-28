#include "pch.h"

#include "LanguageConfig.h"

#include <fstream>
#include <sstream>

LanguageConfig::LanguageConfig() {
}

LanguageConfig::~LanguageConfig() {}

bool LanguageConfig::LoadLanguageConfig(const std::string& language) {
    return true;
}

std::string LanguageConfig::GetHighlightColor(const std::string& tokenType) const {
    return "#FFFFFF"; // Default color (white)
}

std::string LanguageConfig::GetCommentSymbol() const {
    return commentSymbol;
}

const std::vector<std::string>& LanguageConfig::GetKeywords() const {
    return keywords;
}

void LanguageConfig::ParseConfigFile(const std::string& filePath) {

}