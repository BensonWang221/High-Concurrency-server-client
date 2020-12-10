/*==================================================================================================
   Date                        Description of Change
05-Dec-2020           1. ��First version �Ż�����ṹ���ֿ�ʵ��Client
06-Dec-2020           1. �ڶ���send�Ļ����ϼ��϶�ʱsend
$$HISTORY$$
====================================================================================================*/

#ifndef _CELL_CLIENT_INCLUDED
#define _CELL_CLIENT_INCLUDED

#include "CELLBuffer.hpp"

// �ͻ��˶��󣬰���������
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

		// ��Ҫ���͵����ݳ���
		int sendLen = (int)(header->dataLength);
		char* data = (char*)header;

		do {
			if (_lastSendPos + sendLen > SENDBUFSIZE)
			{
				// ֻ���Կ���SENDBUFSIZE - _lastSendPos��С������
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

	// �������
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
			// ��ʱ�䳬���趨ʱ�䲢�ҷ��ͻ�������������ʱ��������������ȫ��������������_dtSend
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

	// ������ʱ
	time_t _dtHeart = 0;
	time_t _dtSend = 0;
	//CELLBuffer _sendBuf;
	char _msgBuf[RECVBUFSIZE] = { 0 };
	char _sendBuf[SENDBUFSIZE] = { 0 };
};

#endif