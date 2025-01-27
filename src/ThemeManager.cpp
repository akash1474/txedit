#include "pch.h"
#include "TokenType.h"
#include "ThemeManager.h"
#include "nlohmann/json.hpp"

void ThemeManager::Init(){
	Get().mCaptureToTokenType = {
		{"default",TxTokenType::TxDefault},
	    {"variable", TxTokenType::TxVariable},
	    {"variable.builtin", TxTokenType::TxVariableBuiltin},
	    {"variable.parameter", TxTokenType::TxVariableParameter},
	    {"variable.parameter.builtin", TxTokenType::TxVariableParameterBuiltIn},
	    {"variable.member", TxTokenType::TxVariableMember},
	    
	    {"constant", TxTokenType::TxConstant},
	    {"constant.builtin", TxTokenType::TxConstantBuiltin},
	    {"constant.macro", TxTokenType::TxConstantMacro},
	    
	    {"module", TxTokenType::TxModule},
	    {"module.builtin", TxTokenType::TxModuleBuiltin},
	    {"label", TxTokenType::TxLabel},
	    
	    {"string", TxTokenType::TxString},
	    {"string.documentation", TxTokenType::TxStringDocumentation},
	    {"string.regexp", TxTokenType::TxStringRegexp},
	    {"string.escape", TxTokenType::TxStringEscape},
	    {"string.special", TxTokenType::TxStringSpecial},
	    {"string.special.symbol", TxTokenType::TxStringSpecialSymbol},
	    {"string.special.url", TxTokenType::TxStringSpecialUrl},
	    {"string.special.path", TxTokenType::TxStringSpecialPath},
	    
	    {"character", TxTokenType::TxCharacter},
	    {"character.special", TxTokenType::TxCharacterSpecial},
	    
	    {"boolean", TxTokenType::TxBoolean},
	    {"number", TxTokenType::TxNumber},
	    {"number.float", TxTokenType::TxNumberFloat},
	    
	    {"type", TxTokenType::TxType},
	    {"type.builtin", TxTokenType::TxTypeBuiltin},
	    {"type.definition", TxTokenType::TxTypeDefinition},

	    {"attribute", TxTokenType::TxAttribute},
	    {"attribute.builtin", TxTokenType::TxAttributeBuiltin},
	    {"property", TxTokenType::TxProperty},
	    
	    {"function", TxTokenType::TxFunction},
	    {"function.builtin", TxTokenType::TxFunctionBuiltin},
	    {"function.call", TxTokenType::TxFunctionCall},
	    {"function.macro", TxTokenType::TxFunctionMacro},
	    {"function.method", TxTokenType::TxFunctionMethod},
	    {"function.method.call", TxTokenType::TxFunctionMethodCall},

	    {"constructor", TxTokenType::TxConstructor},
	    {"operator", TxTokenType::TxOperator},
	    
	    {"keyword", TxTokenType::TxKeyword},
	    {"keyword.coroutine", TxTokenType::TxKeywordCoroutine},
	    {"keyword.function", TxTokenType::TxKeywordFunction},
	    {"keyword.operator", TxTokenType::TxKeywordOperator},
	    {"keyword.import", TxTokenType::TxKeywordImport},
	    {"keyword.type", TxTokenType::TxKeywordType},
	    {"keyword.modifier", TxTokenType::TxKeywordModifier},
	    {"keyword.repeat", TxTokenType::TxKeywordRepeat},
	    {"keyword.debug", TxTokenType::TxKeywordDebug},
	    {"keyword.exception", TxTokenType::TxKeywordException},

	    {"keyword.conditional", TxTokenType::TxKeywordConditional},
	    {"keyword.conditional.ternary", TxTokenType::TxKeywordConditionalTernary},

	    {"keyword.directive", TxTokenType::TxKeywordDirective},
	    {"keyword.directive.define", TxTokenType::TxKeywordDirectiveDefine},
	    
	    {"punctuation.delimiter", TxTokenType::TxPunctuationDelimiter},
	    {"punctuation.bracket", TxTokenType::TxPunctuationBracket},
	    {"punctuation.special", TxTokenType::TxPunctuationSpecial},
	    
	    //Comment
	    {"comment", TxTokenType::TxComment},
	    {"comment.documentation", TxTokenType::TxCommentDocumentation},
	    {"comment.error", TxTokenType::TxCommentError},
	    {"comment.warning", TxTokenType::TxCommentWarning},
	    {"comment.todo", TxTokenType::TxCommentTodo},
	    {"comment.note", TxTokenType::TxCommentNote},
	    
	    //Markup
	    {"markup.strong", TxTokenType::TxMarkupStrong},
	    {"markup.italic", TxTokenType::TxMarkupItalic},
	    {"markup.underline", TxTokenType::TxMarkupUnderline},
	    {"markup.strikethrough", TxTokenType::TxMarkupStrikethrough},

	    {"markup.heading", TxTokenType::TxMarkupHeading},
	    {"markup.heading.1", TxTokenType::TxMarkupHeading1},
	    {"markup.heading.2", TxTokenType::TxMarkupHeading2},
	    {"markup.heading.3", TxTokenType::TxMarkupHeading3},
	    {"markup.heading.4", TxTokenType::TxMarkupHeading4},
	    {"markup.heading.5", TxTokenType::TxMarkupHeading5},
	    {"markup.heading.6", TxTokenType::TxMarkupHeading6},

	    {"markup.quote", TxTokenType::TxMarkupQuote},
	    {"markup.match", TxTokenType::TxMarkupMath},

	    {"markup.link", TxTokenType::TxMarkupLink},
	    {"markup.link.label", TxTokenType::TxMarkupLinkLabel},
	    {"markup.link.url", TxTokenType::TxMarkupLinkUrl},

	    {"markup.raw", TxTokenType::TxMarkupRaw},
	    {"markup.raw.block", TxTokenType::TxMarkupRawBlock},

	    {"markup.list", TxTokenType::TxMarkupList},
	    {"markup.list.checked", TxTokenType::TxMarkupListChecked},
	    {"markup.list.unchecked", TxTokenType::TxMarkupListUnchecked},


	    //Diff
	    {"diff.plus", TxTokenType::TxDiffPlus},
	    {"diff.minus", TxTokenType::TxDiffMinus},
	    {"diff.delta", TxTokenType::TxDiffDelta},
	    
	    //Tags
	    {"tag", TxTokenType::TxTag},
	    {"tag.builtin", TxTokenType::TxTagBuiltin},
	    {"tag.attribute", TxTokenType::TxTagAttribute},
	    {"tag.delimiter", TxTokenType::TxTagDelimiter},

	    // Non-highlighting captures
	    {"none", TxTokenType::TxNone},
	    {"conceal", TxTokenType::TxConceal},
	    {"spell", TxTokenType::TxSpell},
	    {"nospell", TxTokenType::TxNoSpell},

	    //Globals
	    {"foreground", TxTokenType::TxForeground},
	    {"background", TxTokenType::TxBackground},
	    {"caret", TxTokenType::TxCaret},
	    {"blockcaret", TxTokenType::TxBlockCaret},
	    {"linehighlight", TxTokenType::TxLineHightlight},
	    {"missspelling", TxTokenType::TxMissSpelling},
	    {"selection", TxTokenType::TxSelection},
	    {"selectioninactive", TxTokenType::TxSelectionInActive},
	    {"highlight", TxTokenType::TxHighlight},
	    {"findhighlight", TxTokenType::TxFindHighlight}
	};
	OpenGL::ScopedTimer timer("CoreSystem::Init");
	Get().mCaptureToColor=LoadGruvboxColors("./data/gruvbox.json");

	Get().mTokenToColor.resize((size_t)TxTokenType::TxSize);
	// Populate from JSON
	// Get().mTokenToColor[(size_t)TxTokenType::TxDefault]=IM_COL32(255,255,255,255);
	for (auto& [jsonKey, color] : Get().mCaptureToColor) {
	    if (Get().mCaptureToTokenType.find(jsonKey) != Get().mCaptureToTokenType.end()) {
	    	size_t idx=(size_t)Get().mCaptureToTokenType[jsonKey];
	    	// GL_INFO("key:{}, color:{}, size:{}, idx:{}",jsonKey,color,Get().mTokenToColor.size(),idx);
	        Get().mTokenToColor[idx] = color;
	    }
	}


}

