#include "SignalSender.h"

static SignalSender* getInstance()
{
	static SignalSender* instance = new SignalSender();
	return instance;
}

void closeAll()
{
	emit closeAllSignal();
}