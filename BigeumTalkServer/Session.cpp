#include "pch.h"
#include "Session.h"
#include "PacketHandler.h"
#include "Service.h"
#include "SocketUtils.h"
#include "User.h"
#include "Room.h"

Session::Session()
	: _recvBuffer(BUFFER_SIZE)
{
	// 세션 생성시 소켓 생성
	_socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	closesocket(_socket);

#ifdef _DEBUG
	// TEMP LOG
	SOCKADDR_IN sockAddr = _address;
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &sockAddr.sin_addr, ip, INET_ADDRSTRLEN);
	cout << "[SESSION DESTROYED] " << ip << ':' << ntohs(sockAddr.sin_port) << " Disconnected" << endl;
#endif
}


/**
 * \brief RegisterConnect를 호출하는 함수
 * \return RegisterConnect의 성공 여부
 */
bool Session::Connect()
{
	return RegisterConnect();
}


/**
 * \brief 연결이 끊기 세션을 처리하는 함수
 * \param cause 연결이 끊긴 이유
 */
void Session::Disconnect(const WCHAR* cause)
{
	if (_connected.exchange(false) == false)
	{
		return;
	}
#ifdef _DUBUG
	wcout << L"[DISCONNECT SESSION] " << << cause << endl;
#endif

	RegisterDisconnect();
}


/**
 * \brief 해당 세션에게 패킷을 보내는 함수
 * \param sendBuffer 보낼 데이터가 담긴 버퍼
 */
void Session::Send(shared_ptr<SendBuffer> sendBuffer)
{
	if (IsConnected() == false)
	{
		return;
	}

	bool registerSend = false;

	{
		// Scatter-Gather IO를 위한 큐에 메모리 버퍼 저장
		lock_guard lock(_mutex);

		_sendQueue.push(sendBuffer);

		if (_sendRegistered.exchange(true) == false)
		{
			registerSend = true;
		}
	}

	if (registerSend)
	{
		RegisterSend();
	}
}


/**
 * \brief TODO
 * \return TODO
 */
bool Session::RegisterConnect()
{
	/* TODO 분산 서버 구현시 */
	return false;
}


/**
 * \brief Disconnect 비동기 IO 작업을 요청하는 함수
 * \return 작업 요청 성공 여부
 */
bool Session::RegisterDisconnect()
{
	_disconnectEvent.Init();
	_disconnectEvent._owner = shared_from_this(); // ADD REF

	// Disconnect 비동기 IO 작업 요청
	if (false == SocketUtils::DisconnectEx(
		_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0
	))
	{
		int errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_disconnectEvent._owner = nullptr;
			return false;
		}
	}

	return true;
}


/**
 * \brief Recv 비동기 IO 작업을 요청하는 함수
 */
void Session::RegisterRecv()
{
	// Recv 비동기 IO 작업 요청
	if (_connected.load() == false)
	{
		return;
	}

	_recvEvent.Init();
	_recvEvent._owner = shared_from_this(); // ADD REF

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<CHAR*>(_recvBuffer.WritePos());
	wsaBuf.len = _recvBuffer.FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags = 0;

	// WSARecv 비동기 IO 작업 요청
	if (SOCKET_ERROR == WSARecv(_socket, &wsaBuf, 1, OUT &numOfBytes, OUT &flags, &_recvEvent, nullptr))
	{
		int errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_recvEvent._owner = nullptr; // RELEASE REF
			HandleError(errorCode);
		}
	}
}


/**
 * \brief Send 비동기 IO 작업을 요청하는 함수
 */
void Session::RegisterSend()
{
	if (IsConnected() == false)
	{
		return;
	}

	_sendEvent.Init();
	_sendEvent._owner = shared_from_this(); // ADD REF

	{
		// _sendQueue에 쌓여있는 데이터를 _sendEvent의 _sendBuffers로 이동
		// Scatter-Gather IO : 여러개의 메모리 버퍼를 지정하여 전송
		lock_guard lock(_mutex);
		int writeSize = 0;
		while (_sendQueue.empty() == false)
		{
			shared_ptr<SendBuffer> sendBuffer = _sendQueue.front();
			_sendQueue.pop();

			writeSize += sendBuffer->WriteSize();

			_sendEvent._sendBuffers.push_back(sendBuffer);
		}
	}

	vector<WSABUF> wsaBufs;
	wsaBufs.reserve(_sendEvent._sendBuffers.size());
	for (auto sendBuffer : _sendEvent._sendBuffers)
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
		wsaBuf.len = static_cast<ULONG>(sendBuffer->WriteSize());
		wsaBufs.push_back(wsaBuf);
	}

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == WSASend(_socket, wsaBufs.data(), wsaBufs.size(), OUT &numOfBytes, 0, &_sendEvent, nullptr))
	{
		int errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_sendEvent._owner = nullptr;
			_sendEvent._sendBuffers.clear();
			_sendRegistered.store(false);
		}
	}
}


