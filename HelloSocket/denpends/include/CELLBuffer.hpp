#ifndef _CELL_BUFFER_INCLUDED
#define _CELL_BUFFER_INCLUDED

#include "Cell.h"

class CELLBuffer
{
public:
	CELLBuffer(int nSize) : _nSize(nSize)
	{
		_pBuf = new char[nSize];
	}

	~CELLBuffer()
	{
		if (_pBuf)
		{
			delete[] _pBuf;
			_pBuf = nullptr;
		}
	}

	bool Push(const char* data, int nLen)
	{
		if (_lastPos + nLen <= _nSize)
		{
			memcpy(_pBuf + _lastPos, data, nLen);
			
			if ((_lastPos += nLen) == _nSize)
				_bufFullCount++;

			return true;
		}

		_bufFullCount++;
		return false;
	}

	int WriteToSocket(const SOCKET sock)
	{
		int ret = 0;
		if (_lastPos > 0 && sock != INVALID_SOCKET)
		{
			ret = send(sock, _pBuf, _lastPos, 0);
			_lastPos = 0;
		}
		return ret;
	}

	int ReadFromSocket(const SOCKET sock)
	{
		int ret = 0;

		if (_lastPos < _nSize)
		{
			if (ret = recv(sock, _pBuf + _lastPos, _nSize - _lastPos, 0) < 0)
				return ret;

			_lastPos += ret;
		}

		return ret;
	}

	bool HasMsg()
	{
		return (_lastPos > sizeof(DataHeader) && _lastPos > (reinterpret_cast<DataHeader*>(_pBuf))->dataLength);
	}

private:
	char* _pBuf = nullptr;

	int _lastPos = 8192;

	int _nSize = 0;

	int _bufFullCount = 0;
};

#endif