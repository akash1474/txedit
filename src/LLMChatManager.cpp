#include "Log.h"
#include "pch.h"
#include <cpr/cpr.h>
#include <filesystem>
#include "LLMChatManager.h"
#include "nlohmann/json.hpp"

void LLMChatManager::ProcessJsonLineOllama(const std::string& line) {
    if (line.empty()) return;
    
    try {
        nlohmann::json response_json = nlohmann::json::parse(line);

        if (response_json.contains("message") && response_json["message"].contains("content")) {
            mResponseText += response_json["message"]["content"].get<std::string>();
        }
        
    #ifdef GL_DEBUG
        if (response_json.contains("done") && response_json["done"].get<bool>()) {
            std::ofstream file("./response.txt");
            file << mResponseText;
            file.close();
        }
    #endif
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Failed to parse JSON chunk: " << e.what() << std::endl;
    }
}

void LLMChatManager::ProcessJsonLine(const std::string& line) {
    if (line.empty()) return;
    
    // GL_INFO("JSONLINE:{}",line);
    try {
        nlohmann::json response_json = nlohmann::json::parse(line);
        if(!response_json.contains("type"))
            return;

        std::string chunkType=response_json["type"].get<std::string>();
        if(chunkType=="msg"){
            mMessageText=response_json["content"].get<std::string>();
            // GL_WARN(mMessageText);
        }else if(chunkType=="response"){
            if (response_json.contains("content")) {
                mResponseText += response_json["content"].get<std::string>();
            }
    #ifdef GL_DEBUG
            if (response_json.contains("done") && response_json["done"].get<bool>()) {
                GL_INFO("Response Ended");
                std::ofstream file("./response.txt");
                file << mResponseText;
                file.close();
            }
    #endif

        }
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Failed to parse JSON chunk: " << e.what() << std::endl;
    }
}

bool LLMChatManager::WriteCallback(const std::string_view& data, intptr_t userdata) {
    auto* client = reinterpret_cast<LLMChatManager*>(userdata);
    // GL_INFO("Response:{}",data);
    return client->HandleStreamData(data);
}

// Handle incoming stream data
bool LLMChatManager::HandleStreamData(const std::string_view& data) {
    // GL_INFO("Response:{}",mResponseText);
    GL_INFO("Getting Response");
    mIsGettingResponse=true;
    mBuffer.append(data.data(), data.size());

    ProcessJsonLine(mBuffer);
    mBuffer.clear();
    
    return true;
}


bool LLMChatManager::CheckServer() {
    try {
        auto r = cpr::Get(cpr::Url{mBaseURL + "/api/version"});
        return r.status_code == 200;
    } catch (const std::exception& e) {
        std::cerr << "Error checking server: " << e.what() << std::endl;
        return false;
    }
}

void LLMChatManager::CreateDefaultLLMConfigJson(){
    GL_WARN("Creating Default Config File");
    const char* filename = "llmconfig.json";
    std::ofstream file(filename);

    if (!file) {
        GL_ERROR("Failed to create LLMConfig.json file");
        return;
    }

    file << R"({
    "ignored_folders": [
        "node_modules",
        ".git",
        ".vs",
        ".cache",
        ".venv",
        "env",
        ".cache",
        "__pycache__",
        "build",
        "dist",
        "packages",
        "assets",
        "db"
    ],
    "file_extensions": [
        ".py",
        ".java",
        ".css",
        ".js",
        ".c",
        ".cpp",
        ".ts",
        ".html",
        ".h",
        ".hpp"
    ]
})";

    file.close();
    GL_INFO("Default LLMConfig file created!");
}



bool LLMChatManager::InitializeConnectionWithBackend(){
    try {
        mMessageText="Initializing Server";
        cpr::Session session;
        nlohmann::json config_json={};
        std::string configFileName="llmconfig.json";
        if(std::filesystem::exists(configFileName))
        {
            GL_INFO("Config File Found!");
            std::ifstream config(configFileName);
            config_json << config;
        }
        else
        {
            CreateDefaultLLMConfigJson();
            std::ifstream config(configFileName);
            config_json << config;
        }

        config_json["project_dir"]=std::filesystem::current_path().generic_string();
        // nlohmann::json payload = {
        //     {"ignored_folders", {"node_modules", ".git", ".vs", ".cache", ".venv", "env", ".cache", "__pycache__", "build", "dist", "packages", "assets", "db",}},
        //     {"file_extensions",{}},
        //     {"project_dir",std::filesystem::current_path().generic_string()}
        // };
        session.SetUrl(cpr::Url{mBaseURL + "/api/chat/init"});
        session.SetHeader({{"Content-Type", "application/json"}});
        session.SetBody(config_json.dump());
        
        mBuffer.clear();
        session.SetWriteCallback(
            cpr::WriteCallback{WriteCallback, reinterpret_cast<intptr_t>(this)}
        );

        std::this_thread::sleep_for(std::chrono::seconds(5));
        // Make the request
        auto response = session.Post();
        mIsGettingResponse = false;
        mMessageText.clear();

        if (response.status_code != 200) {
            GL_CRITICAL("Error: Server returned status {}",response.status_code);
            if (!response.text.empty()) {
                GL_CRITICAL("Response: {}",response.text);
            }
            return false;
        }

        GL_INFO("Response complete!");
        return true;

    } catch (const std::exception& e) {
        GL_CRITICAL("Error making request: {}",e.what());
        return true;
    }

}

bool LLMChatManager::StreamResponse(const std::string& model, const std::string& prompt,bool use_vector) {
    this->mResponseText.clear();
    try {
        cpr::Session session;
        nlohmann::json payload = {};
        if(model=="llama3.2"){
            payload={
                {"model",model},
                {"messages", {
                    {{"role", "user"}, {"content", prompt}}
                }},
                {"stream",true}
            };
        } else{
            payload={
                {"query",prompt},
                {"use_vector",use_vector}
            };
        }

        
        session.SetUrl(cpr::Url{mBaseURL + "/api/chat"});
        session.SetHeader({{"Content-Type", "application/json"}});
        session.SetBody(payload.dump());
        
        mBuffer.clear();
        session.SetWriteCallback(
            cpr::WriteCallback{WriteCallback, reinterpret_cast<intptr_t>(this)}
        );

        // Make the request
        auto response = session.Post();
        mIsGettingResponse = false;
        mMessageText.clear();

        if (response.status_code != 200) {
            GL_CRITICAL("Error: Server returned status {}",response.status_code);
            if (!response.text.empty()) {
                GL_CRITICAL("Response: {}",response.text);
            }
            return false;
        }

        GL_INFO("Response complete!");
        return true;

    } catch (const std::exception& e) {
        GL_CRITICAL("Error making request: {}",e.what());
        return true;
    }
}