#include "pch.h"
#include "Log.h"
#include "ImageTexture.h"
#include <gl/GL.h>
#include <thread>
#include "stb_img.h"

void ImageTexture::LoadTexture(const char* file_path){
	mImageData = stbi_load(file_path, &mSize.width, &mSize.height, &mChannels, STBI_rgb_alpha);
    if (mImageData == nullptr) {
        GL_CRITICAL("ImageTexture Loading Error:{}",stbi_failure_reason());
        return;
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
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
    mIsLoaded=true;
}

void ImageTexture::LoadAsync(ImageTexture* img){
    if(!img->mFuture.valid())
        img->mFuture=std::async(std::launch::async,&ImageTexture::LoadTexture,img,img->mFilePath.c_str());

    if(!img->IsLoaded() && img->mFuture.valid() && img->mFuture.wait_for(std::chrono::milliseconds(5))==std::future_status::ready){
        img->BindTexture();
    }
}


void ImageTexture::AsyncImGuiImage(ImageTexture& img,const ImVec2& size){
    if(!img.mFuture.valid())
        img.mFuture=std::async(std::launch::async,&ImageTexture::LoadTexture,&img,img.mFilePath.c_str());

    if(!img.IsLoaded() && img.mFuture.valid() && img.mFuture.wait_for(std::chrono::milliseconds(5))==std::future_status::ready)
        img.BindTexture();

    if(img.IsLoaded()){
        ImGui::Image((void*)(intptr_t)img.GetTextureId(), ImVec2(416, 256));
    }else{
        ImGui::Text("Loading..");
    }
}
