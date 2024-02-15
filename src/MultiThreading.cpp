#include "pch.h"
#include "MultiThreading.h"
#include "ImageTexture.h"
#include <algorithm>

void MultiThreading::ImageLoader::AddImagesToQueue(std::vector<ImageTexture*>& images){
	size_t size=images.size();
	for(ImageTexture* img:images){
		Get()->mQueue.push(img);
	}

	GL_INFO("Size:{}",Get()->mQueue.size());
	if(!Get()->mQueue.empty())
		MultiThreading::IsRequired=true;
}

void MultiThreading::ImageLoader::PushImageToQueue(ImageTexture* img){
	Get()->mQueue.push(img);

	if(!Get()->mQueue.empty())
		MultiThreading::IsRequired=true;
}

void MultiThreading::ImageLoader::LoadImages(){
	if(!MultiThreading::IsRequired) return;
	static bool isReserved=false;
	if(!isReserved){
		Get()->mCurrentImages.resize(8);
		isReserved=true;
	}

	auto& que=Get()->mQueue;
	auto& imgs=Get()->mCurrentImages;
	if(que.empty() && imgs.empty()) return;

	for(size_t i=0;i<Get()->mThreadCount;i++){
		if(!imgs[i] && !que.empty()){
			imgs[i]=que.front();
			que.pop();
		}
		ImageTexture::LoadAsync(imgs[i]);
		if(imgs[i] && imgs[i]->IsLoaded()){
			imgs[i]=nullptr;
			bool isFinished=true;
			for(size_t i=0;i<Get()->mThreadCount;i++){if(imgs[i] && !imgs[i]->IsLoaded()) isFinished=false;}
			if(isFinished && que.empty()) MultiThreading::IsRequired=false;
		}
	}
}
