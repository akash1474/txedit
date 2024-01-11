#include "queue"
#include "ImageTexture.h"
#include <stdint.h>


namespace MultiThreading {
	inline static bool IsRequired=false;

	class ImageLoader{
		std::vector<ImageTexture*> mCurrentImages;
		uint8_t mThreadCount=2;
		std::queue<ImageTexture*> mQueue;
		ImageLoader(){}

	public:
		ImageLoader(const ImageLoader&)=delete; //copy

		static ImageLoader* Get(){
			static ImageLoader mInstance;
			return &mInstance;
		}

		static void AddImagesToQueue(std::vector<ImageTexture*>& images);
		static void PushImageToQueue(ImageTexture* img);
		static void LoadImages();
		static void SetThreadCount(uint8_t thread_count){Get()->mThreadCount=thread_count;}
	};
}