#ifndef _CELL_LOG_INCLUDED
#define _CELL_LOG_INCLUDED

#include <stdio.h>

class CELLLog
{
public:
	~CELLLog()
	{

	}

	static CELLLog& Instance()
	{
		static CELLLog log;
		return log;
	}

	void Info(const char* str)
	{
		printf(str);
	}

private:
	CELLLog()
	{

	}
};

#endif