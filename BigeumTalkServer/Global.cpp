#include "pch.h"
#include "Global.h"
#include "SocketUtils.h"
#include "SendBuffer.h"
#include "Room.h"

SendBufferManager* GSendBufferManager = nullptr;


/**
 * \brief Global 클래스
 * \details 프로젝트 전역에서 사용 가능한 글로벌 객체를 생성합니다.
 */
class Global
{
public:
	Global()
	{
		SocketUtils::Init();
		GSendBufferManager = new SendBufferManager();
	}

	~Global()
	{
		delete GSendBufferManager;
	}
} G;
