/*==================================================================================================
   Date                        Description of Change
12-Dec-2020           First version, �Զ����ֽ���

$$HISTORY$$
====================================================================================================*/
#ifndef _CELL_STREAM_INCLUDED
#define _CELL_STREAM_INCLUDED

#include <cstdint>
#include <string.h>

class CELLStream
{
public:
    // �˹��췽�����ڶ�ȡ��������ʱ
    CELLStream(char* data, int nSize, bool bDelete = false) : _pBuf(data), _nSize(nSize)
    {

    }

    // �˹��췽������Ҫд������ʱ
    CELLStream(int nSize = 1024) : _nSize(nSize)
    {
        _pBuf = new char[nSize];
    }

    ~CELLStream()
    {
        if (_bDelete && _pBuf)
        {
            delete[] _pBuf;
            _pBuf = nullptr;
        }
    }

    int8_t ReadInt8()
    {
        int8_t n;
        Read(&n, sizeof(int8_t));

        return n;
    }

    int16_t ReadInt16()
    {
        int16_t n;
        Read(&n, sizeof(int16_t));

        return n;
    }

    int32_t ReadInt32()
    {
        int32_t n;
        Read(&n, sizeof(int32_t));

        return n;
    }

    float ReadFloat()
    {
        float f;
        Read(&f, sizeof(float));

        return f;
    }

    double ReadDouble()
    {
        double d;
        Read(&d, sizeof(double));

        return d;
    }

    // nLen: ����������Էŵ�����
    template <typename T>
    size_t ReadArray(T* dest, size_t nLen)
    {
        return ReadArrayActual(dest, sizeof(T), nLen);
    }

    template <typename T>
    T OnlyRead()
    {
        T result;
        OnlyReadActual(&result, sizeof(T));

        return result;
    }

    bool WriteInt8(int8_t n)
    {
        return Write(&n, sizeof(int8_t));
    }

    bool WriteInt16(int16_t n)
    {
        return Write(&n, sizeof(int16_t));
    }

    bool WriteInt32(int32_t n)
    {
        return Write(&n, sizeof(int32_t));
    }

    bool WriteFloat(float n)
    {
        return Write(&n, sizeof(float));
    }

    bool WriteDouble(double n)
    {
        return Write(&n, sizeof(double));
    }

    // ʹ��template��ðѲ�ͬ�ĵط���template����������������ȡ�������ɷ�ֹ��Ϊtemplate���´�������
    template <typename T>
    bool WriteArray(const T* data, uint32_t len)
    {
        return WriteArrayActual(data, sizeof(T) * len, len);
    }

private:
    bool Read(void* dest, size_t nLen)
    {
        if (OnlyReadActual(dest, nLen))
        {
            _lastReadPos += nLen;
            return true;
        }

        return false;
    }

    bool OnlyReadActual(void* dest, size_t nLen)
    {
        if (_lastReadPos + nLen <= _nSize)
        {
            memcpy(dest, _pBuf + _lastReadPos, nLen);
            return true;
        }

        return false;
    }

    // �ܲ���template�Ͳ���template�ˣ���Ϊtemplate����ɴ�������
    // ʵ�ʲ����ܶ��ظ�����
    bool Write(const void* data, size_t nLen)
    {
        if (_lastWritePos + nLen <= _nSize)
        {
            memcpy(_pBuf + _lastWritePos, data, nLen);
            _lastWritePos += nLen;

            return true;
        }
        return false;
    }

    size_t ReadArrayActual(void* data, size_t typeSize, size_t nLen)
    {
        uint32_t storedLen = 0;

        //Read storedLenʱ�Ȳ�ƫ��_lastReadPos���Է���ȡ���ɹ�����λ�ñ�����
        if (OnlyReadActual(&storedLen, sizeof(uint32_t)) && storedLen <= nLen)
        {
            _lastReadPos += sizeof(int32_t);
            Read(data, storedLen * typeSize);

            return storedLen;
        }

        return 0;
    }

    bool WriteArrayActual(const void* data, size_t totalSize, uint32_t len)
    {
        if (_lastWritePos + totalSize + sizeof(int32_t) <= _nSize)
        {
            WriteInt32(len);
            Write(data, totalSize);

            return true;
        }

        return false;
    }

private:
    char* _pBuf = nullptr;
    int _nSize;

    int _lastReadPos = 0;

    int _lastWritePos = 0;

    bool _bDelete = true;
};
#endif