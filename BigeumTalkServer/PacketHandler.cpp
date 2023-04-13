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

		shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_LOGIN(sPkt);
		session->Send(sendBuffer);

		return true;
	}

	static atomic<unsigned long long> idGenerator = 1;

	// User 객체 생성 후 Session이 참조
	// Cycle이 형성되므로 주의
	auto userRef = make_shared<User>();
	userRef->userId = idGenerator.fetch_add(1);
	userRef->nickname = pkt.user().nickname();
	userRef->ownerSession = session;

	session->_user = userRef;

#ifdef _DEBUG
	cout << "[USER LOGIN] " << '[' << userRef->userId << "] " << userRef->nickname << endl;
#endif

	sPkt.set_success(true);
	sPkt.set_userid(userRef->userId);
	shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_LOGIN(sPkt);
	session->Send(sendBuffer);

	return true;
}

bool Handle_C_CREATE_ROOM(shared_ptr<Session>& session, Protocol::C_CREATE_ROOM& pkt)
{
	auto roomManager = session->GetService()->GetRoomManager();

	// 방 생성
	unsigned long long roomId = roomManager->CreateRoom(session->_user, pkt.roomname());

	// 호스트 입장
	if (roomManager->EnterRoom(session->_user, roomId) == false)
	{
		// 에러 발생
		Protocol::S_CREATE_ROOM sPkt;
		sPkt.set_success(false);

		auto sendBuffer = PacketHandler::MakeBuffer_S_CREATE_ROOM(sPkt);
		session->Send(sendBuffer);

		return true;
	}

	// 결과 패킷
	Protocol::S_CREATE_ROOM sPkt;
	RoomData roomData = roomManager->GetRoomData(roomId);
	auto roomPkt = new Protocol::Room();
	sPkt.set_success(true);
	roomPkt->set_id(roomData.roomId);
	roomPkt->set_roomname(roomData.roomName);
	roomPkt->set_hostname(roomData.hostName);
	roomPkt->set_maxuser(roomData.maxUser);
	roomPkt->set_usercount(roomData.userCount);
	sPkt.set_allocated_room(roomPkt);

	auto sendBuffer = PacketHandler::MakeBuffer_S_CREATE_ROOM(sPkt);
	session->Send(sendBuffer);

	return true;
}

bool Handle_C_ENTER_ROOM(shared_ptr<Session>& session, Protocol::C_ENTER_ROOM& pkt)
{
	auto roomManager = session->GetService()->GetRoomManager();
	Protocol::S_ENTER_ROOM sPkt;

	if (roomManager->EnterRoom(session->_user, pkt.roomid()) == false)
	{
		sPkt.set_success(false);

		shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_ENTER_ROOM(sPkt);
		session->Send(sendBuffer);
		return true;
	}

	// 입장 성공 - 시도한 유저에게
	sPkt.set_success(true);
	auto roomPkt = new Protocol::Room();
	RoomData roomData = roomManager->GetRoomData(pkt.roomid());
	roomPkt->set_id(roomData.roomId);
	roomPkt->set_roomname(roomData.roomName);
	roomPkt->set_hostname(roomData.hostName);
	roomPkt->set_maxuser(roomData.maxUser);
	roomPkt->set_usercount(roomData.userCount);
	sPkt.set_allocated_roomdata(roomPkt);

	auto roomUsers = session->_user->room->GetUsersList();
	for (auto& user : roomUsers)
	{
		auto roomUser = sPkt.add_users();
		roomUser->set_id(user.first);
		roomUser->set_nickname(user.second);
	}
	sPkt.set_allocated_roomdata(roomPkt);

	shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_ENTER_ROOM(sPkt);
	session->Send(sendBuffer);
	sendBuffer = nullptr;

	return true;
}

bool Handle_C_LEAVE_ROOM(shared_ptr<Session>& session, Protocol::C_LEAVE_ROOM& pkt)
{
	if (session->_user->room != nullptr)
	{
		return false;
	}

	auto roomManager = session->GetService()->GetRoomManager();

	roomManager->LeaveRoom(session->_user);
	Protocol::S_LEAVE_ROOM sPkt;
	sPkt.set_success(true);

	auto sendBuffer = PacketHandler::MakeBuffer_S_LEAVE_ROOM(sPkt);
	session->Send(sendBuffer);

	return true;
}

bool Handle_C_ROOM_LIST(shared_ptr<Session>& session, Protocol::C_ROOM_LIST& pkt)
{
	auto roomManager = session->GetService()->GetRoomManager();

	Protocol::S_ROOM_LIST sPkt;
	auto roomList = roomManager->GetRoomList();
	sPkt.set_roomcount(roomList.size());
	for (auto room : roomList)
	{
		auto roomPkt = sPkt.add_rooms();
		roomPkt->set_id(room.roomId);
		roomPkt->set_roomname(room.roomName);
		roomPkt->set_hostname(room.hostName);
		roomPkt->set_maxuser(room.maxUser);
		roomPkt->set_usercount(room.userCount);
	}

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
