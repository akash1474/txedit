#include "Log.h"
#include "Timer.h"
#include "images.h"
#include "pch.h"
#include "CoreSystem.h"
#include <cstdint>
#include <minwindef.h>
#include <processenv.h>
#include <wingdi.h>
#include "Application.h"
#include "TabsManager.h"
#include "Lexer.h"
#include "tree_sitter/api.h"
#include "tree_sitter/parser.h"

#ifndef TREE_SITTER_CPP_H_
#define TREE_SITTER_CPP_H_

typedef struct TSLanguage TSLanguage;

#ifdef __cplusplus
extern "C" {
#endif

const TSLanguage *tree_sitter_cpp(void);

#ifdef __cplusplus
}
#endif

#endif // TREE_SITTER_CPP_H_


// Callback function for EnumFontFamiliesEx
int CALLBACK EnumFontFamExProc(const LOGFONT* lpelfe, const TEXTMETRIC* lpntme, DWORD FontType, LPARAM lParam) {
    GL_INFO(std::filesystem::path(lpelfe->lfFaceName).generic_u8string().c_str());
    return 1; // Continue enumeration
}


// Function to get installed fonts
std::vector<std::string> GetInstalledFonts() {
    std::vector<std::string> fonts;
    LOGFONT lf = { 0 };
    lf.lfCharSet = DEFAULT_CHARSET;
    HDC hdc = GetDC(NULL);
    EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)&fonts, 0);
    ReleaseDC(NULL, hdc);
    return fonts;
}

// Function to read file content
std::string readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file");
    }
    return std::string((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
}

void render_text_with_highlighting(const std::string &text, const int stylecolor) {
    // Apply the style
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, stylecolor);
    // Traverse each character to handle spaces and tabs
    for (char c : text) {
        if (c == ' ') {
            std::cout << ' ';  // Render space as space
        } else if (c == '\t') {
            // Render tabs as 4 spaces (or any consistent tab size you prefer)
            std::cout << "    ";
        } else {
            std::cout << c;  // Render normal characters
        }
    }
}

void highlight_cpp_code(const std::string& source_code) {
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_cpp());

    // Parse source code
    TSTree *tree = ts_parser_parse_string(parser, nullptr, source_code.c_str(),source_code.size());
    // 3. Query syntax nodes for highlighting
    std::string query_string =R"((type_identifier) @type 
    							(comment) @comment 
    							(string_literal) @string 
    							(primitive_type) @keyword 
    							(function_declarator declarator:(_) @function) 
    							(namespace_identifier) @namespace)";

    // Define a simple query to match syntax elements
	uint32_t error_offset;
    TSQueryError error_type;
    TSQuery *query = ts_query_new(tree_sitter_cpp(), query_string.c_str(),query_string.size(), &error_offset, &error_type);

    // Check if the query was successfully created
    if (!query) {
        GL_ERROR("Error creating query at offset {}, error type: {}", error_offset, error_type);
        ts_tree_delete(tree);
        ts_parser_delete(parser);
        return;
    }

    TSQueryCursor *cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));

    

    // 4. Styles for each syntax type
    std::unordered_map<std::string, int> syntax_styles = {
        {"function", 9},  // Green for functions
        {"type", 12},      // Blue for types
        {"comment", 11},   // Grey for comments
        {"string", 10},    // Yellow for strings
        {"keyword", 13},    // Red for keywords
        {"namespace",14}
    };
    // »


    // 5. Highlight matching nodes
    TSQueryMatch match;
    unsigned long cursor_position = 0;
    while (ts_query_cursor_next_match(cursor, &match)) {
        for (unsigned int i = 0; i < match.capture_count; ++i) {
            TSQueryCapture capture = match.captures[i];
            uint32_t len;
            std::string capture_name = ts_query_capture_name_for_id(query, capture.index,&len);
            TSNode node = capture.node;

            // Extract the start and end byte positions of the node
            uint32_t start_byte = ts_node_start_byte(node);
            uint32_t end_byte = ts_node_end_byte(node);

            // Print the text before the highlighted part (Non matching part)
            render_text_with_highlighting(source_code.substr(cursor_position, start_byte - cursor_position),7);

            // Apply the style for the current capture (Matching part)
            render_text_with_highlighting(source_code.substr(start_byte, end_byte - start_byte), syntax_styles[capture_name]);

            // Update cursor position
            cursor_position = end_byte;
        }
    }

    // Print any remaining text after the last highlighted part
    render_text_with_highlighting(source_code.substr(cursor_position), 7);

    // 6. Clean up
    ts_query_cursor_delete(cursor);
    ts_query_delete(query);
    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

