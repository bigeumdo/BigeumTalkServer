#pragma once

#include <string>
#include "Iocp.h"
#include "Listener.h"
#include "Session.h"
#include "Room.h"

/**
 * \brief Service 클래스 \n
 * \details 서버의 서비스를 담당하는 클래스입니다.
 * \details 연결된 세션 정보 및 컨텐츠 정보 등 을 가지고 있습니다.
 */
class Service : public enable_shared_from_this<Service>
{
public:
	Service(shared_ptr<Iocp> iocp, wstring ip, unsigned short port);
	~Service();

	bool Start();

	/* 세션 생성/소멸 */
	shared_ptr<Session> CreateSession();
	void AddSession(shared_ptr<Session> session);
	void ReleaseSession(shared_ptr<Session> session);

	/* 로그인 */
	bool UseNickname(string nickname);
	void ReleaseNickname(string nickname);
	bool IsExistNickname(string nickname);

	/** \brief shared_ptr<Iocp> 반환 함수 \return _iocp */
	shared_ptr<Iocp> GetIocp() { return _iocp; }

	/** \brief 해당 세션의 소켓 주소 반환 함수 \return _address */
	SOCKADDR_IN& GetSockAddr() { return _address; }

	/** \brief 해당 세션의 룸 매니저 반환 함수 \return _roomManager */
	shared_ptr<RoomManager> GetRoomManager() { return _roomManager; }

private:
	mutex _mutexSession, _mutexNickname;

	shared_ptr<Iocp> _iocp;
	SOCKADDR_IN _address;
	shared_ptr<Listener> _listener = nullptr;

	/* 세션 관련 */
	int _sessionCount = 0;
	set<shared_ptr<Session>> _sessions;
	set<string> _usedNickname;

	/* 컨텐츠 관련 */
	shared_ptr<RoomManager> _roomManager;
};
