#include "pch.h"
#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(int bufferSize)
	: _capacity(bufferSize * BUFFER_COUNT), _bufferSize(bufferSize)
{
	_buffer.resize(_capacity);
}

RecvBuffer::~RecvBuffer()
{
}


/**
 * \brief read와 write 커서의 위치를 재설정 하는 함수
 */
void RecvBuffer::Clean()
{
	int dataSize = DataSize();
	if (dataSize == 0)
	{
		_readPos = _writePos = 0;
	}
	else
	{
		// 여유 공간이 버퍼 1개 크기 미만
		if (FreeSize() < _bufferSize)
		{
			memcpy(&_buffer[0], &_buffer[_readPos], dataSize);
			_readPos = 0;
			_writePos = dataSize;
		}
	}
}


/**
 * \brief _readPos를 이동 시키는 함수
 * \param numOfBytes 버퍼에서 읽은 데이터의 크기
 * \return 성공 여부
 */
bool RecvBuffer::OnRead(int numOfBytes)
{
	// 읽을 수 있는 데이터를 넘어서는 경우 실패
	if (numOfBytes > DataSize())
	{
		return false;
	}

	// read 커서의 위치를 numOfBytes 만큼 이동
	_readPos += numOfBytes;
	return true;
}


/**
 * \brief _wirtePos를 이동 시키는 함수
 * \param numOfBytes  버퍼에 쓴 데이터 크기
 * \return 성공 여부
 */
bool RecvBuffer::OnWrite(int numOfBytes)
{
	// 쓸 수 있는 공간을 넘어서는 경우 실패
	if (numOfBytes > FreeSize())
	{
		return false;
	}

	// write 커서의 위치를 numOfBytes 만큼 이동
	_writePos += numOfBytes;
	return true;
}
