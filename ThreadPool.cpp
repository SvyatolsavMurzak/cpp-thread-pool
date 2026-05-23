#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>
#include <vector>  

struct ThreadPool {
    std::vector<std::thread> workers; 
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop = false;
};

void work(ThreadPool* pool) { 
    while (true) { 
        std::unique_lock<std::mutex> lock(pool->queue_mutex);
        
        pool->cv.wait(lock, [pool] { return !pool->tasks.empty() || pool->stop; });

        if (pool->stop && pool->tasks.empty()) {
            return; 
        }

        std::function<void()> task = std::move(pool->tasks.front());
        pool->tasks.pop(); 

        lock.unlock();
        
        task();
    }
}

void submit(ThreadPool* pool, std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(pool->queue_mutex);
        if (pool->stop) return; 
        pool->tasks.push(task);
    }
    pool->cv.notify_one();
}

void shutdown(ThreadPool* pool) {
    {
        std::lock_guard<std::mutex> lock(pool->queue_mutex);
        pool->stop = true;
    }
    pool->cv.notify_all(); 
    
    for (std::thread &thread : pool->workers) {
        if (thread.joinable()) {
            thread.join(); 
        }
    }
}

void task_A() { std::cout << "Задача А выполняется потоком: " << std::this_thread::get_id() << std::endl; }
void task_B() { std::cout << "Задача B выполняется потоком: " << std::this_thread::get_id() << std::endl; }

int main() {
    ThreadPool my_pool;

    int threads_count = 4;
    for (int i = 0; i < threads_count; ++i) {
        my_pool.workers.emplace_back(work, &my_pool);
    }

    submit(&my_pool, task_A);
    submit(&my_pool, task_B);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    shutdown(&my_pool);

    std::cout << "Пул остановлен. Программа завершена!" << std::endl;
    return 0;
}
