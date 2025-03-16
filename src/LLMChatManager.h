#pragma once
#include "string"

class LLMChatManager {
private:
    std::string mBaseURL;
    std::string mBuffer;
    std::string mResponseText;
    std::string mMessageText;

    static bool WriteCallback(const std::string_view& data, intptr_t userdata);
    bool HandleStreamData(const std::string_view& data);
    void ProcessJsonLine(const std::string& line);
    void ProcessJsonLineOllama(const std::string& line);
    void CreateDefaultLLMConfigJson();



public:
    bool mIsGettingResponse=false;
    LLMChatManager(const std::string& host = "localhost", int port = 11434)
        : mBaseURL("http://" + host + ":" + std::to_string(port)) {}

    void SetBaseURL(const std::string& host = "localhost", int port = 11434){
       mBaseURL="http://" + host + ":" + std::to_string(port);
    }
    std::string GetResponse(){return this->mResponseText;};
    std::string GetMessageText(){return this->mMessageText;};
    bool CheckServer();
    bool StreamResponse(const std::string& model, const std::string& prompt,bool use_vector=false);
    bool InitializeConnectionWithBackend();
};