/**
 * \brief Connect 비동기 IO 작업 완료 패킷 처리 함수
 */
void Session::ProcessConnect()
{
	// Listener ProcessAccept 에서도 호출 됨
	_connectEvent._owner = nullptr;
	_connected.store(true);

	GetService()->AddSession(GetSessionRef()); // 서비스에 세션 등록

	RegisterRecv();
}


/**
 * \brief Disconnect 비동기 IO 작업 완료 패킷 처리
 */
void Session::ProcessDisconnect()
{
	_disconnectEvent._owner = nullptr;

	if (_user != nullptr)
	{
		// 세션(유저)이 사용한 컨텐츠 정리
		if (_user->room != nullptr)
		{
			// user가 채팅방에 있었다면 해당 방에 알림
			auto roomManager = _user->room->GetRoomManager();
			roomManager->LeaveRoom(_user);
		}

		// 유저 참조 해제
		_user = nullptr;
	}

	GetService()->ReleaseSession(GetSessionRef()); // 서비스에서 세션 삭제
}


/**
 * \brief Recv 완료 패킷 처리 함수
 * \param numOfBytes 완료 패킷의 크기
 */
void Session::ProcessRecv(int numOfBytes)
{
	_recvEvent._owner = nullptr;

	if (numOfBytes == 0)
	{
		// 수신받은 데이터의 크기가 0이면 연결이 끊긴 상황
		Disconnect(L"Recv 0");
		return;
	}

	if (_recvBuffer.OnWrite(numOfBytes) == false)
	{
		// 수신 버퍼 오류
		Disconnect(L"OnWrite Overflow");
		return;
	}

	// 패킷 처리
	int processLen = 0;
	int totalDataSize = _recvBuffer.DataSize();

	while (true)
	{
		int dataSize = totalDataSize - processLen;

		// 패킷 헤더 파싱 가능 여부
		if (dataSize < sizeof(PacketHeader))
		{
			break;
		}

		BYTE* buffer = &_recvBuffer.ReadPos()[processLen];

		// 헤더 파싱
		PacketHeader header = *(reinterpret_cast<PacketHeader*>(buffer));

		// 데이터 파싱 가능 여부
		if (dataSize < header.size)
		{
			break;
		}

		shared_ptr<Session> session = static_pointer_cast<Session>(shared_from_this());

		// 패킷 핸들러 함수 호출
		PacketHandler::HandlePacket(session, &buffer[sizeof(PacketHeader)],
		                            header.size - sizeof(PacketHeader),
		                            header.id);

		processLen += header.size;
	}

	if (processLen < 0 || _recvBuffer.DataSize() < processLen || _recvBuffer.OnRead(processLen) == false)
	{
		// 처리한 바이트의 수가 0 미만
		// 처리한 데이터의 크기가 처리할 수 있는 데이터의 크기 초과
		// 처리한 데이터의 크기만큼 커서를 옮기는 과정에서 실패하면 오류
		Disconnect(L"OnRead Overflow");
		return;
	}

	// 커서 정리
	_recvBuffer.Clean();

	RegisterRecv();
}


/**
 * \brief Send 비동기 IO 작업 완료 패킷 처리 함수
 * \param numOfBytes 완료 패킷의 크기
 */
void Session::ProcessSend(int numOfBytes)
{
	_sendEvent._owner = nullptr;
	_sendEvent._sendBuffers.clear();

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	lock_guard lock(_mutex);
	if (_sendQueue.empty())
	{
		_sendRegistered.store(false);
	}
	else
	{
		RegisterSend();
	}
}


/**
 * \brief 에러 처리 함수
 * \param errorCode WSAGetLastError로 부터 반환된 값
 */
void Session::HandleError(int errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect(L"HandleError");
		break;
	default:
		// TODO: Log
		cout << "Handle Error: " << errorCode << endl;
		break;
	}
}


/**
 * \brief 소켓을 HANDLE로 캐스팅 하여 반환 하는 함수
 * \return HANDLE로 캐스팅 된 _socket
 */
HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}


/**
 * \brief 완료 패킷 처리 함수 분기를 위한 함수
 * \param iocpEvent 완료 패킷의 이벤트
 * \param numOfBytes 완료 패킷의 크기
 */
void Session::Dispatch(IocpEvent* iocpEvent, int numOfBytes)
{
	switch (iocpEvent->GetEventType())
	{
	case EventType::Connect:
		ProcessConnect();
		break;
	case EventType::Disconnect:
		ProcessDisconnect();
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(numOfBytes);
		break;
	default:
		break;
	}
}
