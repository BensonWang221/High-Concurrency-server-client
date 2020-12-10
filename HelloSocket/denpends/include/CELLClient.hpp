/*==================================================================================================
   Date                        Description of Change
05-Dec-2020           1. ①First version 优化代码结构，分开实现Client
06-Dec-2020           1. 在定量send的基础上加上定时send
$$HISTORY$$
====================================================================================================*/

#ifndef _CELL_CLIENT_INCLUDED
#define _CELL_CLIENT_INCLUDED

#include "CELLBuffer.hpp"

// 客户端对象，包含缓冲区
class Client
{
public:
	//Client() : _sendBuf(SENDBUFSIZE) {}

	virtual ~Client()
	{
		if (_sockFd != INVALID_SOCKET)
		{
			closesocket(_sockFd);
			_sockFd = INVALID_SOCKET;
		}
	}

	SOCKET GetSockFd() const
	{
		return _sockFd;
	}

	char* GetMsgBuf()
	{
		return _msgBuf;
	}

	SOCKET SetSockFd(const SOCKET sock)
	{
		return (_sockFd = sock);
	}

	size_t GetLastRecvPos() const
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
					//printf("Client<%d> has disconnected...\n");
					return ret;
				}
				data += copyLen;
				sendLen -= copyLen;
				_lastSendPos = 0;
				_dtSend = 0;
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

		//if (_sendBuf.Push(reinterpret_cast<const char*>(header), sendLen))
		//{
		//	ret = sendLen;
		//}

		return ret;
	}

	void ResetDtHeart()
	{
		_dtHeart = 0;
	}

	// 心跳检测
	bool HeartCheck(time_t dt)
	{
		if ((_dtHeart += dt) >= CLIENT_HEART_DEAD_TIME)
		{
			printf("Client<%d> heart check dead, time = %lld\n", _sockFd, _dtHeart);
			return true;
		}

		return false;
	}

	void CheckSendTime(time_t dt)
	{
		if ((_dtSend += dt) >= CLIENT_SEND_CHECK_TIME)
		{
			// 当时间超过设定时间并且发送缓冲区中有数据时，将缓冲区数据全部发出，并重置_dtSend
			if (_lastSendPos > 0)
			{
				if (send(_sockFd, _sendBuf, _lastSendPos, 0) < 0)
					//printf("Client<%d> has disconnected...\n", _sockFd);
					_lastSendPos = 0;
			}
			_dtSend = 0;
		}
	}

private:
	SOCKET _sockFd = INVALID_SOCKET; // client socket
	size_t _lastRecvPos = 0;
	size_t _lastSendPos = 0;

	// 心跳计时
	time_t _dtHeart = 0;
	time_t _dtSend = 0;
	//CELLBuffer _sendBuf;
	char _msgBuf[RECVBUFSIZE] = { 0 };
	char _sendBuf[SENDBUFSIZE] = { 0 };
};

#endif