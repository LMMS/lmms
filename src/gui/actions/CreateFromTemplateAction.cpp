#include "CreateFromTemplateAction.h"

#include <QFileInfo>

#include "Engine.h"
#include "Song.h"

#include "embed.h"
#include "GuiApplication.h"
#include "MainWindow.h"

CreateFromTemplateAction::CreateFromTemplateAction(
	const QFileInfo& templateFile) :
	QAction(embed::getIconPixmap( "project_file" ),
			templateFile.completeBaseName().replace("&", "&&")),
	m_templateFilePath(templateFile.absoluteFilePath())
{
#ifdef LMMS_BUILD_APPLE
	setIconVisibleInMenu(false); // QTBUG-44565 workaround
	setIconVisibleInMenu(true);
#endif

	connect(this, SIGNAL(triggered()), this, SLOT(createNewProjectFromTemplate()));
}




void CreateFromTemplateAction::createNewProjectFromTemplate()
{
	if( gui->mainWindow()->mayChangeProject(true) )
	{
		Engine::getSong()->createNewProjectFromTemplate(m_templateFilePath);
	}
}
