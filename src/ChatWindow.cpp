#include "pch.h"
#include "Log.h"
#include "FontAwesome6.h"
#include "imgui_internal.h"
#include "ChatWindow.h"
#include "imgui.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include "imgui_md.h"
#define IMSPINNER_DEMO
#include "imspinner.h"
#include "nlohmann/json.hpp"

ChatWindow::ChatWindow() : scrollToBottom(false) {
    mChatManager.SetBaseURL("localhost",8000);
    LoadConversationHistory();
}
ChatWindow::~ChatWindow() {
    StopServer();
}
int ChatWindow::codeBlockNumber=0;

struct my_markdown : public imgui_md 
{
    ImFont* get_font() const override
    {
        auto& fonts=ImGui::GetIO().Fonts->Fonts;
        if (m_is_table_header) {
            return fonts[4];
        }

        if (m_is_em) { // Italic text
            return fonts[6];
        }

        if(m_is_code){
            return fonts[0];
        }
        if(m_is_strong)
            return fonts[7];

        switch (m_hlevel)
        {
        case 0:
            return m_is_strong ? fonts[4] : fonts[3];
        case 1:
            return fonts[5];
            break;
        case 2:
            return fonts[6];
        default:
            return fonts[4];
        }
    };

    void open_url() const override
    {
        //platform dependent code
        // SDL_OpenURL(m_href.c_str());
    }

    // bool get_image(image_info& nfo) const override
    // {
    //     //use m_href to identify images
    //     nfo.texture_id = g_texture1;
    //     nfo.size = {40,20};
    //     nfo.uv0 = { 0,0 };
    //     nfo.uv1 = {1,1};
    //     nfo.col_tint = { 1,1,1,1 };
    //     nfo.col_border = { 0,0,0,0 };
    //     return true;
    // }

    void html_div(const std::string& dclass, bool e) override
    {
        if (dclass == "red") {
            if (e) { // Opening <div class="red">
                m_table_border = false;
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
            } else { // Closing </div>
                ImGui::PopStyleColor();  // Ensure PopStyleColor is always called
                m_table_border = true;
            }
        }
    }
    void SPAN_CODE(bool e) override
    {
        if (e)
        {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::SameLine(0.0f,0.0f);  // Prevent new line
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 255));  // Orange text
        }
        else 
        {
            ImGui::PopStyleColor();
            ImGui::PopFont();
        }
    }

    void BLOCK_CODE(const MD_BLOCK_CODE_DETAIL* details, bool e) override
    {
        static bool m_shouldcopy=false;
        if (e)
        {
            m_last_code_block_text.clear();
            ImGui::Text("");

            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            std::string idtitle=std::string("##CodeBlockDetails"+std::to_string(ChatWindow::GetCodeBlockNumber()));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0.0f,0.0f));
            ImGui::BeginChild(idtitle.c_str(),ImVec2(-FLT_MIN, 33.0f),ImGuiChildFlags_FrameStyle);
                ImGui::Text("%s", std::string(details->lang.text,details->lang.text+details->lang.substr_offsets[1]).c_str());
                ImGui::SameLine();
                const float availWidth=ImGui::GetContentRegionAvail().x;
                ImGui::Dummy({availWidth-75.0f,10.0f});
                ImGui::SameLine();
                if(ImGui::Button(ICON_FA_COPY" Copy",{75.0f,-1}))
                    m_shouldcopy=true;

            ImGui::EndChild();

            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 170, 0, 255)); // Green for block code
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(30, 30, 30, 255)); // Dark gray background
            std::string id=std::string("##CodeBlock"+std::to_string(ChatWindow::GetCodeBlockNumber()));
            ImGui::BeginChild(id.c_str(),ImVec2(-FLT_MIN, 0.0f), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
            ImGui::PopStyleVar();
        }
        else
        {
            ImGui::PopStyleColor(2);
            ImGui::SetCursorPos({ImGui::GetWindowWidth()-60.0f,10.0f});
            if(m_shouldcopy){
                ImGui::SetClipboardText(m_last_code_block_text.c_str());
                m_shouldcopy=false;
            }
            ImGui::EndChild();
            ImGui::PopFont();
        }
    }
    bool m_in_table = false;
    bool m_in_table_row = false;
    int m_column_count = 0;
    int m_current_column = 0;
    std::string m_current_table_id;
    static int table_counter;

    void BLOCK_TABLE(const MD_BLOCK_TABLE_DETAIL* detail, bool enter) override {
            if (enter) {
                // GL_INFO("BLOCK_TABLE");
                m_in_table = true;
                m_current_table_id = "MarkdownTable_" + std::to_string(table_counter++);
                
                m_column_count = detail->col_count;
                
                ImGui::BeginTable(m_current_table_id.c_str(), m_column_count, 
                    ImGuiTableFlags_Borders | 
                    ImGuiTableFlags_RowBg);
            } else {
                // GL_INFO("BLOCK_TABLE [X]");
                ImGui::EndTable();
                m_in_table = false;
                m_column_count = 0;
            }
        }

        void BLOCK_TR(bool enter) override {
            if (enter) {
                // GL_INFO("BLOCK_TR");
                m_in_table_row = true;
                m_current_column = 0;
                ImGui::TableNextRow();
            } else {
                // GL_INFO("BLOCK_TR [X]");
                m_in_table_row = false;
            }
        }

        void BLOCK_TH(const MD_BLOCK_TD_DETAIL* detail, bool enter) override {
            if (enter) {
                // GL_INFO("BLOCK_TH");
                ImGui::TableSetColumnIndex(m_current_column++);
                // Apply header styling
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                // ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[4]);
            } else {
                // GL_INFO("BLOCK_TH [X]");
                // ImGui::PopFont();
                ImGui::PopStyleColor();
            }
        }

        void BLOCK_TD(const MD_BLOCK_TD_DETAIL* detail, bool enter) override {
            if (enter) {
                // GL_INFO("BLOCK_TD");
                ImGui::TableSetColumnIndex(m_current_column++);
            }else{
                // GL_INFO("BLOCK_TD [X]");
            }
        }
};

