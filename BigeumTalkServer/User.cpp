#include "pch.h"
#include "User.h"

#include "Service.h"

User::~User()
{
	ownerSession->GetService()->ReleaseNickname(nickname);
	ownerSession = nullptr;
#ifdef _DEBUG
	// TEMP LOG
	cout << "[USER DESTROYED] " << '[' << userId << "] " << nickname << endl;
#endif
}
