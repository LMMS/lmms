#ifndef CREATEFROMTEMPLATEACTION_H
#define CREATEFROMTEMPLATEACTION_H

#include <QAction>
#include <QDir>

class CreateFromTemplateAction : public QAction {
	Q_OBJECT
public:
	CreateFromTemplateAction(const QDir &templateDir, const QString &templateFilename);

private:
	QString m_templateFilePath;

private slots:
	void createNewProjectFromTemplate();
};

#endif // CREATEFROMTEMPLATEACTION_H
