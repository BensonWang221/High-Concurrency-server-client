/*==================================================================================================
   Date                        Description of Change
12-Dec-2020           First version, 自定义字节流

$$HISTORY$$
====================================================================================================*/
#ifndef _CELL_STREAM_INCLUDED
#define _CELL_STREAM_INCLUDED

#include <cstdint>
#include <string.h>

class CELLStream
{
public:
    // 此构造方法用于读取已有数据时
    CELLStream(char* data, int nSize, bool bDelete = false) : _pBuf(data), _nSize(nSize)
    {

    }

    // 此构造方法用于要写入数据时
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

    // nLen: 缓冲数组可以放的数量
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

    // 使用template最好把不同的地方用template泛化，公共部分提取出来，可防止因为template导致代码膨胀
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

    // 能不用template就不用template了，因为template会造成代码膨胀
    // 实际产生很多重复代码
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

        //Read storedLen时先不偏移_lastReadPos，以防读取不成功还把位置便宜了
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