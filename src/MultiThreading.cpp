#include "pch.h"
#include "MultiThreading.h"
#include "ImageTexture.h"
#include <algorithm>

void MultiThreading::ImageLoader::AddImagesToQueue(std::vector<ImageTexture>& images){
	size_t size=images.size();
	for(size_t i=size;i>-1;i--)
		Get().mQueue.push(&images[i]);

	if(!Get().mQueue.empty())
		MultiThreading::IsRequired=true;
}

void MultiThreading::ImageLoader::LoadImages(){
	auto& que=Get().mQueue;
	auto& img=Get().mCurrentImage;

	//Execute if img==nullptr and mQueue not empty
	if(!img && !que.empty())
		img=que.front();

	ImageTexture::LoadAsync(img);
	if(img->IsLoaded()){
		que.pop();
		img=nullptr;
		if(que.empty()) MultiThreading::IsRequired=false;
	}
}