#pragma once
#include "Coordinates.h"
#include "string"
#include "vector"
#include <ctype.h>


class Lexer
{
public:
    enum TokenType {
        End = 0,
        HashInclude,
        HeaderName,
        OpenParen,
        CloseParen,
        OpenCurly,
        CloseCurly,
        OpenSquare,
        CloseSquare,
        Symbol,
        Comment,
        SemiColon,
        Keyword,
        DataType,
        ScopeResolution,
        Function,
        Operator,
        String,
        Number,
        TabSpace,
        Invalid
    };

    struct Token {
        TokenType type;
        const char* text = nullptr;
        size_t text_len{0};
        Coordinates location;
    };
private:

    struct LiteralToken {
        TokenType type;
        const char* text=nullptr;
        LiteralToken(TokenType t, const char* c) : type(t), text(c) {}
    };

    std::vector<LiteralToken> mLiteralTokens;

    std::string mContent;
    size_t mContentLength{0};
    size_t mCursorPos{0};
    size_t mLine{0};
    size_t mLineStart{0};

    inline bool mIsSymbolStart(char x) { return isalpha(x) || x == '_'; }
    inline bool mIsIdentifier(char x) { return isalnum(x) || x == '_'; }
    inline void mEscapeWhiteSpace();
    void mNextCharacter(int count=1);
    bool mStartsWith(const char* text);
    Token& mLastToken() { return mTokens.back(); }

    Token mGetNextToken();
    void InitLiteralTokens();
    std::vector<Token> mTokens;

  public:
    Lexer();
    Lexer(std::string content);
    void SetData(std::string content);
    const std::vector<Token>& GetTokens() const noexcept {return mTokens; }

    void Tokenize();
    const char* GetTokenType(int tkn);
};