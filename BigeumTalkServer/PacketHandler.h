#pragma once
#include "Protocol.pb.h"

// TODO 자동화

class Session;
using PacketHandlerFunc = std::function<bool(shared_ptr<Session>&, BYTE*, int)>;
extern unordered_map<unsigned short, PacketHandlerFunc> GPacketHandler;

/*
 * Handle_C_ 함수 정책
 * 비 정상적인 패킷에 한해서 false 반환
 */

bool Handle_C_LOGIN(shared_ptr<Session>& session, Protocol::C_LOGIN& pkt);
bool Handle_C_CREATE_ROOM(shared_ptr<Session>& session, Protocol::C_CREATE_ROOM& pkt);
bool Handle_C_ENTER_ROOM(shared_ptr<Session>& session, Protocol::C_ENTER_ROOM& pkt);
bool Handle_C_LEAVE_ROOM(shared_ptr<Session>& session, Protocol::C_LEAVE_ROOM& pkt);
bool Handle_C_ROOM_LIST(shared_ptr<Session>& session, Protocol::C_ROOM_LIST& pkt);
bool Handle_C_CHAT(shared_ptr<Session>& session, Protocol::C_CHAT& pkt);

/**
 * \brief ServerPacketHandler 클래스
 * \details 서버에 도착한 패킷을 처리하고 클라이언트에게 보낼 데이터를 생성합니다.
 */
class PacketHandler
{
public:
	/**
	 * \brief 정의한 패킷 처리 함수를 초기화하는 함수
	 */
	static void Init()
	{
		GPacketHandler.emplace(Protocol::PACKET_ID_C_LOGIN,
		                       [](shared_ptr<Session>& session, BYTE* buffer, int len) -> bool
		                       {
			                       return HandlePacketTemplate<Protocol::C_LOGIN>(Handle_C_LOGIN, session, buffer, len);
		                       });

		GPacketHandler.emplace(Protocol::PACKET_ID_C_CREATE_ROOM,
		                       [](shared_ptr<Session>& session, BYTE* buffer, int len) -> bool
		                       {
			                       return HandlePacketTemplate<Protocol::C_CREATE_ROOM>(
				                       Handle_C_CREATE_ROOM, session, buffer, len);
		                       });

		GPacketHandler.emplace(Protocol::PACKET_ID_C_ENTER_ROOM,
		                       [](shared_ptr<Session>& session, BYTE* buffer, int len) -> bool
		                       {
			                       return HandlePacketTemplate<Protocol::C_ENTER_ROOM>(
				                       Handle_C_ENTER_ROOM, session, buffer, len);
		                       });

		GPacketHandler.emplace(Protocol::PACKET_ID_C_LEAVE_ROOM,
		                       [](shared_ptr<Session>& session, BYTE* buffer, int len) -> bool
		                       {
			                       return HandlePacketTemplate<Protocol::C_LEAVE_ROOM>(
				                       Handle_C_LEAVE_ROOM, session, buffer, len);
		                       });

		GPacketHandler.emplace(Protocol::PACKET_ID_C_ROOM_LIST,
		                       [](shared_ptr<Session>& session, BYTE* buffer, int len) -> bool
		                       {
			                       return HandlePacketTemplate<Protocol::C_ROOM_LIST>(
				                       Handle_C_ROOM_LIST, session, buffer, len);
		                       });

		GPacketHandler.emplace(Protocol::PACKET_ID_C_CHAT,
		                       [](shared_ptr<Session>& session, BYTE* buffer, int len) -> bool
		                       {
			                       return HandlePacketTemplate<Protocol::C_CHAT>(Handle_C_CHAT, session, buffer, len);
		                       });
	}


