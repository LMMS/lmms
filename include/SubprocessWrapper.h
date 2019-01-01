#ifndef SUBPROCESSWRAPPER_H
#define SUBPROCESSWRAPPER_H

#include <QtCore/QProcess>

#ifdef __linux__
#include <QX11EmbedContainer>
#endif

class SubprocessWrapper : public QObject {
	Q_OBJECT
	//TODO//Q_PROPERTY(QProcess* subprocess MEMBER subprocess)
	QProcess* subprocess;
#ifdef __linux__
	QX11EmbedContainer* embedContainer;
	unsigned long xid;
	unsigned long pid;
	signals:
		void captureWindow();

	public slots:
		void embedClient(unsigned long windowID);
		unsigned long getWindowID();
		void handleClientEmbed();
#endif

	public:
		SubprocessWrapper(){};
		SubprocessWrapper(QString exe, QStringList args, bool capture=false, QWidget* parent=NULL, int width=800, int height=600);
};

#endif