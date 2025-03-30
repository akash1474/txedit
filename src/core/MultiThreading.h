#pragma once

#include "queue"
#include <stdint.h>
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include "core/ImageTexture.h"



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


	class ThreadPool {
	public:
	    static ThreadPool& getInstance(size_t threads = std::thread::hardware_concurrency()) {
	        static ThreadPool instance(threads);
	        return instance;
	    }
	    
	    template<class F, class... Args>
	    auto enqueue(F&& f, Args&&... args) 
	        -> std::future<typename std::invoke_result<F, Args...>::type>;
	    
	    ~ThreadPool();
	    
	    // Delete copy and move constructors
	    ThreadPool(const ThreadPool&) = delete;
	    ThreadPool& operator=(const ThreadPool&) = delete;
	    ThreadPool(ThreadPool&&) = delete;
	    ThreadPool& operator=(ThreadPool&&) = delete;

	private:
	    ThreadPool(size_t);
	    
	    std::vector<std::thread> workers;
	    std::queue<std::function<void()>> tasks;
	    std::mutex queue_mutex;
	    std::condition_variable condition;
	    bool stop;
	};

	 

}