int my_markdown::table_counter=0;
void ChatWindow::RenderChatMessage(const char* text) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiStyle& style=ImGui::GetStyle();
    float parentWidth = ImGui::GetContentRegionAvail().x;
    float chatWidth = parentWidth * 0.8f;  // 80% of parent width
    float textWidth=ImGui::CalcTextSize(text).x;
    const std::string uid=std::to_string((size_t)text);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);  // Rounded corners
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 10));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));  // Background color
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.5f, 1.0f));    // Border color

    if(textWidth < chatWidth)
        chatWidth=textWidth+style.WindowPadding.x*2.0f;

    ImGui::InvisibleButton(std::string(std::string("##bnt")+uid.c_str()).c_str(),{parentWidth-chatWidth-10.0f-50.0f,10.0f});
    ImGui::SameLine();
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::PushStyleColor(ImGuiCol_Button,ImGuiCol_WindowBg);
    ImGui::PushStyleColor(ImGuiCol_Text,ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    if(ImGui::Button(std::string(ICON_FA_PEN"##"+uid).c_str())){
        std::strcpy(userInput,text);
    }
    ImGui::PopFont();
    ImGui::PopStyleColor(2);
    ImGui::SameLine();

    ImGui::BeginChild((std::string("ChatMessage")+std::to_string((size_t)text)).c_str(), ImVec2(chatWidth, 0),ImGuiChildFlags_AutoResizeY |ImGuiChildFlags_AlwaysUseWindowPadding|ImGuiChildFlags_FrameStyle);

    ImGui::TextWrapped("%s", text);  // Render chat text

    ImGui::EndChild();

    ImGui::PopStyleColor(2);  // Restore colors
    ImGui::PopStyleVar(2);    // Restore styles
}

void ChatWindow::RenderError(const char* aErrorMessage){
    const float textWidth=ImGui::CalcTextSize(aErrorMessage).x+65.0f;
    ImGui::InvisibleButton("##errorinvisible",{ImGui::GetWindowWidth()-textWidth,10.0f});
    ImGui::SameLine();
    ImGui::BeginChild("##Error");
    ImGui::TextColored(ImVec4(0.897f,0.140f,0.367f,1.000f),ICON_FA_CIRCLE_EXCLAMATION);
    ImGui::SameLine();
    ImGui::Text("%s", aErrorMessage);
    ImGui::EndChild();
}

