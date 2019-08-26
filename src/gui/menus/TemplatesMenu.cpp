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




void TemplatesMenu::addTemplatesFromDir( const QDir& dir ) {
	QStringList templates = dir.entryList( QStringList( "*.mpt" ),
		QDir::Files | QDir::Readable );

	if ( templates.size() && ! actions().isEmpty() )
	{
		addSeparator();
	}

	for( QStringList::iterator it = templates.begin();
		it != templates.end(); ++it )
	{
		addAction(new CreateFromTemplateAction(dir, *it));
	}
}
