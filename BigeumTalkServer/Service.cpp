#include "pch.h"
#include "Service.h"
#include "Room.h"
#include "Session.h"

Service::Service(shared_ptr<Iocp> iocp, wstring ip, unsigned short port)
	: _iocp(iocp)
{
	_address.sin_family = AF_INET;
	_address.sin_port = htons(port);
	InetPtonW(AF_INET, ip.c_str(), &_address.sin_addr);

	_roomManager = make_shared<RoomManager>();

	// TEMP CODE
	// 서버에서 직접 룸 생성
	_roomManager->CreateRoom("테스트 룸");
}

Service::~Service()
{
	WSACleanup();
	_sessions.clear();
	_iocp = nullptr;
	_listener = nullptr;
}


/**
 * \brief 서비스를 시작하는 함수
 * \return 서비스 시작 성공 여부
 */
bool Service::Start()
{
	// 리슨 소켓 생성
	_listener = make_shared<Listener>();
	if (_listener == nullptr)
	{
		return false;
	}

	// 리슨 소켓 Accept 시작
	if (_listener->StartAccept(shared_from_this()) == false)
	{
		return false;
	}

	cout << "[SERVER STARTED]" << endl;

	return true;
}


/**
 * \brief 세션 생성 함수
 * \return 생성된 빈 세션
 */
shared_ptr<Session> Service::CreateSession()
{
	// 세션 생성 함수

	auto session = make_shared<Session>();
	session->SetService(shared_from_this());

	if (_iocp->Register(session) == false)
	{
		return nullptr;
	}

	return session;
}


/**
 * \brief 연결된 세션을 서비스에 등록하는 함수
 * \param session 연결이 완료된 세션
 */
void Service::AddSession(shared_ptr<Session> session)
{
	lock_guard<mutex> guard(_mutex);
	_sessionCount++;
	_sessions.insert(session);
}


/**
 * \brief 서비스에서 세션을 삭제하는 함수
 * \param session 삭제할 세션
 */
void Service::ReleaseSession(shared_ptr<Session> session)
{
	// 연결이 끊긴 세션 삭제 함수
	lock_guard<mutex> guard(_mutex);
	ASSERT_CRASH(_sessions.erase(session) != 0);
	_sessionCount--;
}


/**
 * \brief 유저가 사용할 닉네임 등록
 * \param nickname 사용할 닉네임
 */
void Service::UseNickname(string nickname)
{
	lock_guard<mutex> guard(_mutex);
	_usedNickname.insert(nickname);
}


/**
 * \brief 닉네임 set에서 닉네임 삭제
 * \param nickname 삭제할 닉네임
 */
void Service::ReleaseNickname(string nickname)
{
	lock_guard<mutex> guard(_mutex);
	ASSERT_CRASH(_usedNickname.erase(nickname) != 0);
}


/**
 * \brief 해당 닉네임이 사용중인지 확인하는 함수
 * \param nickname 확인할 닉네임
 * \return 닉네임의 존재 여부
 */
bool Service::IsExistNickname(string nickname)
{
	return _usedNickname.find(nickname) != _usedNickname.end();
}
