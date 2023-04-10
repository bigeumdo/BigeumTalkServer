#include "pch.h"
#include "PacketHandler.h"
#include "User.h"
#include "Room.h"
#include "Service.h"
#include <chrono>

unordered_map<unsigned short, PacketHandlerFunc> GPacketHandler;

bool Handle_C_LOGIN(shared_ptr<Session>& session, Protocol::C_LOGIN& pkt)
{
	auto service = session->GetService();
	Protocol::S_LOGIN sPkt;

	// 닉네임 사용 등록 중복 닉네임 확인
	if (service->UseNickname(pkt.user().nickname()) == false)
	{
		sPkt.set_success(false);
		sPkt.set_serverid(0);

		shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_LOGIN(sPkt);
		session->Send(sendBuffer);

		return true;
	}

	static atomic<unsigned long long> idGenerator = 0;

	// User 객체 생성 후 Session이 참조
	// Cycle이 형성되므로 주의
	auto userRef = make_shared<User>();
	userRef->userId = ++idGenerator;
	userRef->nickname = pkt.user().nickname();
	userRef->ownerSession = session;

	session->_user = userRef;

#ifdef _DEBUG
	cout << "[USER LOGIN] " << '[' << userRef->userId << "] " << userRef->nickname << endl;
#endif

	sPkt.set_success(true);
	sPkt.set_serverid(userRef->userId);
	shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_LOGIN(sPkt);
	session->Send(sendBuffer);

	return true;
}

bool Handle_C_CREATE_ROOM(shared_ptr<Session>& session, Protocol::C_CREATE_ROOM& pkt)
{
	return true;
}

bool Handle_C_ENTER_ROOM(shared_ptr<Session>& session, Protocol::C_ENTER_ROOM& pkt)
{
	auto service = session->GetService();
	Protocol::S_ENTER_ROOM sPkt;

	if (service->GetRoomManager()->EnterRoom(session->_user, pkt.roomid()) == false)
	{
		sPkt.set_success(false);
		sPkt.set_isfail(true);

		shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_ENTER_ROOM(sPkt);
		session->Send(sendBuffer);
		return true;
	}

	// 입장 성공 - 시도한 유저에게
	sPkt.set_success(true);
	auto roomData = new Protocol::S_ENTER_ROOM::RoomData();
	roomData->set_name(session->_user->room->GetRoomName());

	auto roomUsers = session->_user->room->GetUsersList();
	for (auto& user : roomUsers)
	{
		auto roomUser = roomData->add_users();
		roomUser->set_id(user.first);
		roomUser->set_nickname(user.second->nickname);
	}
	sPkt.set_allocated_roomdata(roomData);

	shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_ENTER_ROOM(sPkt);
	session->Send(sendBuffer);
	sendBuffer = nullptr;

	return true;
}

bool Handle_C_LEAVE_ROOM(shared_ptr<Session>& session, Protocol::C_LEAVE_ROOM& pkt)
{
	return true;
}

bool Handle_C_ROOM_LIST(shared_ptr<Session>& session, Protocol::C_ROOM_LIST& pkt)
{
	return true;
}

bool Handle_C_CHAT(shared_ptr<Session>& session, Protocol::C_CHAT& pkt)
{
	auto room = session->_user->room;
	if (room == nullptr)
	{
		// 비 정상적인 상황
		// TODO Error 패킷 전송
		return false;
	}

	Protocol::S_CHAT sPkt;
	Protocol::User user;
	user.set_id(session->_user->userId);
	user.set_nickname(session->_user->nickname);
	sPkt.set_allocated_user(&user);
	sPkt.set_msg(pkt.msg());
	sPkt.set_timestamp(std::chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).
		count());

	shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_CHAT(sPkt);
	room->Broadcast(sendBuffer);

	return true;
}
