#ifndef SIGNALSENDER_H
#define SIGNALSENDER_H

#include <QObject>

class SignalSender : public QObject
{
	Q_OBJECT

public:
	static SignalSender* getInstance();

signals:
	void closeAllSignal();
	void closeAllButThisSignal(SubWindow * source);

public slots:
	void closeAll();
	void closeAllButThis(SubWindow* source);
};

#endif
