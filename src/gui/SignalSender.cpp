#include "SignalSender.h"

SignalSender* SignalSender::getInstance()
{
	static SignalSender* instance = new SignalSender();
	return instance;
}

void SignalSender::closeAll()
{
	emit closeAllSignal();
}