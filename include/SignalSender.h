#ifndef SIGNALSENDER_H
#define SIGNALSENDER_H

#include <QObject>

class SignalSender : public QObject
{
	Q_OBJECT

public:
	static SignalSender* getInstance();
	void closeAll();

signals:
	void closeAllSignal();
};

#endif