#pragma once

#include <future>
#include <vector>
#include <string>
#include <mutex>
#include "LLMChatManager.h"
#include "windows.h"

enum class ChatMessageType{
    User,
    Assistant
};

struct ChatMessage {
    std::string text;
    ChatMessageType type;
    size_t chatId;
    ChatMessage(){}
    ChatMessage(std::string aText,ChatMessageType aType,size_t id):text(aText),type(aType),chatId(id){}
};

class ChatWindow {
public:
    ChatWindow();
    ~ChatWindow();

    void Render();
    void SendMessage();

    static int codeBlockNumber;
    static int GetCodeBlockNumber(){
        return codeBlockNumber++;
    }
    static void ResetCodeBlockNumber(){
        codeBlockNumber=0;
    }

private:
    size_t mChatID=0;
    bool mUseVectorDB=false;
    std::string mCurrentModel="gemini-2.0-flash";
    std::future<void> mFuture;
    LLMChatManager mChatManager;
    std::vector<ChatMessage> chatHistory;
    PROCESS_INFORMATION mServerProcessInfo;
    char userInput[2048];
    bool scrollToBottom;
    bool hasError=false;
    std::mutex chatMutex;
    void HandleStream();
    void MakeRequest(std::string message);
    void RenderChatMessage(const char* text);
    void RenderError(const char* aErrorMessage);
    void LoadConversationHistory();
    void StartServer();
    void StopServer();
};