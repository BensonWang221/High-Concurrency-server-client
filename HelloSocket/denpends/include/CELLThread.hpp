/*==================================================================================================
   Date                        Description of Change
07-Dec-2020           1. ① First version，封装thread,三个接口供用户设置线程任务
                            注意回调函数EventCall用this作为参数的作用
08-Dec-2020           1. 修改线程无法退出的bug，_sem在wait前没有把锁unlock，形成死锁，
                         解决办法，将_isrunning改为atomic, 或者在wait前调用unlock
						
$$HISTORY$$
====================================================================================================*/
#ifndef _CELL_THREAD_INCLUDED
#define _CELL_THREAD_INCLUDED

#include <thread>
#include <mutex>
#include <functional>
#include <atomic>
#include "CELLSemaphore.hpp"

class CELLThread
{
	// EventCall 需要传入thread作为参数，为了在回调函数里清楚是哪一个线程
	// 假如一个class有多于一个thread成员做不同任务
	// 可以在回调函数中区分不同的thread
	using EventCall = std::function<void(CELLThread*)>;

public:
	CELLThread() = default;

	CELLThread(EventCall onCreate, EventCall onRun, EventCall onDestroy)
	{
		if (onCreate)
			_onCreate = onCreate;

		if (onRun)
			_onRun = onRun;

		if (onDestroy)
			_onDestroy = onDestroy;
	}

	void Start(EventCall onCreate = nullptr, EventCall onRun = nullptr, EventCall onDestroy = nullptr)
	{
		// 防止不同线程同时调用thread
		std::lock_guard<std::mutex> locker(_mutex);

		if (onCreate)
			_onCreate = onCreate;

		if (onRun)
			_onRun = onRun;

		if (onDestroy)
			_onDestroy = onDestroy;

		if (!_isRunning)
		{
			_isRunning = true;

			std::thread t(std::mem_fn(&CELLThread::OnWork), this);
			t.detach();

		}
	}
	
	// 这个Close需要在不同于OnWork的线程中调用
	// 与OnWork同一个线程调用会造成死锁
	void Close()
	{
		if (_isRunning)
		{
			_isRunning = false;
		
			// 等待工作线程执行完毕
			_sem.Wait();
		}
	}

	// 在工作函数中退出应该调用此函数
	void Exit()
	{
		_isRunning = false;
	}

	bool IsRunning()
	{
		return _isRunning.load();
	}

protected:
	void OnWork()
	{
		if (_onCreate)
			_onCreate(this);

		if (_onRun)
			_onRun(this);

		if (_onDestroy)
			_onDestroy(this);

		// 当工作线程结束时才唤醒，关闭线程
		_sem.Wakeup();
	}

private:
	CELLSemaphore _sem;

	EventCall _onCreate = nullptr;
	EventCall _onRun = nullptr;
	EventCall _onDestroy = nullptr;
	std::mutex _mutex;

	std::atomic_bool _isRunning = false;
};

#endif