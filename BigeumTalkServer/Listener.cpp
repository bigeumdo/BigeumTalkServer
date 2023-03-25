#include "pch.h"
#include "Listener.h"
#include "Service.h"
#include "SocketUtils.h"

Listener::~Listener()
{
	closesocket(_socket);
	for (AcceptEvent* acceptEvent : _acceptEvents)
	{
		delete acceptEvent;
	}
}


/**
 * \brief 리슨 소켓 설정 및 Accept 비동기 작업 요청 함수
 * \param service 리슨 소켓을 연결할 서비스
 * \return 작업 성공 여부
 */
bool Listener::StartAccept(shared_ptr<Service> service)
{
	_service = service;
	if (_service == nullptr)
	{
		return false;
	}

	// 리슨 소켓 생성
	_socket = SocketUtils::CreateSocket();
	if (_socket == INVALID_SOCKET)
	{
		return false;
	}

	// 리슨 소켓 CP에 등록
	if (_service->GetIocp()->Register(shared_from_this()) == false)
	{
		return false;
	}

	{
		// 주소 재사용 설정
		bool flag = true;
		if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&flag), sizeof(flag)) ==
			SOCKET_ERROR)
		{
			return false;
		}
	}

	// 주소 bind
	if (bind(_socket, reinterpret_cast<SOCKADDR*>(&_service->GetSockAddr()), sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		return false;
	}

	// 소켓 수신 대기
	if (listen(_socket, SOMAXCONN) == SOCKET_ERROR)
	{
		return false;
	}

	// AcceptEx 소켓 풀 생성
	const int socketPoolCount = 100;
	for (int i = 0; i < socketPoolCount; i++)
	{
		auto acceptEvent = new AcceptEvent();
		acceptEvent->_owner = shared_from_this(); // 참조 증가
		_acceptEvents.push_back(acceptEvent);
		RegisterAccept(acceptEvent);
	}


	return true;
}


/**
 * \brief 소켓을 반환하는 함수
 * \return HANDLE로 캐스팅된 _socket
 */
HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}


/**
 * \brief Iocp::Dispatch에서 호출 될 완료 패킷 처리 함수
 * \param iocpEvent 해당 완료 패킷의 Event. 정상적인 경우 AcceptEvent
 * \param numOfBytes 완료 패킷의 길이
 */
void Listener::Dispatch(IocpEvent* iocpEvent, int numOfBytes)
{
	// Accept Event Dispatch 함수
	_ASSERTE(iocpEvent->GetEventType() == EventType::Accept);

	auto acceptEvent = static_cast<AcceptEvent*>(iocpEvent);

	// 완료 패킷을 처리
	ProcessAccept(acceptEvent);
}


/**
 * \brief Accept 비동기 IO 작업 요청 함수
 * \param acceptEvent AcceptEvent 객체 주소
 */
void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
	shared_ptr<Session> session = _service->CreateSession(); // 세션을 미리 생성
	acceptEvent->Init();
	acceptEvent->_session = session; // 연결이 완료된 세션의 참조는 해제하며 새 세션 참조

	DWORD bytes = 0;
	// 비동기 IO 작업 요청
	if (false == SocketUtils::AcceptEx(_socket, session->GetSocket(), session->_recvBuffer.WritePos(), 0,
	                                   sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, OUT &bytes,
	                                   static_cast<LPOVERLAPPED>(acceptEvent)))
	{
		const int errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			RegisterAccept(acceptEvent);
		}
	}
}


/**
 * \brief Accept 완료 패킷 처리 함수
 * \param acceptEvent 완료 패킷에 있는 AcceptEvent
 */
void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
	shared_ptr<Session> session = acceptEvent->_session;

	// 연결된 소켓의 옵션을 리슨 소켓과 똑같이 함
	if (SOCKET_ERROR == setsockopt(session->GetSocket(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
	                               reinterpret_cast<char*>(&_socket), sizeof(_socket)))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	SOCKADDR_IN sockAddr;
	int sizeOfSockAddr = sizeof(sockAddr);
	// Accept한 소켓의 주소 정보 얻기
	if (SOCKET_ERROR == getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddr), &sizeOfSockAddr))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	session->SetAddress(sockAddr);
	session->ProcessConnect();

	// 다시 Accept 작업 등록
	RegisterAccept(acceptEvent);

	// TEMP LOG
#ifdef _DEBUG
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &sockAddr.sin_addr, ip, INET_ADDRSTRLEN);
	cout << "[CLIENT ACCEPTED] " << ip << ':' << ntohs(sockAddr.sin_port) << " Accepted" << endl;
#endif
}
