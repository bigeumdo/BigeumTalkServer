#pragma once
#include "User.h"

class RoomManager;


/**
 * \brief Room 클래스
 * \details 채팅방 정보를 담고 있는 클래스입니다. RoomManager 클래스에 의해 관리됩니다.
 */
class Room : public enable_shared_from_this<Room>
{
	friend RoomManager;
public:
	Room(shared_ptr<RoomManager> owner, unsigned long long roomId, string roomName, string hostName,
	     unsigned int maxUser);
	~Room();

	void Broadcast(shared_ptr<SendBuffer> sendBuffer);

	/** \brief 채팅방 이름을 반환하는 함수 \return _roomName*/
	string GetRoomName() { return _roomName; }

	/** \brief 채팅방 호스트 이름을 반환하는 함수 \return _hostName*/
	string GetHostName() { return _hostName; }

	/** \brief 채팅방 ID를 반환하는 함수 \return 채팅방 ID*/
	unsigned long long GetRoomId() { return _roomId; }

	/** \brief 채팅방 최대 허용 유저 수를 반환하는 함수 \return 채팅방 최대 허용 유저 수*/
	unsigned int GetRoomMaxUser() { return _maxUser; }

	/** \brief 현재 채팅방에 있는 유저의 수를 반환하는 함수 \return 현재 채팅방에 있는 유저 수*/
	unsigned int GetRoomUserCount() { return _userCount; }

	/** \brief 현재 채팅방의 유저 배욜을 반환하는 함수 */
	vector<pair<unsigned long long, string>> GetUsersList();

	/** \brief 현재 채팅방을 관리하는 RoomManager를 반환하는 함수 \return _owner.lock() */
	shared_ptr<RoomManager> GetRoomManager() { return _owner.lock(); }

private:
	bool Enter(shared_ptr<User> user);
	void Leave(shared_ptr<User> user);

private:
	// 입장 퇴장 빈도보다 Broadcast 빈도가 더 많음
	shared_mutex _sMutex;
	map<unsigned long long, shared_ptr<User>> _users;
	weak_ptr<RoomManager> _owner;
	string _roomName;
	string _hostName;
	unsigned int _maxUser;
	unsigned int _userCount;
	unsigned long long _roomId;
};


struct RoomData
{
	unsigned long long roomId;
	string roomName;
	string hostName;
	unsigned int maxUser;
	unsigned int userCount;
};

/**
 * \brief RoomManager 클래스
 * \details 채팅방을 생성하고 닫는 것을 관리하는 클래스입니다.
 * \details 유저가 채팅방에 입장할 때에도 매니저를 통해 입장합니다.
 */
class RoomManager : public enable_shared_from_this<RoomManager>
{
	friend Room;
public:
	~RoomManager();
	bool EnterRoom(shared_ptr<User> user, unsigned long long roomId);
	void LeaveRoom(shared_ptr<User> user);
	unsigned long long CreateRoom(shared_ptr<User> user, string roomName, unsigned int maxUser = 100);
	bool CanEnter(unsigned long long roomId);
	vector<RoomData> GetRoomList();
	RoomData GetRoomData(unsigned long long roomId);

private:
	void CloseRoom(unsigned long long roomId);

private:
	mutex _mutex;
	map<unsigned long long, shared_ptr<Room>> _rooms;
};
