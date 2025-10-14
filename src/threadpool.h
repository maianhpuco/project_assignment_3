#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
#include <vector>

template <typename T>
class TaskQueue
{
	std::queue<T> m_queue;
	std::mutex	  m_mutex;

public:
	TaskQueue() {}

	~TaskQueue() {}

	bool empty()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_queue.empty();
	}

	int size()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_queue.size();
	}

	void enqueue(T& t)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_queue.push(t);
	}

	bool dequeue(T& t)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_queue.empty())
		{
			return false;
		}

		t = std::move(m_queue.front());
		m_queue.pop();
		return true;
	}
};

class ThreadPool
{
	bool							 m_running;
	TaskQueue<std::function<void()>> m_queue;
	std::vector<std::thread>		 m_threads;
	std::mutex						 m_conditional_mutex;
	std::condition_variable			 m_conditional_lock;

	class ThreadWorker
	{
		int			m_id;
		ThreadPool* m_pool;

	public:
		ThreadWorker(ThreadPool* pool, const int id) : m_pool(pool), m_id(id) {}

		void operator()()
		{
			std::function<void()> func;
			bool				  dequeued = false;
			while (true)
			{
				{
					std::unique_lock<std::mutex> lock(m_pool->m_conditional_mutex);
					m_pool->m_conditional_lock.wait(lock, [this]() {
						return !this->m_pool->m_queue.empty() || !this->m_pool->m_running;
					});
					
					if (!m_pool->m_running && m_pool->m_queue.empty())
						return;

					dequeued = m_pool->m_queue.dequeue(func);
				}
				if (dequeued)
				{
					func();
				}
			}
		}
	};

public:
	ThreadPool(const int n_threads)
			: m_threads(std::vector<std::thread>(n_threads))
	{
		// Construct worker slots up front; threads are launched in init() so users can
		// decide how many to spin up based on std::thread::hardware_concurrency().
	}

	~ThreadPool() {
		for (auto& worker : m_threads)
		{
			if (worker.joinable())
			{
				worker.join();
			}
		}
	}
	void init()
	{
		for (int i = 0, sz = m_threads.size(); i < sz; i++)
		{
			m_threads[i] = std::thread(ThreadWorker(this, i));
		}

		{
			std::unique_lock<std::mutex> lock(m_conditional_mutex);
			m_running = true;
		}

		m_conditional_lock.notify_one();
	}

	void shutdown()
	{
		
		{
			std::unique_lock<std::mutex> lock(m_conditional_mutex);
			m_running = false;

			std::function<void()> func;
			while (!m_queue.empty())
			{
				m_queue.dequeue(func);
			}
		}
	
		m_conditional_lock.notify_all();
	}

	template <typename F, typename... Args>
	auto submit(F&& f, Args&&... args)
	{
		std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		auto								  task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

		std::function<void()> wapper_func = [task_ptr]() {
			(*task_ptr)();
		};

		m_queue.enqueue(wapper_func);
		m_conditional_lock.notify_one();
		return task_ptr->get_future();
	}
};
