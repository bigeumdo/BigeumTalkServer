#pragma once
#include <locale>


/**
 * \brief 프로토콜 enum
 */
enum
{
	C_VERSION_CHECK,
	S_VERSION_CHECK,
	C_LOGIN,
	S_LOGIN,
	C_ENTER_ROOM,
	S_ENTER_ROOM,
	C_LEAVE_ROOM,
	S_LEAVE_ROOM,
	C_CHAT,
	S_CHAT,
	S_OTHER_ENTER,
	S_OTHER_LEAVE
};


/**
 * \brief Result Code enum
 */
enum
{
	LOGIN_EXIST = 2000,
	LOGIN_SUCCESS,
	ENTER_ROOM_FAIL,
	ENTER_ROOM_SUCCESS,
	CHAT_FAIL,
	CHAT_OK,
};

/**
 * \brief PacketHeader 구조체 \n
 * \details 전송될 패킷의 헤더입니다.
 */
struct PacketHeader
{
	unsigned short size;
	unsigned short id;
};

/**
 * \brief wstring을 string으로 변환하는 함수
 * \param var 변환할 wstring
 * \return 변환된 string
 */
static string W2S(const std::wstring& var)
{
	static std::locale loc("");
	auto& facet = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);
	return std::wstring_convert<std::remove_reference_t<decltype(facet)>, wchar_t>(&facet).to_bytes(var);
}


/**
 * \brief string을 wstring으로 변환하는 함수
 * \param var 변환할 string
 * \return 변환된 wstring
 */
static wstring S2W(const std::string& var)
{
	static std::locale loc("");
	auto& facet = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);
	return std::wstring_convert<std::remove_reference_t<decltype(facet)>, wchar_t>(&facet).from_bytes(var);
}


/**
 * \brief Protocol 네임스페이스 \n
 * \details 임의의 프로토콜들의 구조체와 관련 함수들이 포함된 네임스페이스입니다.
 */
namespace Protocol
{
	/**
	 * \brief 버퍼의 내용을 Json::Value로 변환하는 함수
	 * \param buffer 버퍼
	 * \param len 데이터의 길이
	 * \return 변환된 Json::Value
	 */
	static Json::Value Parse(BYTE* buffer, int len)
	{
		Json::Reader reader;
		Json::Value root;
		wstring body;
		body.resize(len / sizeof(WCHAR));

		memcpy((void*)body.data(), buffer, len);

		reader.parse(W2S(body), root);

		return root;
	}

	static Json::StreamWriterBuilder Builder()
	{
		static Json::StreamWriterBuilder builder;
		builder.settings_["emitUTF8"] = true;
		builder.settings_["indentation"] = "";

		return builder;
	}

	/*
		프로토콜 구조체
		
		C: 클라이언트로부터 받은 패킷의 정의
		S: 서버에서 보낼 패킷 정의 
	 */

	struct C_LOGIN
	{
		string nickname;

		void GetPacket(BYTE* buffer, int len)
		{
			Json::Value root = Parse(buffer, len);
			nickname = root["nickname"].asString();
		}
	};

	struct S_LOGIN
	{
		unsigned short resultCode = -1;
		unsigned long long userId = 0;

		string MakeJson()
		{
			Json::Value root;
			root["resultCode"] = resultCode;
			root["userId"] = userId;

			return writeString(Builder(), root);
		}
	};

	struct C_ENTER_ROOM
	{
		unsigned long long roomId;
		unsigned long long userId;

		void GetPacket(BYTE* buffer, int len)
		{
			Json::Value root = Parse(buffer, len);
			roomId = root["roomId"].asUInt64();
			userId = root["userId"].asUInt64();
		}
	};

	struct S_ENTER_ROOM
	{
		unsigned short resultCode;
		unsigned long long roomId;
		string roomName;
		vector<pair<unsigned long long, string>> users;

		string MakeJson()
		{
			Json::Value root;
			root["resultCode"] = resultCode;
			root["roomId"] = roomId;
			root["roomName"] = roomName;

			for (auto& user : users)
			{
				Json::Value child;
				child["userId"] = user.first;
				child["nickname"] = user.second;
				root["users"].append(child);
			}

			return writeString(Builder(), root);
		}
	};

	struct C_CHAT
	{
		unsigned long long userId;
		string nickname;
		string message;

		void GetPacket(BYTE* buffer, int len)
		{
			Json::Value root = Parse(buffer, len);
			userId = root["userId"].asUInt64();
			nickname = root["nickname"].asString();
			message = root["message"].asString();
		}
	};

	struct S_CHAT
	{
		unsigned short resultCode;
		bool serverMessage;
		unsigned long long userId;
		string nickname;
		string message;
		unsigned long long timestamp;

		string MakeJson()
		{
			Json::Value root;
			root["resultCode"] = resultCode;
			root["serverMessage"] = serverMessage;
			root["userId"] = userId;
			root["nickname"] = nickname;
			root["message"] = message;
			root["timestamp"] = timestamp;

			return writeString(Builder(), root);
		}
	};

	// 프로토콜 전용 유저 클래스
	struct PROTOCOL_USER
	{
		unsigned long long userId;
		string nickName;
	};

	struct S_OTHER_ENTER
	{
		PROTOCOL_USER user;
		unsigned long long timestamp;

		string MakeJson()
		{
			Json::Value root;
			root["user"]["userId"] = user.userId;
			root["user"]["nickName"] = user.nickName;
			root["timestamp"] = timestamp;

			return writeString(Builder(), root);
		}
	};

	struct S_OTHER_LEAVE
	{
		PROTOCOL_USER user;
		unsigned long long timestamp;

		string MakeJson()
		{
			Json::Value root;
			root["user"]["userId"] = user.userId;
			root["user"]["nickName"] = user.nickName;
			root["timestamp"] = timestamp;

			return writeString(Builder(), root);
		}
	};
}
