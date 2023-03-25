#pragma once


/**
 * \brief 수신 버퍼 클래스
 * \details RecvBuffer 클래스느 다음 정책을 따릅니다.
 * \details 1. 버퍼는 요청한 크기보다 10배 크게 할당 받는다.
 * \details 2. _readPos와 _writePos가 같은 위치라면 두 위치 모두 0으로 이동한다.
 * \details 3. _writePos가 끝에 도달했다면 _readPos와 _writePos를 0부터 시작하도록 이동한다.
 */
class RecvBuffer
{
	enum { BUFFER_COUNT = 10 };

public:
	RecvBuffer(int bufferSize);
	~RecvBuffer();

	void Clean();
	bool OnRead(int numOfBytes);
	bool OnWrite(int numOfBytes);

	/** \brief 현재 읽을 위치를 반환하는 함수 \return _readPos */
	BYTE* ReadPos() { return &_buffer[_readPos]; }

	/** \brief 현재 쓸 위치를 반환하는 함수 \return _writePos */
	BYTE* WritePos() { return &_buffer[_writePos]; }

	/** \brief 현재 버퍼에 있는 데이터의 크기를 반환하는 함수 \return 데이터 크기 */
	int DataSize() { return _writePos - _readPos; }

	/** \brief 현재 데이터를 쓸 수 있는 공간의 크기를 반환하는 함수 \return 쓸 수있는 공간 크기 */
	int FreeSize() { return _capacity - _writePos; }


private:
	int _capacity = 0;
	int _bufferSize = 0;
	int _readPos = 0;
	int _writePos = 0;
	vector<BYTE> _buffer;
};
