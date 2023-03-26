#include "pch.h"
#include "Room.h"
#include "PacketHandler.h"


Room::Room(shared_ptr<RoomManager> owner, unsigned long long roomId, string roomName, unsigned int maxUser)
	: _owner(owner), _roomName(roomName), _maxUser(maxUser), /* TEMP USER COUNT */ _userCount(1), _roomId(roomId)
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


/**
 * \brief 인자 user를 채팅방에 입장 시키는 함수
 * \param user 입장할 유저
 * \return 입장 성공 여부를 반환합니다.
 */
bool Room::Enter(shared_ptr<User> user)
{
	user->room = shared_from_this(); // Cycle 유의
	_users[user->userId] = user;
	_userCount++;

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
	_users.erase(user->userId);
	_userCount--;
#ifdef _DEBUG
	cout << "[USER LEAVE ROOM] " << '[' << user->userId << "] " << user->nickname << " From " << '[' << _roomId <<
		"] " << _roomName <<
		endl;
#endif
}


/**
 * \brief 채팅방을 닫는 함수
 */
void Room::Close()
{
	_users.clear();
}


/**
 * \brief 채팅방에 있는 유저 전체에게 메시지를 보내는 함수
 * \param sendBuffer 보낼 메시지
 */
void Room::Broadcast(shared_ptr<SendBuffer> sendBuffer)
{
	lock_guard<mutex> guard(_mutex);
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
	lock_guard<mutex> guard(_mutex);

	return CanEnter(roomId) == true ? _rooms[roomId]->Enter(user) : false;
}


/**
 * \brief 유저가 채팅방에서 퇴장 하는 함수
 * \param user 퇴장할 유저
 * \param roomId 퇴장할 방 ID
 */
void RoomManager::LeaveRoom(shared_ptr<User> user)
{
	lock_guard<mutex> guard(_mutex);
	if (user->room == nullptr)
	{
		return;
	}
	unsigned long long roomId = user->room->GetRoomId();
	auto& room = _rooms[roomId];

	room->Leave(user);

	shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakeBuffer_S_OTHER_LEAVE(user);
	room->Broadcast(sendBuffer);

	user->room = nullptr;

	if (room->GetRoomUserCount() == 0)
	{
		room->Close();
		CloseRoom(roomId);
	}
}


/**
 * \brief 채팅방 생성 함수
 * \param roomName 채팅방 이름
 * \param maxUser 최대 허용 유저 수
 * \return 생성된 채팅방의 ID
 */
unsigned long long RoomManager::CreateRoom(string roomName, unsigned int maxUser)
{
	lock_guard<mutex> guard(_mutex);

	// ID는 1부터 CreateRoom이 호출되는 순서대로 증가
	static atomic<unsigned long long> idGenerator = 1;
	shared_ptr<RoomManager> roomManager = shared_from_this();
	auto room = make_shared<Room>(roomManager, idGenerator, roomName, maxUser);

	_rooms.insert({idGenerator, room});

	return idGenerator++;
}


/**
 * \brief 채팅방을 닫는 함수
 * \param roomId 닫을 채팅방 ID
 */
void RoomManager::CloseRoom(unsigned long long roomId)
{
	ASSERT_CRASH(_rooms.erase(roomId) != 0);
}


/**
 * \brief 채팅방에 입장이 가능한지 확인 반환하는 함수
 * \param roomId 확인할 채팅방 ID
 * \return 입장 가능 여부
 */
bool RoomManager::CanEnter(unsigned long long roomId)
{
	if (_rooms.find(roomId) == _rooms.end())
	{
		return false;
	}

	return _rooms[roomId]->GetRoomMaxUser() > _rooms[roomId]->GetRoomUserCount();
}
