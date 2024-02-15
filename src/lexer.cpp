#include "Timer.h"
#include "pch.h"
#include "Lexer.h"
#include "iostream"
#include "cstring"
#include <cctype>
#include <ctype.h>

const char* keywords[] = {
    "and", "asm", "for", "new", "not", "xor",
    "else", "goto","case", "enum", "this", "true", "class",
    "union","catch", "const", "decltype", "delete", "export", "inline", "module", "return",
    "bitand", "bitor", "compl", "import", "not_eq", "or_eq", "typeid", "typename", "volatile",
    "alignas", "bitxor", "concept", "private", "protected", "requires", "sizeof", "static", "switch", "virtual",
    "alignof", "co_await", "co_return", "co_yield", "dynamic_cast", "namespace", "reinterpret_cast", "noexcept", "nullptr", "operator",
    "consteval", "constexpr", "constinit", "const_cast", "continue", "explicit", "extern", "register", "static_assert", "thread_local",
    "catchs", "integers", 
    "catched", "integrate",
    "catching", "integral",
};

const char* data_type[]= {
    "short","int","char","long","bool", "char8_t", "char16_t", "char32_t",
    "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t",
    "uint32_t", "uint64_t", "float", "double", "signed", "unsigned",
    "wchar_t","map","vector","set","string","unordered_map",
    "pair","list","deque","stack","queue","array"
};


#define SIZE_KEYWORD sizeof(keywords)/sizeof(keywords[0])
#define SIZE_DATATYPE sizeof(data_type)/sizeof(data_type[0])

Lexer::Lexer(){}


Lexer::Lexer(std::string content) : mContent(content), mContentLength(content.size()) {
    InitLiteralTokens();
}

void Lexer::SetData(std::string data){
    mContent=data;
    mContentLength=data.size();
    InitLiteralTokens();
}

void Lexer::InitLiteralTokens(){
    GL_WARN("LiteralToken");
    mLiteralTokens.emplace_back(OpenParen,"(");
    mLiteralTokens.emplace_back(CloseParen,")");
    mLiteralTokens.emplace_back(OpenCurly,"{");
    mLiteralTokens.emplace_back(CloseCurly,"}");
    mLiteralTokens.emplace_back(OpenSquare,"[");
    mLiteralTokens.emplace_back(CloseSquare,"]");
    mLiteralTokens.emplace_back(SemiColon,";");
    mLiteralTokens.emplace_back(Operator,"+");
    mLiteralTokens.emplace_back(Operator,"-");
    mLiteralTokens.emplace_back(Operator,"/");
    mLiteralTokens.emplace_back(Operator,"*");
    mLiteralTokens.emplace_back(Operator,"&");
    mLiteralTokens.emplace_back(Operator,"|");
    mLiteralTokens.emplace_back(Operator,"!=");
    mLiteralTokens.emplace_back(Operator,"!");
    mLiteralTokens.emplace_back(Operator,"=");
    mLiteralTokens.emplace_back(Operator,">>");
    mLiteralTokens.emplace_back(Operator,"<<");
    mLiteralTokens.emplace_back(Operator,"<=");
    mLiteralTokens.emplace_back(Operator,">=");
    mLiteralTokens.emplace_back(Operator,"<");
    mLiteralTokens.emplace_back(Operator,">");
    mLiteralTokens.emplace_back(Operator,".");
    mLiteralTokens.emplace_back(ScopeResolution,"::");
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
    case DataType:
        return "DataType";
        break;
    case String:
        return "String";
        break;
    case Number:
        return "Number";
        break;
    case ScopeResolution:
        return "ScopeResolution";
        break;
    case Function:
        return "Function";
        break;
    case Operator:
        return "Operator";
        break;
    case TabSpace:
        return "TabSpace";
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
    if(x=='\t'){
        Token tkn;
        tkn.type=TabSpace;
        tkn.location=Coordinates(mLine,mLineStart-mCursorPos-count);
        mTokens.push_back(tkn);
    }
}

Lexer::Token Lexer::mGetNextToken()
{
    mEscapeWhiteSpace();
    Token tkn;
    tkn.type = End;
    tkn.text = &mContent[mCursorPos];

    if (mCursorPos > mContentLength) return tkn;


    //#include
    if (mContent[mCursorPos] == '#') {
        tkn.type = HashInclude;
        tkn.location = Coordinates(mLine, mCursorPos - mLineStart);
        while (mCursorPos < mContentLength && mContent[mCursorPos] != ' ' ) {
            tkn.text_len++;
            mNextCharacter();
        }
        return tkn;
    }

    //HeaderName
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


    //COmment
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


    //String
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


    //mLiteralTokens
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

    //Numbers
    if(isdigit(mContent[mCursorPos])){
        tkn.type=Number;
        tkn.location = Coordinates(mLine, mCursorPos - mLineStart);
        while(mCursorPos < mContentLength && mContent[mCursorPos]!=';'){
            tkn.text_len++;
            mNextCharacter();
        }
        return tkn;
    }



    //Identifier/Symbols
    if (mIsSymbolStart(mContent[mCursorPos])) {
        tkn.type = Symbol;
        tkn.location = Coordinates(mLine, mCursorPos - mLineStart);
        while (mCursorPos < mContentLength && mIsIdentifier(mContent[mCursorPos])) {
            tkn.text_len++;
            mNextCharacter();
        }
        for(int i=0;i<SIZE_KEYWORD;i++){
            if(strlen(keywords[i])==tkn.text_len && memcmp(tkn.text,keywords[i],tkn.text_len)==0){
                tkn.type=Keyword;
                return tkn;
            }
        }

        for(int i=0;i<SIZE_DATATYPE;i++){
            if(strlen(data_type[i])==tkn.text_len && memcmp(tkn.text,data_type[i],tkn.text_len)==0){
                tkn.type=DataType;
                return tkn;
            }
        }


        mEscapeWhiteSpace();
        //Function
        if(mLastToken().type==DataType && tkn.type==Symbol && mContent[mCursorPos]=='('){
            tkn.type=Function;
            return tkn;
        }

        return tkn;
    }

    tkn.location = Coordinates(mLine, mCursorPos - mLineStart);
    mCursorPos++;
    tkn.type = End;
    tkn.text_len = 1;

    return tkn;
}

void Lexer::Tokenize()
{
    OpenGL::ScopedTimer timer("Tokenizing");
    Token tkn;
    tkn = mGetNextToken();
    mTokens.push_back(tkn);
    while (tkn.type != End) {
        // fprintf(stderr, "%.*s, (%s) [ %d, %d ]\n", (int)tkn.text_len, tkn.text, GetTokenType(tkn.type),tkn.location.mColumn,tkn.location.mLine);
        tkn = mGetNextToken();
        mTokens.push_back(tkn);
    }
    GL_INFO("Tokenization Complete");
}

// int main()
// {
//     std::string content = "  #include <iostream>\n"
//                           "//This is comment\n"
//                           "int main(){\n"
//                           "     int max=1458;\n"
//                           "     float offset=2.04f;\n"
//                           "     std::cout << \"Google\" << std::endl;\n"
//                           "     return 0;\n"
//                           "}\n";
//     Lexer lexer(content);
//     lexer.Tokenize();
//     return 0;
// }
