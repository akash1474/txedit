#include <iostream>
#include <clang-c/Index.h>

const char* getCursorKindString(CXCursorKind kind) {
    return (const char*)clang_getCursorKindSpelling(kind).data;
}

int main() {
    // Initialize Clang
    CXIndex index = clang_createIndex(0, 0);

    // Specify the C++ source file to tokenize
    const char* source_filename = "./src/lexer.cpp";

    // Compilation options
    const char* compile_args[] = {
        "-std=c++11",  // Adjust this according to your desired C++ standard
        // Additional compiler flags or include paths if needed
    };

    // Create a translation unit
    CXTranslationUnit translationUnit = clang_createTranslationUnitFromSourceFile(
        index, source_filename, sizeof(compile_args) / sizeof(compile_args[0]), compile_args, 0, 0);

    // Check for errors in creating the translation unit
    if (!translationUnit) {
        std::cerr << "Error creating translation unit" << std::endl;
        return 1;
    }

    // Tokenization
    CXToken* tokens;
    unsigned num_tokens;
    clang_tokenize(translationUnit, clang_getCursorExtent(clang_getTranslationUnitCursor(translationUnit)), &tokens, &num_tokens);

    // Get and print the token kinds
    for (unsigned i = 0; i < num_tokens; ++i) {
        CXToken token = tokens[i];
        CXCursor cursor = clang_getCursor(translationUnit, clang_getTokenLocation(translationUnit, token));

        // Get the cursor kind and print it
        CXCursorKind kind = clang_getCursorKind(cursor);
        std::cout << "Token Kind: " << getCursorKindString(kind) << std::endl;
    }

    // Clean up resources
    clang_disposeTokens(translationUnit, tokens, num_tokens);
    clang_disposeTranslationUnit(translationUnit);
    clang_disposeIndex(index);

    return 0;
}
                                                    
