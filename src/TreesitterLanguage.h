#pragma once
#include "tree_sitter/api.h"

extern "C" {
    TSLanguage *tree_sitter_c();
    TSLanguage *tree_sitter_cpp();
    TSLanguage *tree_sitter_python();
    TSLanguage *tree_sitter_lua();
    TSLanguage *tree_sitter_json();
    TSLanguage *tree_sitter_java();
    // TSLanguage *tree_sitter_javascript();
}
