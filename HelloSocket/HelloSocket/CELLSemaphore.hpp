#ifndef _CELL_SEMAPHORE_INCLUDED
#define _CELL_SEMAPHORE_INCLUDED

#include <mutex>
#include <atomic>
#include <condition_variable>

class CELLSemaphore
{
public:
	void Wait()
	{
		std::unique_lock<std::mutex> locker(_mutex);
		
		// 防止在调用wait前调用了wakeup
		if (--_wait < 0)
		{
			_cv.wait(locker, [this]()
				{
					return _wakeup > 0;
				});
			--_wakeup;
		}
	}

	void Wakeup()
	{
		std::lock_guard<std::mutex> locker(_mutex);
		if (++_wait <= 0)
		{
			++_wakeup;
			_cv.notify_one();
		}
	}
private:
	std::mutex _mutex;
	std::condition_variable _cv;
	int _wait = 0;
	int _wakeup = 0;
};

#endif