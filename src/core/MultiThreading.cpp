#include "pch.h"
#include "core/MultiThreading.h"
#include "core/ImageTexture.h"

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

inline MultiThreading::ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for(size_t i = 0; i < threads; ++i)
        workers.emplace_back([
            this] {
                for(;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            }
        );
}

template<class F, class... Args>
auto MultiThreading::ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
    using return_type = typename std::invoke_result<F, Args...>::type;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks.emplace([task]() { (*task)(); });
    }
    condition.notify_one();
    return res;
}

inline MultiThreading::ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}
