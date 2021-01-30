#include "SignalSender.h"

SignalSender* SignalSender::getInstance()
{
	return new SignalSender();
}

void SignalSender::closeAll()
{
	emit closeAllSignal();
}

void SignalSender::closeAllButThis(SubWindow* source)
{
	emit closeAllButThisSignal(source);
}
