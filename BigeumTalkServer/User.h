#pragma once

class Room;


/**
 * \brief User 클래스
 * \details 클라이언트 세션에서 정의한 사용자의 정보를 담고 있습니다.
 * \details User 객체를 생성한 Session의 정보와 현재 입장중인 채팅방의 정보를 가지고 있습니다.
 */
class User
{
public:
	~User();

	unsigned long long userId = 0;
	string nickname;
	shared_ptr<Session> ownerSession; // Cycle
	shared_ptr<Room> room;
};
