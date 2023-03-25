#include "pch.h"
#include "SocketUtils.h"

LPFN_CONNECTEX SocketUtils::ConnectEx = nullptr;
LPFN_DISCONNECTEX SocketUtils::DisconnectEx = nullptr;
LPFN_ACCEPTEX SocketUtils::AcceptEx = nullptr;

SocketUtils::SocketUtils()
{
}

SocketUtils::~SocketUtils()
{
	Clear();
}


/**
 * \brief 정적 변수들을 초기화 하는 함수
 */
void SocketUtils::Init()
{
	WSADATA wsaData;
	_ASSERTE(WSAStartup(MAKEWORD(2, 2), OUT & wsaData) == 0);

	/* 런타임에 주소 얻어오는 API */
	SOCKET dummySocket = CreateSocket();
	_ASSERTE(BindWindowsFunction(dummySocket, WSAID_CONNECTEX, reinterpret_cast<LPVOID*>(&ConnectEx)));
	_ASSERTE(BindWindowsFunction(dummySocket, WSAID_DISCONNECTEX, reinterpret_cast<LPVOID*>(&DisconnectEx)));
	_ASSERTE(BindWindowsFunction(dummySocket, WSAID_ACCEPTEX, reinterpret_cast<LPVOID*>(&AcceptEx)));
	closesocket(dummySocket);
}

void SocketUtils::Clear()
{
	WSACleanup();
}


/**
 * \brief 비동기 IO 함수의 주소를 받아오는 함수
 * \param socket 더미 소켓
 * \param guid 대상 비동기 IO
 * \param fn 주소를 저장할 함수 포인터
 * \return 주소 불러오기 성공 여부
 */
bool SocketUtils::BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn)
{
	DWORD bytes = 0;
	return SOCKET_ERROR != WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), fn, sizeof(*fn),
	                                OUT &bytes, nullptr, nullptr);
}


/**
 * \brief 소켓을 생성하는 함수입니다.
 * \return 생성된 소켓
 */
SOCKET SocketUtils::CreateSocket()
{
	return WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
}
