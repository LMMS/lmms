#ifndef CREATEFROMTEMPLATEACTION_H
#define CREATEFROMTEMPLATEACTION_H

#include <QAction>
#include <QFileInfo>

class CreateFromTemplateAction : public QAction {
	Q_OBJECT
public:
	CreateFromTemplateAction(const QFileInfo& templateFile);

private:
	QString m_templateFilePath;

private slots:
	void createNewProjectFromTemplate();
};

#endif // CREATEFROMTEMPLATEACTION_H
