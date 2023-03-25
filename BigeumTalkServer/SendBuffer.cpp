#include "pch.h"
#include "SendBuffer.h"

SendBuffer::SendBuffer(shared_ptr<SendBufferChunk> owner, BYTE* buffer, unsigned allocSize)
	: _buffer(buffer), _allocSize(allocSize), _owner(owner)
{
}

SendBuffer::~SendBuffer()
{
}


/**
 * \brief 버퍼에 쓰기를 확정하고 청크에 버퍼를 반환하는 함수
 * \param writeSize 버퍼에 쓴 데이터 크기
 */
void SendBuffer::Close(unsigned writeSize)
{
	// 할당받은 주소보다 더 많이 데이터를 썻다면 ASSERT
	_ASSERTE(_allocSize >= writeSize);
	_writeSize = writeSize;
	_owner->Close(writeSize);
}


SendBufferChunk::SendBufferChunk()
{
	_buffer.resize(SEND_BUFFER_CHUNK_SIZE);
}

SendBufferChunk::~SendBufferChunk()
{
}


/**
 * \brief 청크를 초기화 하는 함수
 */
void SendBufferChunk::Reset()
{
	_open = false;
	_usedSize = 0;
}


/**
 * \brief 요청한 만큼의 버퍼를 반환하는 함수
 * \param allocSize 요청할 버퍼 크기
 * \return allocSize만큼의 쓰기가 가능한 버퍼
 */
shared_ptr<SendBuffer> SendBufferChunk::Open(unsigned allocSize)
{
	// 요청한 주소가 청크 크기보다 크거나 청크가 이미 열린 상태면 ASSERT
	_ASSERTE(allocSize <= SEND_BUFFER_CHUNK_SIZE);
	_ASSERTE(_open == false);


	if (allocSize > FreeSize())
		return nullptr;

	_open = true;
	return make_shared<SendBuffer>(shared_from_this(), Buffer(), allocSize);
}


/**
 * \brief 청크에 sendBuffer가 사용한 크기를 기록하는 함수
 * \param writeSize sendBuffer가 청크를 사용한 크기
 */
void SendBufferChunk::Close(unsigned writeSize)
{
	_ASSERTE(_open == true);
	_open = false;
	_usedSize += writeSize;
}


/**
 * \brief 사용 가능한 SendBuffer를 반환 하는 함수
 * \param size 사용할 SendBuffer의 크기
 * \return size만큼 사용할 수 있는 shared_ptr<SendBuffeR>
 */
shared_ptr<SendBuffer> SendBufferManager::Open(unsigned size)
{
	// 청크가 없다면 새 청크를 받고 초기화
	if (LSendBufferChunk == nullptr)
	{
		LSendBufferChunk = Pop();
		LSendBufferChunk->Reset();
	}

	_ASSERTE(LSendBufferChunk->IsOpen() == false);

	// 청크의 남은 사용가능한 공간이 요청한 크기보다 작다면 새 청크 받기
	if (LSendBufferChunk->FreeSize() < size)
	{
		LSendBufferChunk = Pop();
		LSendBufferChunk->Reset();
	}

	return LSendBufferChunk->Open(size);
}


/**
 * \brief 사용 가능한 새 청크를 꺼내는 함수
 * \return 사용 가능한 청크
 */
shared_ptr<SendBufferChunk> SendBufferManager::Pop()
{
	lock_guard<mutex> guard(_mutex);

	// _sendBufferChunks에 청크가 있다면 마지막 것을 반환
	if (_sendBufferChunks.empty() == false)
	{
		shared_ptr<SendBufferChunk> sendBufferChunk = _sendBufferChunks.back();
		_sendBufferChunks.pop_back();
		return sendBufferChunk;
	}

	// PushGlobal을 삭제자로 가지는 새 청크 반환
	return shared_ptr<SendBufferChunk>(new SendBufferChunk, PushGlobal);
}


/**
 * \brief 청크를 반납하는 함수
 * \param chunk 반납할 청크
 */
void SendBufferManager::Push(shared_ptr<SendBufferChunk> chunk)
{
	lock_guard<mutex> guard(_mutex);
	_sendBufferChunks.push_back(chunk);
}


/**
 * \brief shared_ptr<SendBufferChunk>의 삭제자 함수
 * \param buffer 소멸될 SendBufferChunk 객체 주소
 */
void SendBufferManager::PushGlobal(SendBufferChunk* buffer)
{
	// 참조가 다해 소멸될 buffer를 소멸 시키지 않고
	// 이후 다시 사용하도록 함
	GSendBufferManager->Push(shared_ptr<SendBufferChunk>(buffer, PushGlobal));
}
