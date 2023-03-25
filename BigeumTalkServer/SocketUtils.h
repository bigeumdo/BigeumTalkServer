#pragma once

#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")


/**
 * \brief SocketUtils 클래스\n
 * \details 전역에서 자주 사용하게될 소켓 관련 함수들을 전역으로 관리하는 클래스입니다.
 */
class SocketUtils
{
public:
	SocketUtils();
	~SocketUtils();

public:
	static LPFN_CONNECTEX ConnectEx;
	static LPFN_DISCONNECTEX DisconnectEx;
	static LPFN_ACCEPTEX AcceptEx;

public:
	static void Init();
	static void Clear();

	static bool BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn);
	static SOCKET CreateSocket();
};
