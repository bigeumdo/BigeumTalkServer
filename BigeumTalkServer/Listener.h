#pragma once

#include "Iocp.h"

using namespace std;

class Service;


/**
 * \brief Listener 클래스
 * \details 리슨 소켓을 담당하는 클래스입니다.
 * \details 클라이언트의 연결 요청을 받고, 연결된 클라이언트를 세션 클래스에 정보를 담아 CP에 연결하도록 합니다.
 */
class Listener : public IocpObject
{
public:
	Listener() = default;
	~Listener();

public:
	bool StartAccept(shared_ptr<Service> service);
	HANDLE GetHandle() override;
	void Dispatch(IocpEvent* iocpEvent, int numOfBytes = 0) override;

private:
	void RegisterAccept(AcceptEvent* acceptEvent);
	void ProcessAccept(AcceptEvent* acceptEvent);

private:
	SOCKET _socket = INVALID_SOCKET;
	vector<AcceptEvent*> _acceptEvents;
	shared_ptr<Service> _service;
};
