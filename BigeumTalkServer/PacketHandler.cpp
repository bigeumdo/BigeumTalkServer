#include "pch.h"
#include "PacketHandler.h"
#include "User.h"
#include "Room.h"
#include "Service.h"
#include <chrono>


/**
 * \brief 도착한 패킷의 프로토콜을 구분하여 핸들러를 호출하는 함수
 * \param session 패킷을 보낸 세션 shared_ptr 주소
 * \param buffer 패킷이 담긴 버퍼 주소
 * \param len 패킷의 길이
 * \param protocol 패킷의 프로토콜
 */
void PacketHandler::HandlePacket(shared_ptr<Session>& session, BYTE* buffer, int len, unsigned short protocol)
{
	switch (protocol)
	{
	case C_VERSION_CHECK:
		break;
	case C_LOGIN:
		Handle_C_LOGIN(session, buffer, len);
		break;
	case C_ENTER_ROOM:
		Handle_C_ENTER_ROOM(session, buffer, len);
		break;
	case C_CHAT:
		Handle_C_CHAT(session, buffer, len);
		break;
	}
}


/**
 * \brief S_LOGIN 프로토콜의 버퍼를 생성하는 함수
 * \param resultCode 서버 처리 결과 코드
 * \param userId User의 ID
 * \return 생성된 버퍼
 */
shared_ptr<SendBuffer> PacketHandler::MakeBuffer_S_LOGIN(unsigned short resultCode, unsigned long long userId)
{
	Protocol::S_LOGIN pkt;
	pkt.resultCode = resultCode;
	pkt.userId = userId;

	return MakeSendBuffer(pkt, S_LOGIN);
}


/**
 * \brief S_ENTER_ROOM 프로토콜의 버퍼를 생성하는 함수
 * \param resultCode 서버 처리 결과 코드
 * \param user User 객체 shared_ptr
 * \return 생성된 버퍼
 */
shared_ptr<SendBuffer> PacketHandler::MakeBuffer_S_ENTER_ROOM(unsigned short resultCode, shared_ptr<User> user)
{
	Protocol::S_ENTER_ROOM pkt;
	pkt.resultCode = resultCode;
	pkt.roomId = user->room->GetRoomId();
	pkt.roomName = user->room->GetRoomName();

	// 채팅방에 있는 유저 리스트
	pkt.users.reserve(user->room->GetUsersList().size());

	auto& users = user->room->GetUsersList();
	for (auto& user : users)
	{
		pkt.users.push_back({user.first, user.second->nickname});
	}

	return MakeSendBuffer(pkt, S_ENTER_ROOM);
}


/**
 * \brief S_CHAT 프로토콜의 버퍼를 생성하는 함수
 * \param resultCode 서버 처리 결과 코드
 * \param cPkt 클라이언트에서 보낸 패킷
 * \return 생성된 버퍼
 */
shared_ptr<SendBuffer> PacketHandler::MakeBuffer_S_CHAT(unsigned short resultCode, Protocol::C_CHAT cPkt)
{
	Protocol::S_CHAT pkt;
	{
		pkt.serverMessage = false;
		pkt.userId = 0;
		pkt.nickname = "";
		pkt.message = "";
		pkt.timestamp = 0;
	}

	pkt.resultCode = resultCode;
	if (resultCode == CHAT_OK)
	{
		pkt.nickname = cPkt.nickname;
		pkt.userId = cPkt.userId;
		pkt.message = cPkt.message;
		pkt.serverMessage = false;
		pkt.timestamp = std::chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).
			count();
	}

	return MakeSendBuffer(pkt, S_CHAT);
}


/**
 * \brief S_OTHER_ENTER 프로토콜의 버퍼를 생성하는 함수
 * \param other 채팅방에 들어온 유저
 * \return 생성된 버퍼
 */
shared_ptr<SendBuffer> PacketHandler::MakeBuffer_S_OTHER_ENTER(shared_ptr<User> other)
{
	Protocol::S_OTHER_ENTER pkt;
	pkt.user.nickName = other->nickname;
	pkt.user.userId = other->userId;
	pkt.timestamp = std::chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).
		count();

	return MakeSendBuffer(pkt, S_OTHER_ENTER);
}


