#include "SignalSender.h"

static SignalSender* getInstance(QObject* parent = 0)
{
	static SignalSender* instance = new SignalSender(parent);
	return instance;
}

void closeAll()
{
	emit closeAllSignal();
}