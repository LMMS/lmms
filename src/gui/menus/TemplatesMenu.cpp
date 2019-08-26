#include "TemplatesMenu.h"

#include "ConfigManager.h"

#include "CreateFromTemplateAction.h"

TemplatesMenu::TemplatesMenu(QWidget *parent) :
	QMenu(tr("New from template"), parent)
{
	connect( this, SIGNAL( aboutToShow() ), SLOT( fillTemplatesMenu() ) );
}





void TemplatesMenu::fillTemplatesMenu()
{
	clear();

	addTemplatesFromDir(ConfigManager::inst()->userTemplateDir() );
	addTemplatesFromDir( ConfigManager::inst()->factoryProjectsDir() + "templates" );
}




void TemplatesMenu::addTemplatesFromDir(const QDir& templateDir) {
	QList<QFileInfo> templates = templateDir.entryInfoList(
		QStringList("*.mpt" ), QDir::Files | QDir::Readable );

	if ( !templates.empty() && ! actions().isEmpty() )
	{
		addSeparator();
	}

	for(const QFileInfo& fileInfo : templates)
	{
		addAction(new CreateFromTemplateAction(fileInfo));
	}
}