// Convert hex color string to ImU32
ImU32 ThemeManager::HexToImU32(const std::string& hex) {
    unsigned int color = 0;
    if (hex[0] == '#') {
        color = std::stoul(hex.substr(1), nullptr, 16);
    }
    // Construct the ImU32 value
    return IM_COL32((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, 255);
}


std::unordered_map<std::string, ImU32> ThemeManager::LoadGruvboxColors(const std::string& filename) {
    std::unordered_map<std::string, ImU32> colorMap;
    std::ifstream file(filename);
    if (!file.good()) {
        throw std::runtime_error("Failed to open JSON file");
    }
    GL_INFO("Gruvbox Color Scheme Loaded");

    nlohmann::json jsonData;
    file >> jsonData;

    // Convert variables to ImU32 and store in colorMap
    if (jsonData.contains("variables")) {
        for (auto& [key, value] : jsonData["variables"].items()) {
            colorMap[key] = HexToImU32(value.get<std::string>());
        }
    }

    // Map globals to their respective colors from variables
    if (jsonData.contains("globals")) {
        for (auto& [key, value] : jsonData["globals"].items()) {
            if (colorMap.find(value) != colorMap.end()) {
                colorMap[key] = colorMap[value];
            }
        }
    }


    if (jsonData.contains("rules")) {
        for (const auto& rule : jsonData["rules"]) 
        {
            if (rule.contains("capture") && rule.contains("color")) 
            {
                std::string colorKey = rule["color"].get<std::string>();
                if (colorMap.find(colorKey) != colorMap.end()) 
                {
                    std::istringstream ss(rule["capture"].get<std::string>());
                    std::string capture;
                    while (std::getline(ss, capture, ',')) 
                    {
                        colorMap[capture] = colorMap.at(colorKey);
                    }
                }
            }
        }
    }

    return std::move(colorMap);
}