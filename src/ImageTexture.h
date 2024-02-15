#pragma once
#include "imgui.h"
#include <future>
#include <Windows.h>
#include <gl/GL.h>
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_CLAMP 0x2900
#define GL_CLAMP_TO_BORDER 0x812D

struct ImageSize{
	int width;
	int height;
};

class ImageTexture{
	GLuint mTextureId;
	unsigned char* mImageData=nullptr;
	ImageSize mSize;
	int mChannels;
	bool mIsLoaded=false;
	std::future<void> mFuture;
	std::string mFilePath;
		
public:
	ImageTexture(const char* path):mFilePath(path){}
	void LoadTexture(const char* img_path);
	void BindTexture();
	bool IsLoaded(){return mIsLoaded;}
	unsigned int GetTextureId(){return mTextureId;};
	static void AsyncImGuiImage(ImageTexture& img,const ImVec2& size);
	static void AsyncImage(ImageTexture* img,const ImVec2& size);
	static void LoadAsync(ImageTexture* img);
};