std::vector<std::string> llm_models = {
    "gemini-2.0-flash",
    // "llama3.2"
};
static int selected_model_idx = 0;

void ChatWindow::Render() {
    static my_markdown markdownRenderer;
    ChatWindow::ResetCodeBlockNumber();
    int childNo=0;
#ifdef GL_DEBUG
    static bool loadTestMarkdown=false;
    if(loadTestMarkdown){
        loadTestMarkdown=false;
        std::ifstream file("response.txt",std::ios::binary);
        std::ostringstream data;
        data << file.rdbuf();
        file.close();
        chatHistory.emplace_back("Give a markdown sample",ChatMessageType::User,mChatID++);
        chatHistory.emplace_back(data.str(),ChatMessageType::Assistant,mChatID++);
    }
    // ImGui::Begin("Parsed Text");
    // ImGui::TextWrapped("%s", markdownRenderer.get_parsed_text().c_str());
    // ImGui::End();
#endif


    // ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
    // ImGui::SetNextWindowPos({0,0});
    if (ImGui::Begin("Chat Window", 0, ImGuiWindowFlags_NoDecoration))
    {

        static bool initialized = [&] {
            GL_INFO("Initialized");
            std::thread server_thread(&ChatWindow::StartServer,this);
            server_thread.detach();
            mFuture=std::async(std::launch::async,&ChatWindow::MakeRequest,this,std::string("INIT"));
            return true;
        }();


        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && ImGuiIO().MouseWheel != 0.0f) {
            GL_INFO("Scrolling");
            scrollToBottom=false;
        }

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[8]);
        ImGui::Text("LLM Model:");
        ImGui::SameLine();
        ImGui::PushItemWidth(200.0f);
        if (ImGui::BeginCombo("##LLM_Model", llm_models[selected_model_idx].c_str())) {
            for (int i = 0; i < llm_models.size(); i++) {
                bool is_selected = (selected_model_idx == i);
                if (ImGui::Selectable(llm_models[i].c_str(), is_selected)) {
                    selected_model_idx = i;  // Update selected model
                    mCurrentModel=llm_models[selected_model_idx];
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();  // Highlight selected item
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::Text("Use Project Context:");
        ImGui::SameLine();
        ImGui::Checkbox("##usecontext",&mUseVectorDB);
        ImGui::PopFont();

        if(ImGui::BeginChild("ChatHistory", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 60), true))
        {
            {
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[3]);
                std::scoped_lock<std::mutex> lock(chatMutex);
                for (const auto& msg : chatHistory) {
                    if (msg.type == ChatMessageType::User) {
                        ImGui::Dummy({-1,10.0f});
                        RenderChatMessage(msg.text.c_str());
                    }else{
                        markdownRenderer.print(msg.text.c_str(), msg.text.c_str() + msg.text.size());
                    }
                }
                ImGui::PopFont();
            }
            if(this->hasError){
                RenderError("Unable to reach the server!");
                ImGui::SetScrollY(ImGui::GetScrollMaxY());
            }


            if(mChatManager.mIsGettingResponse){
                HandleStream();
                if(scrollToBottom)
                    ImGui::SetScrollY(ImGui::GetScrollMaxY());
            }
            ImGui::InvisibleButton("##spacing", {100.0,100.0f});

            static int isScrolledToBottom=0;
            if(isScrolledToBottom < 5){
                ImGui::SetScrollY(ImGui::GetScrollMaxY());
                isScrolledToBottom++;
            }
            
        }
        ImGui::EndChild();

        ImGui::PushItemWidth(-80);
        if (ImGui::InputTextMultiline("##Input", userInput,1024,{0,-1},ImGuiInputTextFlags_EnterReturnsTrue)) {
            this->SendMessage();
        }
        ImGui::PopItemWidth();
        
        ImGui::SameLine();
        if (ImGui::Button("Send",{-1,-1})) {
            this->SendMessage();
        }

        if(!mChatManager.GetMessageText().empty()){
            const std::string& msg=mChatManager.GetMessageText();
            const float bannerHeight=40.0f;
            const float bannerWidth=50.0f+ImGui::CalcTextSize(msg.c_str()).x;
            ImGui::SetCursorPos(ImVec2((ImGui::GetWindowWidth()-bannerWidth)*0.5f, ImGui::GetWindowHeight()-bannerHeight-110.0f)); // Set static position inside the main window

            // Child window that stays fixed
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,5.0f);
            ImGui::PushStyleColor(ImGuiCol_ChildBg,IM_COL32(30,30,30,255));
            ImGui::BeginChild("FixedChild", ImVec2(bannerWidth, bannerHeight),  1,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMouseInputs);
            ImSpinner::SpinnerArcRotation("##LoadingSpinner",10.0f,5,{1.0f,1.0f,1.0f,1.0f},4.0f,4,3);
            ImGui::SameLine();
            ImGui::TextWrapped("%s",msg.c_str());
            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }
        // ImSpinner::demoSpinners();

    }
    ImGui::End();

}