int main(int argc,char* argv[])
{
	OpenGL::Timer timer;
	if(!Application::Init()) return -1;
	Application::SetupSystemSignalHandling();
	Application::SetApplicationIcon(logo_img,IM_ARRAYSIZE(logo_img));


    // std::string source_code = readFile("D:/Projects/c++/txedit/src/main.cpp");
    // highlight_cpp_code(source_code);

    // TSParser *parser = ts_parser_new();
    // ts_parser_set_language(parser, tree_sitter_cpp());

    // // Parse source code
    // TSTree *tree = ts_parser_parse_string(parser, nullptr, source_code.c_str(),source_code.size());

    // // Define a simple query to match syntax elements
	// const char *query_string = "(function_declarator declarator:(_) @fn.name)";
	// uint32_t error_offset;
    // TSQueryError error_type;
    // TSQuery *query = ts_query_new(tree_sitter_cpp(), query_string, strlen(query_string), &error_offset, &error_type);

    // // Check if the query was successfully created
    // if (!query) {
    //     GL_ERROR("Error creating query at offset {}, error type: {}", error_offset, error_type);
    //     ts_tree_delete(tree);
    //     ts_parser_delete(parser);
    //     return 1;
    // }

    // TSQueryCursor *cursor = ts_query_cursor_new();
    // ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));

    // // Highlight based on query matches
    // TSQueryMatch match;
    // while (ts_query_cursor_next_match(cursor, &match)) {
    //     for (uint32_t i = 0; i < match.capture_count; i++) {
    //         TSQueryCapture capture = match.captures[i];
    //         TSNode node = capture.node;
    //         uint32_t len;
    //         uint32_t start_byte = ts_node_start_byte(node);
    //         uint32_t end_byte = ts_node_end_byte(node);

	// 		std::string symbol = source_code.substr(start_byte, end_byte - start_byte);
	// 		GL_INFO("MATCH:{}",symbol.c_str());
 			
    //     }
    // }

    // // Clean up
    // ts_query_cursor_delete(cursor);
    // ts_query_delete(query);
    // ts_tree_delete(tree);
    // ts_parser_delete(parser);



	// Initialize ImGUI
	if(!Application::InitImGui()) return -1;
	Application::GetCoreSystem()->InitFonts();

	if(argc > 1) Application::HandleArguments(GetCommandLineW());

	CoreSystem::GetFileNavigation()->AddFolder("D:/Projects/c++/txedit");
	// size_t size{0};
	// std::ifstream t("D:/Projects/c++/txedit/src/main.cpp");

	// std::string file_data{0};
	// t.seekg(0, std::ios::end);
	// size = t.tellg();
	// file_data.resize(size, ' ');
	// t.seekg(0);
	// t.read(&file_data[0], size);
	// Lexer lex(file_data);
	// lex.Tokenize();
	TabsManager::Get().OpenFile("D:/Projects/c++/txedit/src/TextEditor.cpp");
	// TabsManager::Get().OpenFile("D:/Projects/c++/txedit/src/Application.cpp");
	// TabsManager::Get().OpenFile("D:/Projects/c++/txedit/src/TabsManager.cpp");

    // std::string path = "C:\\Windows\\Fonts";
    // std::ofstream file("./win32_fontfiles.txt");
    // for (const auto& entry : std::filesystem::directory_iterator(path)) file << entry.path().generic_u8string().c_str() << std::endl;

	// std::vector<std::string> fonts = GetInstalledFonts();


	GL_WARN("BootUp Time: {}ms",timer.ElapsedMillis());
	while (!glfwWindowShouldClose(Application::GetGLFWwindow())) {
		Application::Draw();
	}

	Application::Destroy();
	return 0;
}


#ifdef _WIN32
#ifdef GL_DIST
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	std::stringstream test(lpCmdLine);
	std::string segment;
	std::vector<std::string> seglist;

	while(std::getline(test, segment, ' ')) seglist.push_back(segment);
	
	return main((int)seglist.size(),&lpCmdLine);
}
#endif
#endif
                                                                            
