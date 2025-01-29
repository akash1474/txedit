#pragma once

#include <string>
#include "HighlightType.h"
#include "tree_sitter/api.h"

typedef TSLanguage* (*TSLanguageFunc)(); // Define function pointer type



class LanguageConfig {
public:
    LanguageConfig(){};
    ~LanguageConfig(){
        ts_query_delete(pQuery);
    };

    void LoadLanguageQuery(TxEdit::HighlightType aType);
    std::string GetCommentSymbol() const;

    std::string commentSymbol;
    TSLanguageFunc tsLanguage;
    std::string pQueryString;
    TSQuery* pQuery=nullptr;


    // void ParseConfigFile(const std::string& filePath);
};