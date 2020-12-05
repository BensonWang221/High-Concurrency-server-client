/*==================================================================================================
   Date                        Description of Change
30-Nov-2020           1. ① First version
                         ② 设置Task类，用于分离send函数到独立线程中
05-Dec-2020           1. 使用std::function来代替原有class
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
#include "CELLClient.hpp"

class CellTaskServer
{
	using CellTask = std::function<void()>;

public:
	inline void AddTask(CellTask task)
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
				task();
			}
			_tasksList.clear();
		}
	}

private:
	std::list<CellTask> _tasksList;
	std::list<CellTask> _tasksBuf;
	std::mutex _mutex;
};


// 执行任务的服务类

#endif