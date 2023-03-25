#pragma once

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <mutex>
#include <memory>
#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <atomic>

#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

#include "Global.h"
#include "SendBuffer.h"
#include "Session.h"
#include "json/json.h"

/* Jsoncpp */
#ifdef _DEBUG
#pragma comment(lib, "jsoncpp\\Debug\\jsoncpp.lib")
#else
#pragma comment(lib, "jsoncpp\\Release\\jsoncpp.lib")
#endif


/* Thread Local Storage */
extern thread_local shared_ptr<SendBufferChunk> LSendBufferChunk; // 스레드 로컬 송신 버퍼
