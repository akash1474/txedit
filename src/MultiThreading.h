#include "queue"
#include "ImageTexture.h"


namespace MultiThreading {
	inline static bool IsRequired=false;

	class ImageLoader{
		ImageTexture* mCurrentImage;
		std::queue<ImageTexture*> mQueue;
		ImageLoader(){}

	public:
		ImageLoader(const ImageLoader&)=delete; //copy

		static ImageLoader& Get(){
			static ImageLoader mInstance;
			return mInstance;
		}

		static void AddImagesToQueue(std::vector<ImageTexture>& images);
		static void PushImageToQueue(ImageTexture& img);
		static void LoadImages();
	};
}