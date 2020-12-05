#ifndef _INET_EVENT_INCLUDED
#define _INET_EVENT_INCLUDED

#include "Cell.h"

class INetEvent
{
public:
	virtual void OnNetLeave(Client*) = 0;

	virtual void OnNetMsg(CellServer*, Client*, DataHeader*) = 0;

	virtual void OnNetRecv(Client*) = 0;
};


#endif