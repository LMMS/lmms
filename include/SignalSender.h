#ifndef SIGNALSENDER_H
#define SIGNALSENDER_H

#include <QObject>
#include "SubWindow.h"

class SignalSender : public QObject
{
	Q_OBJECT

public:
	static SignalSender* getInstance();

signals:
	void closeAllSignal();
	void closeOthersSignal(SubWindow * source);

public slots:
	void closeAll();
	void closeOthers(SubWindow* source);
};

#endif
