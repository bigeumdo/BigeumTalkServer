#pragma once
#include "Protocol.h"

class Session;

/**
 * \brief ServerPacketHandler 클래스
 * \details 서버에 도착한 패킷을 처리하고 클라이언트에게 보낼 패킷을 생성합니다.
 */
class PacketHandler
{
public:
	static void HandlePacket(shared_ptr<Session>& session, BYTE* buffer, int len, unsigned short protocol);

	static shared_ptr<SendBuffer> MakeBuffer_S_LOGIN(unsigned short resultCode, unsigned long long userId = 0);
	static shared_ptr<SendBuffer> MakeBuffer_S_ENTER_ROOM(unsigned short resultCode, shared_ptr<User> user);
	static shared_ptr<SendBuffer> MakeBuffer_S_CHAT(unsigned short resultCode, Protocol::C_CHAT cPkt);
	static shared_ptr<SendBuffer> MakeBuffer_S_OTHER_ENTER(shared_ptr<User> other);
	static shared_ptr<SendBuffer> MakeBuffer_S_OTHER_LEAVE(shared_ptr<User> other);

	template <typename T>
	static shared_ptr<SendBuffer> MakeSendBuffer(T& pkt, unsigned short pktId);
};


/**
 * \brief 생성된 패킷이 담긴 버퍼를 반환하는 함수
 * \tparam T 서버에서 보낼 패킷 정의 구조체
 * \param pkt 패킷으로 생성할 구조체
 * \param pktId 패킷 프로토콜 ID
 * \return 생성 완료된 버퍼 shared_ptr
 */
template <typename T>
shared_ptr<SendBuffer> PacketHandler::MakeSendBuffer(T& pkt, unsigned short pktId)
{
	// 보낼 데이터를 string 자료형의 json으로 변환 -> wstring으로 변환
	wstring pktBody = S2W(pkt.MakeJson());

	const unsigned short dataSize = pktBody.length() * sizeof(WCHAR);
	const unsigned short packetSize = dataSize + sizeof(PacketHeader);

	shared_ptr<SendBuffer> sendBuffer = GSendBufferManager->Open(packetSize);
	auto header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());

	header->size = packetSize;
	header->id = pktId;

	// 헤더의 다음 위치에 패킷 내용 memcpy
	memcpy(&header[1], pktBody.data(), dataSize);
	sendBuffer->Close(packetSize);

	return sendBuffer;
}

void Handle_C_LOGIN(shared_ptr<Session>& session, BYTE* buffer, int len);
void Handle_C_ENTER_ROOM(shared_ptr<Session>& session, BYTE* buffer, int len);
void Handle_C_CHAT(shared_ptr<Session>& session, BYTE* buffer, int len);
