#include "pch.h"
#include "Iocp.h"

IocpEvent::IocpEvent(EventType type)
	: _type(type)
{
	Init();
}


/**
 * \brief Overlapped 객체를 초기화 하는 함수
 */
void IocpEvent::Init()
{
	hEvent = nullptr;
	Internal = 0;
	InternalHigh = 0;
	Offset = 0;
	OffsetHigh = 0;
}

Iocp::Iocp()
{
	// CP 생성
	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
}

Iocp::~Iocp()
{
	CloseHandle(_iocpHandle);
}


/**
 * \brief 소켓을 CP에 연결
 * \param iocpObject CP에 연결할 iocpObject(Session, Listener)
 * \return 연결 성공 여부
 */
bool Iocp::Register(shared_ptr<IocpObject> iocpObject)
{
	// _iocpHandle CP에 iocpObject 등록
	return CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, 0, 0);
}


/**
 * \brief IOCP 완료 패킷 처리 함수
 * \param timeoutMs GetQueuedCompletionStatus이 Block될 시간. 기본값은 INFINITE
 * \return 완료 패킷 처리 성공 여부
 */
bool Iocp::Dispatch(unsigned timeoutMs)
{
	DWORD numOfBytes = 0; // 송수신한 데이터의 크기
	ULONG_PTR key = 0;
	IocpEvent* iocpEvent = nullptr; // Overlapped

	// CP 완료 패킷 확인
	if (GetQueuedCompletionStatus(_iocpHandle, OUT &numOfBytes, OUT &key,
	                              OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
	{
		// 완료 패킷 처리
		shared_ptr<IocpObject> iocpObject = iocpEvent->_owner;
		// _owner에는 비동기 IO를 요청한 객체(Listener, Session)가 있음
		iocpObject->Dispatch(iocpEvent, numOfBytes);
	}
	else
	{
		int errCode = WSAGetLastError();
		switch (errCode)
		{
		case WAIT_TIMEOUT:
			return false;
		default:
			// TODO: 로그 찍기
			shared_ptr<IocpObject> iocpObject = iocpEvent->_owner;
			iocpObject->Dispatch(iocpEvent, numOfBytes);
			break;
		}
	}

	return true;
}
