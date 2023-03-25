#include "pch.h"
#include "User.h"

#include "Service.h"

User::~User()
{
#ifdef _DEBUG
	// TEMP LOG
	cout << "[USER LEAVED] " << '[' << userId << "] " << nickname << endl;
#endif
}
