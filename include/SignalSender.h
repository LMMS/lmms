#ifndef SIGNALSENDER_H
#define SIGNALSENDER_H

class SignalSender : public QObject
{
	Q_OBJECT

public:
	static SignalSender* getInstance(QObject* parent = 0);
	void closeAll();

signals:
	void closeAllSignal();
};

#endif