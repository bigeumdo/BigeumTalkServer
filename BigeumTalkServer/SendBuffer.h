#pragma once

class SendBufferChunk;


/**
 * \brief SendBuffer 클래스
 * \details 사용자가 전달한 데이터가 실질적으로 작성되는 버퍼입니다.
 */
class SendBuffer
{
public:
	SendBuffer(shared_ptr<SendBufferChunk> owner, BYTE* buffer, unsigned int allocSize);
	~SendBuffer();

	/** \brief 버퍼의 주소를 반환하는 함수 \return _buffer의 시작 주소 */
	BYTE* Buffer() { return _buffer; }

	/** \brief 버퍼의 할당 크기를 반환하는 함수 \return _allocSize */
	unsigned int AllocSize() { return _allocSize; }

	/** \brief 버퍼에 쓴 데이터의 크기를 반환하는 함수 \return _writeSize */
	unsigned int WriteSize() { return _writeSize; }

	void Close(unsigned int writeSize);

private:
	BYTE* _buffer;
	unsigned int _allocSize = 0;
	unsigned int _writeSize = 0;
	shared_ptr<SendBufferChunk> _owner;
};


/**
 * \brief SendBufferChunk 클래스
 * \details SendBuffer로 나눌 수 있는 큰 메모리 공간을 가지는 클래스입니다.
 * \details 청크는 쓰레드 별로(TLS) 존재하며 SendBufferManager에 의해 관리됩니다.
 */
class SendBufferChunk : public enable_shared_from_this<SendBufferChunk>
{
	enum
	{
		SEND_BUFFER_CHUNK_SIZE = 0x2000
	};

	friend class SendBufferManager;
	friend class SendBuffer;
private:
	SendBufferChunk();
	~SendBufferChunk();

	void Reset();
	shared_ptr<SendBuffer> Open(unsigned int allocSize);
	void Close(unsigned int writeSize);

	/** \brief 현재 사용 가능한 버퍼의 첫 주소를 반환합니다. \return &_buffer[_usedSize] */
	BYTE* Buffer() { return &_buffer[_usedSize]; }

	/** \brief 청크에 남은 사용 가능한 크기를 반환합니다. \return 청크에서 사용 가능한 크기 */
	unsigned int FreeSize() { return _buffer.size() - _usedSize; }

	/** \brief 청크가 열린 상태인지 여부를 반환합니다. \return _open */
	bool IsOpen() { return _open; }

private:
	vector<BYTE> _buffer;
	unsigned int _usedSize = 0;
	bool _open;
};


/**
 * \brief SendBufferMananger 클래스
 * \details 청크를 관리하는 클래스 입니다. 객체는 Global에서 생성되며, 여러 쓰레드에서 접근 가능한 공용변수가 있기에 lock을 사용합니다.
 */
class SendBufferManager
{
public:
	shared_ptr<SendBuffer> Open(unsigned int size);

private:
	shared_ptr<SendBufferChunk> Pop();
	void Push(shared_ptr<SendBufferChunk> chunk);
	static void PushGlobal(SendBufferChunk* buffer);

private:
	mutex _mutex;
	vector<shared_ptr<SendBufferChunk>> _sendBufferChunks;
};
