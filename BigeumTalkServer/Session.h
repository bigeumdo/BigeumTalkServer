#pragma once

#include <mutex>

#include "Iocp.h"
#include "RecvBuffer.h"

class Service;
class User;

/**
 * \brief Session 클래스 \n
 * \details 클라이언트의 연결 정보를 담고 있습니다.
 * \details 해당 클래스를 통해 패킷을 보내고 받은 패킷을 처리할 수 있습니다.
 */
class Session : public IocpObject
{
	enum
	{
		BUFFER_SIZE = 0x10000,
	};

	friend class Listener;
	friend class Service;
public:
	Session();
	~Session();

public:
	/* 외부 사용 */
	bool Connect();
	void Disconnect(const WCHAR* cause);
	void Send(shared_ptr<SendBuffer> sendBuffer);

	/* 정보 */
	/** \brief 세션의 서비스를 반환하는 함수 \return _service의 shared_ptr */
	shared_ptr<Service> GetService() { return _service.lock(); }

	/** \brief 세션의 주소 정보를 반환하는 함수 \return _address */
	SOCKADDR_IN GetAddress() { return _address; }

	/** \brief 세션의 소켓을 반환하는 함수 \return _socket */
	SOCKET GetSocket() { return _socket; }

	/** \brief 세션의 shared_ptr을 반환하는 함수 \return shared_ptr<Session>으로 형 변환된 IocpObject */
	shared_ptr<Session> GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

	/** \brief 세션의 서비스를 설정하는 함수 */
	void SetService(shared_ptr<Service> service) { _service = service; }

	/** \brief 세션의 주소를 설정하는 함수 */
	void SetAddress(SOCKADDR_IN address) { _address = address; }

	/** \brief 세션이 연결되었는지 확인하는 함수 \return _connected */
	bool IsConnected() { return _connected; }

public:
	/* 컨텐츠 함수 */
	bool EnterRoom(unsigned long long roomId);

private:
	/* 비동기 IO 요청 */
	bool RegisterConnect();
	bool RegisterDisconnect();
	void RegisterRecv();
	void RegisterSend();

	/* 완료 패킷 처리 */
	void ProcessConnect();
	void ProcessDisconnect();
	void ProcessRecv(int numOfBytes);
	void ProcessSend(int numOfBytes);

	void HandleError(int errorCode);

private:
	/* IocpObject 인터페이스 */
	HANDLE GetHandle() override;
	void Dispatch(IocpEvent* iocpEvent, int numOfBytes) override;

private:
	mutex _mutex;

	/* 세션 정보 */
	weak_ptr<Service> _service;
	SOCKET _socket = INVALID_SOCKET;
	SOCKADDR_IN _address = {};
	atomic<bool> _connected = false;
	wstring _nickname;

	/* 송신 */
	RecvBuffer _recvBuffer;

	/* 수신 */
	queue<shared_ptr<SendBuffer>> _sendQueue;
	atomic<bool> _sendRegistered = false;

	/* IOCP 이벤트 재사용 */
	ConnectEvent _connectEvent;
	DisconnectEvent _disconnectEvent;
	RecvEvent _recvEvent;
	SendEvent _sendEvent;

public:
	/* 콘텐츠 정보 */
	shared_ptr<User> _user = nullptr; // Cycle, 게임의 경우 가변 배열로 나의 캐릭터들 표현
};
