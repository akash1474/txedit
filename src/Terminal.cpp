#include "DataTypes.h"
#include "GLFW/glfw3.h"
#include "Log.h"
#include "Timer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "pch.h"
#include <handleapi.h>
#include <mutex>
#include "Terminal.h"
#include "regex"


// TODO:
// Separate read output to different lines and implement selection copy + rightclick copy

Terminal::Terminal() : mIsCommandRunning(false), mScrollToBottom(false),mTerminalThread(false) {
    assert(mConPTY.Initialize());
    assert(mConPTY.CreateAndStartProcess(L"cmd.exe chcp 65001"));
    StartShell();
}

Terminal::~Terminal() {
    CloseShell();
}

std::string RemoveANSISequences(const std::string& str) {
    OpenGL::ScopedTimer("Terminal::ANSI Parsing");
    std::regex ansiRegex(R"(\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~]))");
    std::regex positioningRegex(R"(\x1B\[\d+;\d+H)");
    std::string result=std::regex_replace(str,positioningRegex,"\n");
    
    
    std::string temp=result;
    result.clear();
    auto it = std::sregex_iterator(temp.begin(), temp.end(), ansiRegex);
    auto end = std::sregex_iterator();
    
    std::size_t lastPos = 0;

    for (; it != end; ++it) {
        std::smatch match = *it;
        result.append(temp, lastPos, match.position() - lastPos);
        lastPos = match.position() + match.length();
    }

    result.append(temp, lastPos, temp.size() - lastPos);

    std::regex pathRegex(R"(0;C:\\Windows\\SYSTEM32\\cmd\.exe)");
    result = std::regex_replace(result, pathRegex, "");

    return result;
}


void Terminal::Render() {
    ImGui::Begin("Terminal");
    static bool sendInterrupt=false;
    if(ImGui::IsWindowFocused() && ImGui::IsKeyDown(ImGuiKey_LeftCtrl,false) && ImGui::IsKeyPressed(ImGuiKey_C,false)){
        GL_CRITICAL("INTERRUPT");
        sendInterrupt=true;
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
    {
        std::lock_guard<std::mutex> lock(mOutputMutex);
        if(ImGui::InputTextMultiline("##toutput", (char*)mDisplayBuffer.c_str(), mDisplayBuffer.size(),{-1,-1},ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CtrlEnterForNewLine)){
            std::string command{mDisplayBuffer.substr(mPrevBufferSize).c_str()};
            mDisplayBuffer.erase(mPrevBufferSize);
            RunCommand(command);
        }
        if(ImGui::IsItemFocused() && sendInterrupt) ImGui::SetWindowFocus(NULL);
    }
    ImGui::PopStyleColor();

    if(mScrollToBottom){
        static int count=0;
        ImGuiContext& g = *GImGui;
        static const char* child_window_name = NULL;

        ImFormatStringToTempBuffer(&child_window_name, NULL, "%s/%s_%08X", g.CurrentWindow->Name, "##toutput", ImGui::GetID("##toutput"));
        ImGuiWindow* child_window = ImGui::FindWindowByName(child_window_name);
        
        ImGui::SetScrollY(child_window, child_window->ScrollMax.y);
        if(count>3){
            mScrollToBottom=false; 
            count=0;
        }
        else count++;
    }


    
    if((glfwGetTime()-mPrevTime > 0.1f) && mIsCommandRunning){
        GL_INFO("COMPLETED");

        mPrevBufferSize=mBuffer.size();
        mDisplayBuffer.resize(mPrevBufferSize+256);
        mIsCommandRunning=false;
        ImGui::SetKeyboardFocusHere(-1);
    }


    if(sendInterrupt){
        mDisplayBuffer.shrink_to_fit();
        mConPTY.SendInterrupt();
        sendInterrupt=false;
    }



    ImGui::End();
}

void Terminal::StartShell() {
    std::thread([this]() { ShellReader(); }).detach();
}

void Terminal::ShellReader() {
    static std::string buffer;
    while (!mTerminalThread) {
        buffer=mConPTY.ReadOutput();
        if(!buffer.empty()){
            mBuffer+=RemoveANSISequences(buffer);
            if(mBuffer.back()=='\x07') mBuffer.pop_back();
            std::lock_guard<std::mutex> lock(mOutputMutex);
            mDisplayBuffer=mBuffer;
            mScrollToBottom = true;
            mIsCommandRunning=true;
        }
        mPrevTime=glfwGetTime();
        buffer.clear();
    }
}




void Terminal::CloseShell() {
    mTerminalThread=true;
    mConPTY.CloseShell();
}

void Terminal::RunCommand(std::string& command) {
    if (mIsCommandRunning || command.empty()) return;
    if(command=="cls"){
        std::string dir=mDisplayBuffer.substr(mDisplayBuffer.find_last_of('\n')+1);
        mDisplayBuffer.clear();
        mBuffer.clear();
        mDisplayBuffer=dir;
        mDisplayBuffer.resize(mDisplayBuffer.size()+256);
        mScrollToBottom=true;
        return;
    }


    GL_WARN("Terminal::CMD:'{}'",command.c_str());
    command += "\r\n";
    mDisplayBuffer.shrink_to_fit();
    mConPTY.WriteInput(command);
}