void ChatWindow::HandleStream(){
    std::scoped_lock<std::mutex> lock(chatMutex);
	if (!chatHistory.empty()) {
		ChatMessage& msg=chatHistory.back();
		msg.text=mChatManager.GetResponse();
    }
}

void ChatWindow::MakeRequest(std::string message){
    GL_INFO("Message[WORKER-THREAD]:{}",message);
    if(message=="INIT"){
        if(!mChatManager.InitializeConnectionWithBackend())
            this->hasError=true;

        return;
    }
    if(!mChatManager.StreamResponse(mCurrentModel, message,mUseVectorDB))
        this->hasError=true;

    // mChatManager.StreamResponse("codellama:7b", message);
}

void ChatWindow::SendMessage() {
    if (strlen(userInput)==0) return;

    {
        std::scoped_lock<std::mutex> lock(chatMutex);
        chatHistory.emplace_back(userInput,ChatMessageType::User,mChatID++);
    }

    {
        std::scoped_lock<std::mutex> lock(chatMutex);
        chatHistory.emplace_back("",ChatMessageType::Assistant,mChatID++);
    }

    GL_INFO("Message[MAIN-THREAD]:{}",userInput);
    mFuture=std::async(std::launch::async,&ChatWindow::MakeRequest,this,std::string(userInput));

    userInput[0]='\0';
    scrollToBottom=true;
    this->hasError=false;
}

void ChatWindow::LoadConversationHistory(){
    std::string filePath=std::filesystem::current_path().generic_string() + "/.cache/chat_history.json";
    if(!std::filesystem::exists(filePath))
        return;

    std::ifstream file(filePath);
    nlohmann::json conversations;
    conversations << file;

    if(!conversations.is_array()){
        GL_CRITICAL("Conversation History should be an array");
        return;
    }

    chatHistory.clear();
    for (const auto& obj : conversations) {
        if(obj.contains("type") && obj["type"].is_string() && obj.contains("content") && obj["content"].is_string()){
            const std::string& type=obj["type"];
            const std::string& content=obj["content"];

            if(type=="ai")
            {
                chatHistory.emplace_back(content,ChatMessageType::Assistant,mChatID++);
            }
            else
            {
                chatHistory.emplace_back(content,ChatMessageType::User,mChatID++);
            }
        }
    }

    GL_INFO("Chat History Loaded");
}


void ChatWindow::StartServer(){
    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    ZeroMemory(&mServerProcessInfo, sizeof(PROCESS_INFORMATION));

    // Command to start the Uvicorn server
    const char* command = "cmd /C uvicorn scripts.main:app --host 0.0.0.0 --port 8000 --reload --reload-dir scripts";

    // Create the process
    if (!CreateProcessA(NULL, const_cast<char*>(command), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &mServerProcessInfo)) {
        std::cerr << "Error: Failed to start the Python server!" << std::endl;
    } else {
        std::cout << "Python server started with PID: " << mServerProcessInfo.dwProcessId << std::endl;
    }
}

void ChatWindow::StopServer(){
    if (mServerProcessInfo.hProcess) {
        std::cout << "Stopping Python server..." << std::endl;
        TerminateProcess(mServerProcessInfo.hProcess, 0);
        CloseHandle(mServerProcessInfo.hProcess);
        CloseHandle(mServerProcessInfo.hThread);
        // atexit([]() {
        //     system("taskkill /F /IM python.exe /T");
        // });
    }
}