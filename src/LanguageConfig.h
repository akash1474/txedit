#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class LanguageConfig {
public:
    LanguageConfig();
    ~LanguageConfig();

    bool LoadLanguageConfig(const std::string& language);

    std::string GetHighlightColor(const std::string& tokenType) const;

    std::string GetCommentSymbol() const;

    const std::vector<std::string>& GetKeywords() const;

private:
    std::string currentLanguage;
    std::unordered_map<std::string, std::string> syntaxColors; // Token type -> color
    std::vector<std::string> keywords;
    std::string commentSymbol;

    // Helper function to parse a language configuration file
    void ParseConfigFile(const std::string& filePath);
};