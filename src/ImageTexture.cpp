#include "pch.h"
#include "imgui.h"
#include "Log.h"
#include "ImageTexture.h"
#include <chrono>
#include <gl/GL.h>
#include <thread>
#include <vcruntime.h>
#include "stb_img.h"

void ImageTexture::LoadTexture(const char* file_path){
	mImageData = stbi_load(file_path, &mSize.width, &mSize.height, &mChannels, STBI_rgb_alpha);
    if (mImageData == nullptr) {
        GL_CRITICAL("ImageTexture Loading Error:{}, FilePath:{}",stbi_failure_reason(),file_path);
        GL_ERROR("ImageData: w({}) h({})",mSize.width,mSize.height);
        return;
    }
}

void ImageTexture::BindTexture(){
	glGenTextures(1, &mTextureId);
    glBindTexture(GL_TEXTURE_2D, mTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mSize.width, mSize.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mImageData);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(mImageData);
    GL_INFO("Image:{} LOADED",mFilePath);
    mIsLoaded=true;
}

void ImageTexture::LoadAsync(ImageTexture* img){
    if(!img) return;
    if(!img->mFuture.valid())
    {
        img->mFuture=std::async(std::launch::async,&ImageTexture::LoadTexture,img,img->mFilePath.c_str());
    }

    if(!img->IsLoaded() && img->mFuture.valid() && img->mFuture.wait_for(std::chrono::milliseconds(5))==std::future_status::ready){
        img->BindTexture();
    }
}

void ImageTexture::AsyncImage(ImageTexture* img,const ImVec2& size){
    if(img->IsLoaded()) 
        ImGui::Image((ImTextureID)(intptr_t)img->GetTextureId(),size);
    else
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(img->mFilePath.c_str());
        float threshold=window->Size.x-80.0f;

        ImVec2 pos=window->DC.CursorPos;
        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb,0);
        if (!ImGui::ItemAdd(bb, id)) return;
        ImGui::GetCurrentWindow()->DrawList->AddRectFilled(pos,{pos.x+size.x,pos.y+size.y},ImColor(61,61,69,255));
    }
}


void ImageTexture::AsyncImGuiImage(ImageTexture& img,const ImVec2& size){
    if(!img.mFuture.valid())
        img.mFuture=std::async(std::launch::async,&ImageTexture::LoadTexture,&img,img.mFilePath.c_str());

    if(!img.IsLoaded() && img.mFuture.valid() && img.mFuture.wait_for(std::chrono::milliseconds(5))==std::future_status::ready)
        img.BindTexture();

    if(img.IsLoaded()){
        ImGui::Image((ImTextureID)(intptr_t)img.GetTextureId(), ImVec2(416, 256));
    }else{
        ImGui::Text("Loading..");
    }
}
