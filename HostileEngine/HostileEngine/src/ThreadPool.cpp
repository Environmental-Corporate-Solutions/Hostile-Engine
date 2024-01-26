#include "stdafx.h"
#include "ThreadPool.h"
namespace Hostile {
	ThreadPool::ThreadPool(size_t threads) : m_stop(false)
	{
		for (size_t i{}; i < threads; ++i)
		{
			m_workers.emplace_back([this] {
				while (true)
				{
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock(this->m_queueMutex);
						this->m_condition.wait(lock, [this] { return this->m_stop || !this->m_tasks.empty(); });
						if (this->m_stop && this->m_tasks.empty()) return;
						task = std::move(this->m_tasks.front());
						this->m_tasks.pop();
					}
					task();
				}
				});
		}
	}

	// need a custom destructor to join them. 
	// since threads can't be trivially copied, needs custom copy and move operations
	ThreadPool::~ThreadPool()
	{
		{
			std::unique_lock<std::mutex> lock(m_queueMutex);
			m_stop = true;
		}
		m_condition.notify_all();
		Wait();
	}

	void ThreadPool::Wait()
	{
		for (std::thread& worker : m_workers)
		{
			worker.join();
		}
	}

}