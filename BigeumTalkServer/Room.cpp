﻿#include "pch.h"
#include "Room.h"
#include "PacketHandler.h"


Room::Room(shared_ptr<RoomManager> owner, unsigned long long roomId, string roomName, string hostName,
           unsigned int maxUser)
	: _owner(owner), _roomName(roomName), _hostName(hostName), _maxUser(maxUser), /* TEMP USER COUNT */ _userCount(0),
	  _roomId(roomId)
{
#ifdef _DEBUG
	cout << "[ROOM CREATED] " << '[' << _roomId << "] " << _roomName << endl;
#endif
}

Room::~Room()
{
#ifdef _DEBUG
	cout << "[ROOM DESTROYED] " << '[' << _roomId << "] " << _roomName << endl;
#endif
}


vector<pair<unsigned long long, string>> Room::GetUsersList()
{
	unique_lock lock(_sMutex);
	vector<pair<unsigned long long, string>> ret;

	for (auto& user : _users)
	{
		ret.emplace_back(user.second->userId, user.second->nickname);
	}

	return ret;
}

/**
 * \brief 인자 user를 채팅방에 입장 시키는 함수
 * \param user 입장할 유저
 * \return 입장 성공 여부를 반환합니다.
 */
bool Room::Enter(shared_ptr<User> user)
{
	{
		// 입장 처리만 Lock
		unique_lock lock(_sMutex);

		// 최대 허용 인원 확인
		if (_userCount >= _maxUser)
		{
			return false;
		}

		user->room = shared_from_this(); // Cycle 유의
		_users[user->userId] = user;
		_userCount++;
	}

	Protocol::S_OTHER_ENTER pkt;
	auto pUser = new Protocol::User();
	pUser->set_id(user->userId);
	pUser->set_nickname(user->nickname);
	pkt.set_allocated_user(pUser);
	pkt.set_timestamp(std::chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).
		count());

	// 입장 알림
	auto sendBuffer = PacketHandler::MakeBuffer_S_OTHER_ENTER(pkt);
	Broadcast(sendBuffer);

#ifdef _DEBUG
	cout << "[USER ENTER ROOM] " << '[' << user->userId << "] " << user->nickname << " To " << '[' << _roomId <<
		"] " << _roomName <<
		endl;
#endif


	return true;
}


/**
 * \brief 인자 user가 채팅방에서 나오도록 하는 함수
 * \param user 떠날 유저
 */
void Room::Leave(shared_ptr<User> user)
{
	{
		// 퇴장 처리만 Lock
		unique_lock lock(_sMutex);

		_users.erase(user->userId);
		_userCount--;

#ifdef _DEBUG
		cout << "[USER LEAVE ROOM] " << '[' << user->userId << "] " << user->nickname << " From " << '[' << _roomId <<
			"] " << _roomName <<
			endl;
#endif

		if (_userCount == 0)
		{
			GetRoomManager()->CloseRoom(_roomId); // 참조 해제
			return;
		}
	}

	// 퇴장 알림
	Protocol::S_OTHER_LEAVE pkt;
	auto pUser = new Protocol::User();
	pUser->set_id(user->userId);
	pUser->set_nickname(user->nickname);
	pkt.set_allocated_user(pUser);
	pkt.set_timestamp(std::chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).
		count());


	shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_OTHER_LEAVE(pkt);
	Broadcast(sendBuffer);
}


/**
 * \brief 채팅방에 있는 유저 전체에게 메시지를 보내는 함수
 * \param sendBuffer 보낼 메시지
 */
void Room::Broadcast(shared_ptr<SendBuffer> sendBuffer)
{
	shared_lock lock(_sMutex);
	for (auto& p : _users)
	{
		p.second->ownerSession->Send(sendBuffer);
	}
}


RoomManager::~RoomManager()
{
	for (auto& room : _rooms)
	{
		CloseRoom(room.first);
	}
	_rooms.clear();
}

/**
 * \brief 채팅방에 유저를 입장시키는 함수
 * \param user 입장할 유저
 * \param roomId 입장할 방 ID
 * \return 입장 성공 여부
 */
bool RoomManager::EnterRoom(shared_ptr<User> user, unsigned long long roomId)
{
	lock_guard lock(_mutex);
	// 방 존재 여부 확인
	auto room = _rooms.find(roomId);
	if (room == _rooms.end())
	{
		return false;
	}

	return room->second->Enter(user);
}


/**
 * \brief 유저가 채팅방에서 퇴장 하는 함수
 * \param user 퇴장할 유저
 * \param roomId 퇴장할 방 ID
 */
void RoomManager::LeaveRoom(shared_ptr<User> user)
{
	lock_guard lock(_mutex);
	if (user->room == nullptr)
	{
		return;
	}

	user->room->Leave(user);
}


/**
 * \brief 채팅방 생성 함수
 * \param roomName 채팅방 이름
 * \param maxUser 최대 허용 유저 수
 * \return 생성된 채팅방의 ID
 */
unsigned long long RoomManager::CreateRoom(shared_ptr<User> user, string roomName, unsigned int maxUser)
{
	// ID는 1부터 CreateRoom이 호출되는 순서대로 증가
	static atomic<unsigned long long> idGenerator = 1;
	unsigned long long roomId = idGenerator.fetch_add(1);
	shared_ptr<RoomManager> roomManager = shared_from_this();
	auto room = make_shared<Room>(roomManager, roomId, roomName, user->nickname, maxUser);

	lock_guard lock(_mutex);
	_rooms.emplace(roomId, room);

	return roomId;
}


vector<RoomData> RoomManager::GetRoomList()
{
	lock_guard lock(_mutex);
	vector<RoomData> ret;
	for (auto& room : _rooms)
	{
		RoomData roomData;
		roomData.roomId = room.second->GetRoomId();
		roomData.roomName = room.second->GetRoomName();
		roomData.hostName = room.second->GetHostName();
		roomData.maxUser = room.second->GetRoomMaxUser();
		roomData.userCount = room.second->GetRoomUserCount();

		ret.push_back(roomData);
	}

	return ret;
}

RoomData RoomManager::GetRoomData(unsigned long long roomId)
{
	lock_guard lock(_mutex);
	RoomData roomData;
	roomData.roomId = roomId;
	roomData.roomName = _rooms[roomId]->GetRoomName();
	roomData.hostName = _rooms[roomId]->GetHostName();
	roomData.maxUser = _rooms[roomId]->GetRoomMaxUser();
	roomData.userCount = _rooms[roomId]->GetRoomUserCount();

	return roomData;
}

/**
 * \brief 채팅방을 닫는 함수
 * \param roomId 닫을 채팅방 ID
 */
void RoomManager::CloseRoom(unsigned long long roomId)
{
	// LeaveRoom 에서만 호출되므로 Lock 불필요
	ASSERT_CRASH(_rooms.erase(roomId) != 0);
}
