#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace dsp56k
{
	// An unfair (first consumer blocking may not be the first notified)
	// user-space semaphore.
	class BlockingSemaphore
	{
	public:
		explicit BlockingSemaphore(const int _count = 0) : m_count(_count) {}

		void notify()
		{
			Lock lock(m_mutex);
			m_count++;
			// notify the waiting thread
			m_cv.notify_one();
		}

		void wait()
		{
			Lock lock(m_mutex);

			while (m_count == 0)
			{
				// wait on the mutex until notify is called
				m_cv.wait(lock);
			}

			m_count--;
		}

	private:
		using Lock = std::unique_lock<std::mutex>;
		std::mutex m_mutex;
		std::condition_variable m_cv;
		int m_count;
	};

	// An unfair (first consumer blocking may not be the first notified)
	// user-space semaphore that avoids blocking when there are available
	// notifications.
	class Semaphore
	{
	public:
		explicit Semaphore(const int _count = 0) : m_sem(0), m_gate(_count) {}

		void notify()
		{
			int prev = m_gate.fetch_add(1, std::memory_order_release);
			if (prev >= 0)
			{
				return;
			}
			m_sem.notify();
		}

		void wait()
		{
			int prev = m_gate.fetch_sub(1, std::memory_order_acquire);
			if (prev > 0)
			{
				return;
			}
			m_sem.wait();
		}

	private:
		std::atomic<int> m_gate;
		BlockingSemaphore m_sem;
	};

	class NopSemaphore
	{
	public:
		explicit NopSemaphore(const int _count = 0) {}
		void notify() {}
		void wait() {}
	};
}; // namespace dsp56k
