#pragma once

using namespace std;

class Session;


/**
 * \brief IOCP event 타입 enum 클래스
 */
enum class EventType : __int8
{
	Connect,
	Disconnect,
	Accept,
	Recv,
	Send,
};

class IocpObject;

/**
 * \brief IOCP 이벤트 클래스
 * \details 비동기 IO 함수의 lpOverlapped 인자로 사용되며 이후 완료 패킷이 어떤 이벤트로 부터 발생한 것인지 구분할 때 사용됩니다.
 */
class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType type);
	void Init();

	EventType GetEventType() { return _type; }
	shared_ptr<IocpObject> _owner;

private:
	EventType _type;
};


/**
 * \brief Connect 비동기 이벤트
 */
class ConnectEvent : public IocpEvent
{
public:
	ConnectEvent() : IocpEvent(EventType::Connect)
	{
	}
};


/**
 * \brief Disconnect 비동기 이벤트
 */
class DisconnectEvent : public IocpEvent
{
public:
	DisconnectEvent() : IocpEvent(EventType::Disconnect)
	{
	}
};


/**
 * \brief Accept 비동기 이벤트
 * \details 미리 대기중인 세션의 shared_ptr을 가지고 있습니다.
 */
class AcceptEvent : public IocpEvent
{
public:
	AcceptEvent() : IocpEvent(EventType::Accept)
	{
	}

	shared_ptr<Session> _session = nullptr;
};


/**
 * \brief Recv 비동기 이벤트
 */
class RecvEvent : public IocpEvent
{
public:
	RecvEvent() : IocpEvent(EventType::Recv)
	{
	}
};


/**
 * \brief Send 비동기 이벤트
 */
class SendEvent : public IocpEvent
{
public:
	SendEvent() : IocpEvent(EventType::Send)
	{
	}

	vector<shared_ptr<SendBuffer>> _sendBuffers;
};


/**
 * \brief Iocp Object 클래스
 * \details 비동기 IO 요청을 발생시킬 클래스의 기초 클래스로 사용합니다.
 * \details IocpEvent의 owner 변수에 대입되며, 해당 이벤트를 발생시킨 주체를 구분할 수 있도록 사용됩니다.
 */
class IocpObject : public enable_shared_from_this<IocpObject>
{
public:
	virtual HANDLE GetHandle() abstract;
	virtual void Dispatch(IocpEvent* iocpEvent, int numOfBytes = 0) abstract;
};


/**
 * \brief Iocp 클래스 \n
 * \details IOCP 코어에 해당하며 IocpObject의 CP 등록과 완료패킷 처리를 담당합니다.
 */
class Iocp
{
public:
	Iocp();
	~Iocp();

	HANDLE GetHandle() { return _iocpHandle; }
	bool Register(shared_ptr<IocpObject> iocpObject);
	bool Dispatch(unsigned int timeoutMs = INFINITE);

private:
	HANDLE _iocpHandle;
};
