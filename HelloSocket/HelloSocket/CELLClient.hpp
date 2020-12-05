#ifndef _CELL_CLIENT_INCLUDED
#define _CELL_CLIENT_INCLUDED

#include "Cell.h"

// 客户端对象，包含缓冲区
class Client
{
public:
	virtual ~Client()
	{
		if (_sockFd != INVALID_SOCKET)
			closesocket(_sockFd);
	}

	inline SOCKET GetSockFd() const
	{
		return _sockFd;
	}

	inline char* GetMsgBuf()
	{
		return _msgBuf;
	}

	inline SOCKET SetSockFd(const SOCKET sock)
	{
		return (_sockFd = sock);
	}

	inline size_t GetLastRecvPos() const
	{
		return _lastRecvPos;
	}

	inline void SetLastRecvPos(const size_t pos)
	{
		_lastRecvPos = pos;
	}

	int SendData(const DataHeader* header)
	{
		int ret = SOCKET_ERROR;

		// 需要发送的数据长度
		int sendLen = (int)(header->dataLength);
		char* data = (char*)header;

		do {
			if (_lastSendPos + sendLen > SENDBUFSIZE)
			{
				// 只可以拷贝SENDBUFSIZE - _lastSendPos大小的数据
				const int copyLen = SENDBUFSIZE - _lastSendPos;
				memcpy(_sendBuf + _lastSendPos, data, SENDBUFSIZE - _lastSendPos);
				ret = send(_sockFd, _sendBuf, SENDBUFSIZE, 0);

				if (ret == SOCKET_ERROR)
				{
					printf("Client<%d> has disconnected...\n");
					return ret;
				}
				data += copyLen;
				sendLen -= copyLen;
				_lastSendPos = 0;
			}
			else
			{
				memcpy(_sendBuf + _lastSendPos, data, sendLen);
				_lastSendPos += sendLen;
				sendLen = 0;
				ret = 0;
				break;
			}
		} while (true);

		return ret;
	}

	inline size_t GetLastSendPos() const
	{
		return _lastSendPos;
	}

	inline void SetLastSendPos(size_t pos)
	{
		_lastSendPos = pos;
	}

private:
	SOCKET _sockFd = INVALID_SOCKET; // client socket
	size_t _lastRecvPos = 0;
	size_t _lastSendPos = 0;
	char _msgBuf[RECVBUFSIZE] = { 0 };
	char _sendBuf[SENDBUFSIZE] = { 0 };
};

#endif