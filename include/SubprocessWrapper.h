// note: that in Qt4 qmake MOC will not define __linux__, so the slots need to be defined outside of the macro
// this was not required in Qt5!! fixed by moving #ifdef __linux__ to SubprocessWrapper.cpp

#ifndef SUBPROCESSWRAPPER_H
#define SUBPROCESSWRAPPER_H

#include <QtCore/QProcess>
#include <QMetaType>

#ifdef __linux__
#include <QX11EmbedContainer>
#endif

class SubprocessWrapper : public QObject {
	Q_OBJECT
	Q_PROPERTY(QProcess* subprocess READ getProcess)
	QProcess* subprocess;
	unsigned long xid;
	unsigned long pid;
#ifdef __linux__
	QX11EmbedContainer* embedContainer;
#endif
	public slots:
		void captureWindow();
		void embedClient(unsigned long windowID);
		unsigned long getWindowID();
		void handleClientEmbed();

	public:
		SubprocessWrapper(){};
		SubprocessWrapper(QString exe, QStringList args, bool capture=false, QWidget* parent=NULL, int width=800, int height=600);
		inline QProcess* getProcess() { return this->subprocess; }
};

Q_DECLARE_METATYPE( SubprocessWrapper* )

#endif