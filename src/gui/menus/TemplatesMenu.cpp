#include "TemplatesMenu.h"
#include "GuiApplication.h"
#include "ConfigManager.h"
#include "Engine.h"
#include "embed.h"
#include "MainWindow.h"
#include "Song.h"

TemplatesMenu::TemplatesMenu(QWidget *parent) :
	QMenu(tr("New from template"), parent),
	m_customTemplatesCount(0)
{
	connect( this, SIGNAL( aboutToShow() ), SLOT( fillTemplatesMenu() ) );
	connect( this, SIGNAL( triggered( QAction * ) ),
		SLOT( createNewProjectFromTemplate( QAction * ) ) );
}




void TemplatesMenu::createNewProjectFromTemplate( QAction * _idx )
{
	if( gui->mainWindow()->mayChangeProject(true) )
	{
		int indexOfTemplate = actions().indexOf( _idx );
		bool isFactoryTemplate = indexOfTemplate >= m_customTemplatesCount;
		QString dirBase =  isFactoryTemplate ?
			ConfigManager::inst()->factoryTemplatesDir() :
			ConfigManager::inst()->userTemplateDir();

		const QString f = dirBase + _idx->text().replace("&&", "&") + ".mpt";
		Engine::getSong()->createNewProjectFromTemplate(f);
	}
}





void TemplatesMenu::fillTemplatesMenu()
{
	clear();

	m_customTemplatesCount = addTemplatesFromDir(ConfigManager::inst()->userTemplateDir() );
	addTemplatesFromDir( ConfigManager::inst()->factoryProjectsDir() + "templates" );
}




int TemplatesMenu::addTemplatesFromDir( const QDir& dir ) {
	QStringList templates = dir.entryList( QStringList( "*.mpt" ),
		QDir::Files | QDir::Readable );

	if ( templates.size() && ! actions().isEmpty() )
	{
		addSeparator();
	}

	for( QStringList::iterator it = templates.begin();
		it != templates.end(); ++it )
	{
		addAction(
			embed::getIconPixmap( "project_file" ),
			( *it ).left( ( *it ).length() - 4 ).replace("&", "&&") );
#ifdef LMMS_BUILD_APPLE
		actions().last()->setIconVisibleInMenu(false); // QTBUG-44565 workaround
		actions().last()->setIconVisibleInMenu(true);
#endif
	}

	return templates.size();
}
