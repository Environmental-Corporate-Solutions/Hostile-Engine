#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>  // std::packaged_task and std::future
#include "Log.h"

namespace Hostile {

    class ThreadPool {
        std::vector<std::thread> m_workers;
        std::queue<std::function<void()>> m_tasks;
        std::mutex m_queueMutex;
        std::condition_variable m_condition;
        bool m_stop;
    public:
        ThreadPool(size_t threads);
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool(const ThreadPool&&) noexcept = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool& operator=(ThreadPool&&) noexcept = delete;
        ~ThreadPool();

        template<class F, class... Args>
        auto enqueue(F&& f, Args&&... args) {
            auto task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                );
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                if (m_stop) 
                {
                    Log::Error("enqueue on stopped ThreadPool");
                }
                m_tasks.emplace([task]() { (*task)(); });
            }
            m_condition.notify_one();
            return task->get_future();
        }

        void Wait();
    };
}