/**
 * \brief S_OTHER_LEAVE 프로토콜의 버퍼를 생성하는 함수
 * \param other 채팅방에서 퇴장한 유저
 * \return 생성된 버퍼
 * \details RoomManager 클래스의 LeaveRoom함수에서 호출됨
 */
shared_ptr<SendBuffer> PacketHandler::MakeBuffer_S_OTHER_LEAVE(shared_ptr<User> other)
{
	Protocol::S_OTHER_LEAVE pkt;
	pkt.user.nickName = other->nickname;
	pkt.user.userId = other->userId;
	pkt.timestamp = std::chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).
		count();

	return MakeSendBuffer(pkt, S_OTHER_LEAVE);
}

/**
 * \brief C_LOGIN 프로토콜을 처리하는 함수
 * \param session C_LOGIN 프로토콜 패킷을 전달한 세션
 * \param buffer 패킷이 담긴 버퍼 주소
 * \param len 패킷의 길이
 */
void Handle_C_LOGIN(shared_ptr<Session>& session, BYTE* buffer, int len)
{
	Protocol::C_LOGIN pkt;
	pkt.GetPacket(buffer, len);

	auto service = session->GetService();

	if (service->IsExistNickname(pkt.nickname))
	{
		// 중복 닉네임
		shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_LOGIN(LOGIN_EXIST);
		session->Send(sendBuffer);
		return;
	}

	static atomic<unsigned long long> idGenerator = 1;

	auto userRef = make_shared<User>();
	userRef->userId = idGenerator++;
	userRef->nickname = pkt.nickname;
	userRef->ownerSession = session;

	session->_user = userRef;

	service->UseNickname(pkt.nickname);

#ifdef _DEBUG
	cout << "[USER LOGIN] " << '[' << userRef->userId << "] " << userRef->nickname << endl;
#endif

	shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_LOGIN(LOGIN_SUCCESS, userRef->userId);
	session->Send(sendBuffer);
}

/**
 * \brief C_ENTER_ROOM 프로토콜을 처리하는 함수
 * \param session C_ENTER_ROOM 프로토콜 패킷을 전달한 세션
 * \param buffer 패킷이 담긴 버퍼 주소
 * \param len 패킷의 길이
 */
void Handle_C_ENTER_ROOM(shared_ptr<Session>& session, BYTE* buffer, int len)
{
	Protocol::C_ENTER_ROOM pkt;
	pkt.GetPacket(buffer, len);

	auto service = session->GetService();
	if (service->GetRoomManager()->EnterRoom(session->_user, pkt.roomId) == false)
	{
		shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_ENTER_ROOM(
			ENTER_ROOM_FAIL, session->_user);
		session->Send(sendBuffer);
		return;
	}

	// 입장 성공
	shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_ENTER_ROOM(
		ENTER_ROOM_SUCCESS, session->_user);
	session->Send(sendBuffer);
	sendBuffer = nullptr;

	// 입장 알림
	sendBuffer = PacketHandler::MakeBuffer_S_OTHER_ENTER(session->_user);
	session->_user->room->Broadcast(sendBuffer);
}

/**
 * \brief C_CHAT 프로토콜을 처리하는 함수
 * \param session C_CHAT 프로토콜 패킷을 전달한 세션
 * \param buffer 패킷이 담긴 버퍼 주소
 * \param len 패킷의 길이
 */
void Handle_C_CHAT(shared_ptr<Session>& session, BYTE* buffer, int len)
{
	Protocol::C_CHAT pkt;
	pkt.GetPacket(buffer, len);

	auto room = session->_user->room;
	if (room == nullptr)
	{
		shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_CHAT(CHAT_FAIL, pkt);
		session->Send(sendBuffer);
		return;
	}

	shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_CHAT(CHAT_OK, pkt);
	room->Broadcast(sendBuffer);
}
