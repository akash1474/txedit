#pragma once

enum class TxTokenType {
    TxDefault=0,
    // Identifiers
    TxVariable,
    TxVariableBuiltin,
    TxVariableParameter,
    TxVariableParameterBuiltIn,
    TxVariableMember,
    
    TxConstant,
    TxConstantBuiltin,
    TxConstantMacro,
    
    TxModule,
    TxModuleBuiltin,
    TxLabel,
    
    // Literals
    TxString,
    TxStringDocumentation,
    TxStringRegexp,
    TxStringEscape,
    TxStringSpecial,
    TxStringSpecialSymbol,
    TxStringSpecialUrl,
    TxStringSpecialPath,
    
    TxCharacter,
    TxCharacterSpecial,
    
    TxBoolean,
    TxNumber,
    TxNumberFloat,
    
    // Types
    TxType,
    TxTypeBuiltin,
    TxTypeDefinition,
    
    TxAttribute,
    TxAttributeBuiltin,
    TxProperty,
    
    // Functions
    TxFunction,
    TxFunctionBuiltin,
    TxFunctionCall,
    TxFunctionMacro,
    
    TxFunctionMethod,
    TxFunctionMethodCall,
    
    TxConstructor,
    TxOperator,
    
    // Keywords
    TxKeyword,
    TxKeywordCoroutine,
    TxKeywordFunction,
    TxKeywordOperator,
    TxKeywordImport,
    TxKeywordType,
    TxKeywordModifier,
    TxKeywordRepeat,
    TxKeywordReturn,
    TxKeywordDebug,
    TxKeywordException,
    
    TxKeywordConditional,
    TxKeywordConditionalTernary,
    
    TxKeywordDirective,
    TxKeywordDirectiveDefine,
    
    // Punctuation
    TxPunctuationDelimiter,
    TxPunctuationBracket,
    TxPunctuationSpecial,
    
    // Comments
    TxComment,
    TxCommentDocumentation,
    
    TxCommentError,
    TxCommentWarning,
    TxCommentTodo,
    TxCommentNote,
    
    // Markup
    TxMarkupStrong,
    TxMarkupItalic,
    TxMarkupStrikethrough,
    TxMarkupUnderline,
    
    TxMarkupHeading,
    TxMarkupHeading1,
    TxMarkupHeading2,
    TxMarkupHeading3,
    TxMarkupHeading4,
    TxMarkupHeading5,
    TxMarkupHeading6,
    
    TxMarkupQuote,
    TxMarkupMath,
    
    TxMarkupLink,
    TxMarkupLinkLabel,
    TxMarkupLinkUrl,
    
    TxMarkupRaw,
    TxMarkupRawBlock,
    
    TxMarkupList,
    TxMarkupListChecked,
    TxMarkupListUnchecked,
    
    // Diff
    TxDiffPlus,
    TxDiffMinus,
    TxDiffDelta,
    
    // Tags
    TxTag,
    TxTagBuiltin,
    TxTagAttribute,
    TxTagDelimiter,
    
    // Non-highlighting captures
    TxNone,
    TxConceal,
    TxSpell,
    TxNoSpell,

    //Globals
    TxForeground,
    TxBackground,
    TxCaret,
    TxBlockCaret,
    TxLineHightlight,
    TxMissSpelling,
    TxSelection,
    TxSelectionInActive,
    TxHighlight,
    TxFindHighlight,
    

    TxSize
};