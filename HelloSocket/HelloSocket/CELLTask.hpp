/*==================================================================================================
   Date                        Description of Change
30-Nov-2020           1. �� First version
                         �� ����Task�࣬���ڷ���send�����������߳���
$$HISTORY$$
====================================================================================================*/
#ifndef _CELL_TASK_INCLUDED
#define _CELL_TASK_INCLUDED

#include <mutex>
#include <thread>
#include <iostream>
#include <list>
#include <functional>

// �������
class CellTask
{
public:
	CellTask() {}

	virtual  ~CellTask() {}

	virtual void DoTask() {}

private:
};

class CellTaskServer
{
public:
	inline void AddTask(CellTask* task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(task);
	}

	void Start()
	{
		std::thread t(std::mem_fn(&CellTaskServer::OnRun), this);
		t.detach();
	}

protected:

	void OnRun()
	{
		while (true)
		{
			if (!_tasksBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);

				for (auto task : _tasksBuf)
					_tasksList.push_back(task);

				_tasksBuf.clear();
			}

			if (_tasksList.empty())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			for (auto task : _tasksList)
			{
				task->DoTask();
				delete task;
			}
			_tasksList.clear();
		}
	}

private:
	std::list<CellTask*> _tasksList;
	std::list<CellTask*> _tasksBuf;
	std::mutex _mutex;
};


// ִ������ķ�����

#endif