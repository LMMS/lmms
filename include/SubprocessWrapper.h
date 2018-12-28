#ifndef SUBPROCESSWRAPPER_H
#define SUBPROCESSWRAPPER_H

#include <QMdiArea>
#include <QtCore/QProcess>
#ifdef __linux__
#include "X11EmbedContainer.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#endif

class SubprocessWrapper : public QObject {
	Q_OBJECT
	Q_PROPERTY(QProcess* subprocess MEMBER subprocess)
	QProcess* subprocess;
#ifdef __linux__
	QX11EmbedContainer* embedContainer;
	int xid;
	public slots:
		inline void embedClient(int windowID) {
			//connect(embedContainer, SIGNAL(clientIsEmbedded()), this, SLOT(handleClientEmbed()));
			this->embedContainer->embedClient( windowID );
		};
		inline int getWindowID() {
			return this->xid;
		}
#endif

	public:
		SubprocessWrapper(){};
		SubprocessWrapper(QString exe, QStringList args, int width, int height) {
#ifdef __linux__
			embedContainer = new QX11EmbedContainer();
			embedContainer->setMinimumSize(width, height);
			gui->mainWindow()->workspace()->addSubWindow(embedContainer);
			embedContainer->show();  // this also sets the xid
			this->xid = embedContainer->winId();
			this->subprocess = new QProcess( embedContainer );
			args << QString::number(xid);
			subprocess->start(exe, args);
#else
			this->subprocess = new QProcess();
			subprocess->start(exe, args);

#endif
		};
};

#endif