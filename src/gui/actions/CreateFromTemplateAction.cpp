#include "CreateFromTemplateAction.h"

#include <QFileInfo>

#include "Engine.h"
#include "Song.h"

#include "embed.h"
#include "GuiApplication.h"
#include "MainWindow.h"

inline const QString& actionTextFromFilename(const QString& filename) {
	return filename.left(filename.length() - 4 ).replace("&", "&&");
}

CreateFromTemplateAction::CreateFromTemplateAction(
	const QDir &templateDir, const QString &templateFilename) :
	QAction(embed::getIconPixmap( "project_file" ),
		actionTextFromFilename(templateFilename)),
	m_templateFilePath(templateDir.absoluteFilePath(templateFilename))
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
