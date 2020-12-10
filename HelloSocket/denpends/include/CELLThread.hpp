/*==================================================================================================
   Date                        Description of Change
07-Dec-2020           1. �� First version����װthread,�����ӿڹ��û������߳�����
                            ע��ص�����EventCall��this��Ϊ����������
08-Dec-2020           1. �޸��߳��޷��˳���bug��_sem��waitǰû�а���unlock���γ�������
                         ����취����_isrunning��Ϊatomic, ������waitǰ����unlock
						
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
	// EventCall ��Ҫ����thread��Ϊ������Ϊ���ڻص��������������һ���߳�
	// ����һ��class�ж���һ��thread��Ա����ͬ����
	// �����ڻص����������ֲ�ͬ��thread
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
		// ��ֹ��ͬ�߳�ͬʱ����thread
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
	
	// ���Close��Ҫ�ڲ�ͬ��OnWork���߳��е���
	// ��OnWorkͬһ���̵߳��û��������
	void Close()
	{
		if (_isRunning)
		{
			_isRunning = false;
		
			// �ȴ������߳�ִ�����
			_sem.Wait();
		}
	}

	// �ڹ����������˳�Ӧ�õ��ô˺���
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

		// �������߳̽���ʱ�Ż��ѣ��ر��߳�
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