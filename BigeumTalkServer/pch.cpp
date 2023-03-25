#include "pch.h"

/* Thread Local */
thread_local shared_ptr<SendBufferChunk> LSendBufferChunk; // 스레드 로컬 송신 버퍼
