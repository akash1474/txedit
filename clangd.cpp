#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include "../_xplayer/packages/nlohmann/nlohmann/json.hpp"  // For handling JSON

using json = nlohmann::json;

std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}

bool start_clangd(HANDLE& hInputWrite, HANDLE& hOutputRead, PROCESS_INFORMATION& procInfo) {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create pipes for stdin and stdout
    HANDLE hInputRead, hOutputWrite;
    CreatePipe(&hOutputRead, &hOutputWrite, &saAttr, 0);
    CreatePipe(&hInputRead, &hInputWrite, &saAttr, 0);

    // Ensure the write handle to the pipe for STDIN is not inherited.
    SetHandleInformation(hInputWrite, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hOutputRead, HANDLE_FLAG_INHERIT, 0);

    // Set up the start-up info for the process
    STARTUPINFOA startInfo;
    ZeroMemory(&startInfo, sizeof(startInfo));
    startInfo.cb = sizeof(STARTUPINFOA);
    startInfo.hStdError = hOutputWrite;
    startInfo.hStdOutput = hOutputWrite;
    startInfo.hStdInput = hInputRead;
    startInfo.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&procInfo, sizeof(procInfo));

    // Start clangd process
    std::string clangd_path = "clangd.exe";  // Assuming clangd is in your PATH
    return CreateProcessA(nullptr, const_cast<LPSTR>(clangd_path.c_str()), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startInfo, &procInfo);
}

json send_lsp_request(HANDLE hOutputRead, HANDLE hInputWrite, const json& request) {
    std::string request_str = request.dump();
    std::string content_length = "Content-Length: " + std::to_string(request_str.size()) + "\r\n\r\n";

    // Write request to clangd
    DWORD bytes_written;
    WriteFile(hInputWrite, content_length.c_str(), content_length.size(), &bytes_written, NULL);
    WriteFile(hInputWrite, request_str.c_str(), request_str.size(), &bytes_written, NULL);

    // Read response from clangd
    char buffer[4096];
    DWORD bytes_read;
    std::string response_str;
    while (ReadFile(hOutputRead, buffer, sizeof(buffer) - 1, &bytes_read, NULL) && bytes_read != 0) {
        buffer[bytes_read] = '\0';
        response_str += buffer;
        if (response_str.find("\r\n\r\n") != std::string::npos) break;
    }

    // Extract JSON body
    size_t pos = response_str.find("\r\n\r\n");
    if (pos != std::string::npos) {
        std::string body = response_str.substr(pos + 4);
        return json::parse(body);
    }

    return {};
}

struct SemanticToken {
    std::string type;
    size_t start;
    size_t length;
};

std::vector<SemanticToken> process_semantic_tokens(const json& response) {
    std::vector<SemanticToken> tokens;

    if (response.contains("result")) {
        auto result = response["result"];
        auto data = result["data"].get<std::vector<int>>();

        size_t index = 0;
        size_t line = 0, character = 0;
        for (size_t i = 0; i < data.size(); i++) {
            size_t delta_line = data[index++];
            size_t delta_character = data[index++];
            size_t length = data[index++];
            size_t token_type = data[index++];

            line += delta_line;
            character += delta_character;

            SemanticToken token;
            token.start = line * 10000 + character; // Combine line and character for position
            token.length = length;

            // Convert token_type to meaningful type string
            switch (token_type) {
                case 0: token.type = "namespace"; break;
                case 1: token.type = "type"; break;
                case 2: token.type = "class"; break;
                // Add other types based on clangd's token type encoding
                default: token.type = "unknown"; break;
            }

            tokens.push_back(token);
        }
    }

    return tokens;
}

json semantic_tokens_request(const std::string& uri) {
    return {
        {"jsonrpc", "2.0"},
        {"method", "textDocument/semanticTokens"},
        {"params", {
            {"textDocument", {
                {"uri", uri}
            }}
        }}
    };
}

int main() {
    std::string filename = "./clangd.cpp";
    std::string file_content = read_file(filename);
    std::string uri = "file://" + filename;

    // Set up the process and pipes for clangd
    HANDLE hInputWrite, hOutputRead;
    PROCESS_INFORMATION procInfo;
    if (!start_clangd(hInputWrite, hOutputRead, procInfo)) {
        std::cerr << "Failed to start clangd process" << std::endl;
        return 1;
    }

    // Initialize LSP
    json initialize_request = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "initialize"},
        {"params", {
            {"processId", nullptr},
            {"rootUri", nullptr},
            {"capabilities", {}}
        }}
    };
    send_lsp_request(hOutputRead, hInputWrite, initialize_request);

    // Open document
    json did_open_request = {
        {"jsonrpc", "2.0"},
        {"method", "textDocument/didOpen"},
        {"params", {
            {"textDocument", {
                {"uri", uri},
                {"languageId", "cpp"},
                {"version", 1},
                {"text", file_content}
            }}
        }}
    };
    send_lsp_request(hOutputRead, hInputWrite, did_open_request);

    // Request semantic tokens
    json tokens_request = semantic_tokens_request(uri);
    json tokens_response = send_lsp_request(hOutputRead, hInputWrite, tokens_request);

    std::vector<SemanticToken> tokens = process_semantic_tokens(tokens_response);
    for (const auto& token : tokens) {
        std::cout << "Token: " << token.type << ", Start: " << token.start << ", Length: " << token.length << "\n";
    }

    // Close handles and terminate clangd process
    CloseHandle(hInputWrite);
    CloseHandle(hOutputRead);
    TerminateProcess(procInfo.hProcess, 0);
    CloseHandle(procInfo.hProcess);
    CloseHandle(procInfo.hThread);

    return 0;
}
