#pragma once

#include <string>
#include "Language.h"




class LanguageConfig {
public:
    LanguageConfig(){};
    ~LanguageConfig(){
        ts_query_delete(pQuery);
    };

    void LoadLanguageQuery(TxEdit::Language aType);
    std::string GetCommentSymbol() const;

    std::string commentSymbol;
    TSLanguageFunc tsLanguage;
    std::string pQueryString;
    TSQuery* pQuery=nullptr;


    // void ParseConfigFile(const std::string& filePath);
};