	/**
	 * \brief 도착한 패킷을 처리하는 함수
	 * \param session 패킷을 Recv한 Session
	 * \param buffer Session의 Buffer
	 * \param len 패킷의 길이
	 * \return 정상 처리 여부
	 */
	static bool HandlePacket(shared_ptr<Session>& session, BYTE* buffer, int len)
	{
		auto header = reinterpret_cast<PacketHeader*>(buffer);
		auto handler = GPacketHandler.find(header->id);
		if (handler == GPacketHandler.end())
		{
			return false;
		}
		return handler->second(session, buffer, len);
	}

	static shared_ptr<SendBuffer> MakeBuffer_S_LOGIN(Protocol::S_LOGIN& pkt)
	{
		return MakeSendBuffer(pkt, Protocol::PACKET_ID_S_LOGIN);
	}

	static shared_ptr<SendBuffer> MakeBuffer_S_CREATE_ROOM(Protocol::S_CREATE_ROOM& pkt)
	{
		return MakeSendBuffer(pkt, Protocol::PACKET_ID_S_CREATE_ROOM);
	}

	static shared_ptr<SendBuffer> MakeBuffer_S_ENTER_ROOM(Protocol::S_ENTER_ROOM& pkt)
	{
		return MakeSendBuffer(pkt, Protocol::PACKET_ID_S_ENTER_ROOM);
	}

	static shared_ptr<SendBuffer> MakeBuffer_S_LEAVE_ROOM(Protocol::S_LEAVE_ROOM& pkt)
	{
		return MakeSendBuffer(pkt, Protocol::PACKET_ID_S_LEAVE_ROOM);
	}

	static shared_ptr<SendBuffer> MakeBuffer_S_CHAT(Protocol::S_CHAT& pkt)
	{
		return MakeSendBuffer(pkt, Protocol::PACKET_ID_S_CHAT);
	}

	static shared_ptr<SendBuffer> MakeBuffer_S_OTHER_ENTER(Protocol::S_OTHER_ENTER& pkt)
	{
		return MakeSendBuffer(pkt, Protocol::PACKET_ID_S_OTHER_ENTER);
	}

	static shared_ptr<SendBuffer> MakeBuffer_S_OTHER_LEAVE(Protocol::S_OTHER_LEAVE& pkt)
	{
		return MakeSendBuffer(pkt, Protocol::PACKET_ID_S_OTHER_LEAVE);
	}

private:
	/**
	 * \brief Init을 통해 map에 정의한 패킷 처리 함수를 등록
	 * \tparam PacketType 정의한 패킷
	 * \tparam HandleFunc 패킷 처리 함수
	 * \param func 패킷 처리 함수
	 * \param session 패킷을 Recv한 Session
	 * \param buffer Session의 버퍼
	 * \param len 패킷의 길이
	 * \return 정상 처리 여부
	 */
	template <typename PacketType, typename HandleFunc>
	static bool HandlePacketTemplate(HandleFunc func, shared_ptr<Session>& session, BYTE* buffer, int len)
	{
		PacketType pkt;
		const int headerSize = sizeof(PacketHeader);
		if (pkt.ParseFromArray(buffer + headerSize, len - headerSize) == false)
		{
			return false;
		}

		return func(session, pkt);
	}


	/**
	 * \brief SendBuffer에 정의한 패킷 객체를 직렬화하여 담는 함수
	 * \tparam PacketType 정의한 패킷
	 * \param pkt 정의한 패킷
	 * \param pktId 프로토콜 ID
	 * \return 직렬화된 내용이 담긴 버퍼
	 */
	template <typename PacketType>
	static shared_ptr<SendBuffer> MakeSendBuffer(PacketType& pkt, unsigned short pktId)
	{
		const unsigned short dataSize = static_cast<unsigned short>(pkt.ByteSizeLong());
		const unsigned short packetSize = dataSize + sizeof(PacketHeader);

		shared_ptr<SendBuffer> sendBuffer = GSendBufferManager->Open(packetSize);
		auto header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->id = pktId;
		header->size = packetSize;

		pkt.SerializeToArray(&header[1], dataSize);

		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};
