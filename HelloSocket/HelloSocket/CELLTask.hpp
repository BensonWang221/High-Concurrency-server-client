/*==================================================================================================
   Date                        Description of Change
30-Nov-2020           1. ① First version
                         ② 设置Task类，用于分离send函数到独立线程中
$$HISTORY$$
====================================================================================================*/
#ifndef _CELL_TASK_INCLUDED
#define _CELL_TASK_INCLUDED

#include <mutex>
#include <thread>
#include <iostream>
#include <list>
#include <functional>
#include <memory>

extern class CellSendMsgTask;


// 任务基类
class CellTask
{
public:
	CellTask() {}

	virtual  ~CellTask() {}

	virtual void DoTask() {}

private:
};

using CellTaskPtr = std::shared_ptr<CellTask>;

class CellTaskServer
{
public:
	inline void AddTask(CellTaskPtr task)
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
			}
			_tasksList.clear();
		}
	}

private:
	std::list<CellTaskPtr> _tasksList;
	std::list<CellTaskPtr> _tasksBuf;
	std::mutex _mutex;
};


// 执行任务的服务类

#endif