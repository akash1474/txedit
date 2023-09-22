#include "Lexer.h"
#include "iostream"
#include "cstring"
#include <cctype>

const char* keywords[] = {
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break",
    "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const",
    "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype", "default",
    "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false",
    "float", "for", "friend", "goto", "if", "import", "inline", "int", "long", "module",
    "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq",
    "private", "protected", "public", "register", "reinterpret_cast", "requires", "return", "short", "signed", "sizeof",
    "static", "static_assert", "static_cast", "struct", "switch", "synchronized", "template", "this", "thread_local", "throw",
    "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
    "volatile", "wchar_t", "while", "xor", "xor_eq", "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t",
    "uint16_t", "uint32_t", "uint64_t",
};


Lexer::Lexer(std::string content) : mContent(content), mContentLength(content.size()) {
    mLiteralTokens.emplace_back(OpenParen,"(");
    mLiteralTokens.emplace_back(CloseParen,")");
    mLiteralTokens.emplace_back(OpenCurly,"{");
    mLiteralTokens.emplace_back(CloseCurly,"}");
    mLiteralTokens.emplace_back(OpenSquare,"[");
    mLiteralTokens.emplace_back(CloseSquare,"]");
    mLiteralTokens.emplace_back(SemiColon,";");
}


const char* Lexer::GetTokenType(int type)
{
    switch (type) {
    case Symbol:
        return "Symbol";
        break;
    case HashInclude:
        return "HashInclude";
        break;
    case HeaderName:
        return "HeaderName";
        break;
    case OpenParen:
        return "OpenParen";
        break;
    case CloseParen:
        return "CloseParen";
        break;
    case OpenCurly:
        return "OpenCurly";
        break;
    case CloseCurly:
        return "CloseCurly";
        break;
    case OpenSquare:
        return "OpenSquare";
        break;
    case CloseSquare:
        return "CloseSquare";
        break;
    case Comment:
        return "Comment";
        break;
    case SemiColon:
        return "SemiColon";
        break;
    case Keyword:
        return "Keyword";
        break;
    case String:
        return "String";
        break;
    case End:
        return "End";
        break;
    case Invalid:
        return "Invalid";
        break;
    default:
        return "Invalid Symbol";
    }
}


inline void Lexer::mEscapeWhiteSpace()
{
    while (mCursorPos < mContentLength && isspace(mContent[mCursorPos])) {
        mNextCharacter();
    }
}

bool Lexer::mStartsWith(const char* prefix){
    int len=strlen(prefix);
    if(len==0) return true;
    if(mCursorPos+len-1>=mContentLength) return false;

    for(int i=0;i<len;i++){
        if(mContent[mCursorPos+i]!=prefix[i]) return false;
    }
    return true;
}

void Lexer::mNextCharacter(int count) {
    char x = mContent[mCursorPos];
    mCursorPos+=count;
    if (x == '\n') {
        mLine++;
        mLineStart = mCursorPos;
    }
}

Lexer::Token Lexer::mGetNextToken()
{
    mEscapeWhiteSpace();
    Token tkn;
    tkn.type = End;
    tkn.text = &mContent[mCursorPos];

    if (mCursorPos > mContentLength) return tkn;

    if (mContent[mCursorPos] == '#') {
        tkn.type = HashInclude;
        tkn.location = Coordinates(mLine, mCursorPos - mLineStart);
        while (mCursorPos < mContentLength && mContent[mCursorPos] != ' ' ) {
            tkn.text_len++;
            mNextCharacter();
        }
        return tkn;
    }

    if ( mLastToken().type==HashInclude && (mContent[mCursorPos] == '"' || mContent[mCursorPos] == '<') )
    {
        tkn.type=HeaderName;
        tkn.location = Coordinates(mLine, mCursorPos - mLineStart);
        while(mContentLength > mCursorPos && mContent[mCursorPos]!='\n'){
            tkn.text_len++;
            mNextCharacter();
        }
        if(mCursorPos < mContentLength) mNextCharacter();
        return tkn;
    } 

    if(mStartsWith("//")){
        tkn.type=Comment;
        tkn.location = Coordinates(mLine, mCursorPos - mLineStart);
        while(mContentLength > mCursorPos && mContent[mCursorPos]!='\n'){
            tkn.text_len++;
            mNextCharacter();
        }
        if(mCursorPos < mContentLength) mNextCharacter();
        return tkn;
    }

    if(mContent[mCursorPos]=='"'){
        tkn.type=String;
        tkn.location = Coordinates(mLine, mCursorPos - mLineStart);
        tkn.text_len++;
        mCursorPos++;
        while(mContentLength > mCursorPos && mContent[mCursorPos]!='"'){
            tkn.text_len++;
            mCursorPos++;
        }
        mCursorPos++;
        tkn.text_len++;
        return tkn;
    }


    for(const LiteralToken& lToken:mLiteralTokens){
        if(mStartsWith(lToken.text)){
            tkn.location = Coordinates(mLine, mCursorPos - mLineStart);
            int len=strlen(lToken.text);
            tkn.type=lToken.type;
            tkn.text_len=len;
            mCursorPos+=len;
            return tkn;
        }
    }

    if(mLastToken().type==)


    if (mIsSymbolStart(mContent[mCursorPos])) {
        tkn.type = Symbol;
        tkn.location = Coordinates(mLine, mCursorPos - mLineStart);
        while (mCursorPos < mContentLength && mIsIdentifier(mContent[mCursorPos])) {
            tkn.text_len++;
            mNextCharacter();
        }
        for(int i=0;i<sizeof(keywords)/sizeof(keywords[0]);i++){
            if(strlen(keywords[i])==tkn.text_len && memcmp(tkn.text,keywords[i],tkn.text_len)){
                tkn.type=Keyword;
            }
        }
        return tkn;
    }

    tkn.location = Coordinates(mLine, mCursorPos - mLineStart);
    mCursorPos++;
    tkn.type = Invalid;
    tkn.text_len = 1;

    return tkn;
}

void Lexer::Tokenize()
{
    Token tkn;
    tkn = mGetNextToken();
    mTokens.push_back(tkn);
    while (tkn.type != End) {
        fprintf(stderr, "%.*s, (%s) [ %d, %d ]\n", (int)tkn.text_len, tkn.text, GetTokenType(tkn.type),tkn.location.mColumn,tkn.location.mLine);
        tkn = mGetNextToken();
        mTokens.push_back(tkn);
    }
}

int main()
{
    std::string content = "  #include <iostream>\n"
                          "//This is comment\n"
                          "int main(){\n"
                          "     std::cout << \"Google\" << std::endl;\n"
                          "     return 0;\n"
                          "}\n";
    Lexer lexer(content);
    lexer.Tokenize();
    return 0;
}