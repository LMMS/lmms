/*
 * export_project_dialog.cpp - implementation of dialog for exporting project
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtGui/QMessageBox>

#include "export_project_dialog.h"
#include "song.h"
#include "engine.h"
#include "MainWindow.h"


exportProjectDialog::exportProjectDialog( const QString & _file_name,
							QWidget * _parent, bool multi_export=false ) :
	QDialog( _parent ),
	Ui::ExportProjectDialog(),
	m_fileName( _file_name ),
	m_multi_export(multi_export)
{
	setupUi( this );
	setWindowTitle( tr( "Export project to %1" ).arg( 
					QFileInfo( _file_name ).fileName() ) );

	// get the extension of the chosen file
	QStringList parts = _file_name.split( '.' );
	QString fileExt;
	if( parts.size() > 0 )
	{
		fileExt = "." + parts[parts.size()-1];
	}

	int cbIndex = 0;
	for( int i = 0; i < ProjectRenderer::NumFileFormats; ++i )
	{
		if( __fileEncodeDevices[i].m_getDevInst != NULL )
		{
			// get the extension of this format
			QString renderExt = __fileEncodeDevices[i].m_extension;

			// add to combo box
			fileFormatCB->addItem( ProjectRenderer::tr(
				__fileEncodeDevices[i].m_description ) );

			// if this is our extension, select it
			if( QString::compare( renderExt, fileExt,
									Qt::CaseInsensitive ) == 0 )
			{
				fileFormatCB->setCurrentIndex( cbIndex );
			}

			cbIndex++;
		}
	}

	connect( startButton, SIGNAL( clicked() ),
			this, SLOT( startBtnClicked() ) );

}




exportProjectDialog::~exportProjectDialog()
{

	for( std::vector<ProjectRenderer*>::const_iterator it = m_renderers.begin();
							it != m_renderers.end(); ++it )
	{
		delete (*it);
	}
}




void exportProjectDialog::reject()
{
	for( std::vector<ProjectRenderer*>::const_iterator it = m_renderers.begin();
							it != m_renderers.end(); ++it )
	{
		(*it)->abortProcessing();
	}
}
void exportProjectDialog::accept()
{
	// If more to render, kick off next render job
	if (m_renderers.size() > 0)
	{
		pop_render( );
	}
	else
	{
		// If done, then reset mute states
		while(!m_unmuted.empty())
		{
			track* restore_track = m_unmuted.back();
			m_unmuted.pop_back();
			restore_track->setMuted(false);
		}

		QDialog::accept();

	}
}




void exportProjectDialog::closeEvent( QCloseEvent * _ce )
{
	for( std::vector<ProjectRenderer*>::const_iterator it = m_renderers.begin();
							it != m_renderers.end(); ++it )
	{
		if( (*it)->isRunning() )
		{
			(*it)->abortProcessing();
		}
	}
	QDialog::closeEvent( _ce );
}

void exportProjectDialog::pop_render() {

	track* render_track = m_to_render_vec.back();
	m_to_render_vec.pop_back();

	for (std::vector<track*>::const_iterator it = m_unmuted.begin();
			it != m_unmuted.end(); ++it) {
		if ((*it) == render_track) {
			(*it)->setMuted(false);
		} else {
			(*it)->setMuted(true);
		}
	}

	// Pop next render job and start
	ProjectRenderer* r = m_renderers.back();
	m_renderers.pop_back();
	render(r);
}

void exportProjectDialog::multi_render()
{
	m_dirName = m_fileName;
	QString path = QDir(m_fileName).filePath("text.txt");
	std::string strTest = path.toStdString();

	const trackContainer::trackList & tl = engine::getSong()->tracks();

	// Check for all unmuted tracks.  Remember list.
	int x = 0;
	for( trackContainer::trackList::const_iterator it = tl.begin();
							it != tl.end(); ++it )
	{
		// Don't mute automation tracks
		if (! (*it)->isMuted() && (*it)->nodeName() != "automationtrack")
		{
			m_unmuted.push_back((*it));
			QString nextName = (*it)->name();
			nextName = nextName.remove(QRegExp("[^a-zA-Z]"));
			QString name = QString("%1_%2.wav").arg(x++).arg(nextName);
			m_fileName = QDir(m_dirName).filePath(name);
			std::string strTest = m_fileName.toStdString();
			prep_render();
		}
	}

	m_to_render_vec = m_unmuted;

	pop_render( );
}

ProjectRenderer* exportProjectDialog::prep_render(
		) {
	mixer::qualitySettings qs =
			mixer::qualitySettings(
					static_cast<mixer::qualitySettings::Interpolation>(interpolationCB->currentIndex()),
					static_cast<mixer::qualitySettings::Oversampling>(oversamplingCB->currentIndex()),
					sampleExactControllersCB->isChecked(),
					aliasFreeOscillatorsCB->isChecked());
	ProjectRenderer::OutputSettings os = ProjectRenderer::OutputSettings(
			samplerateCB->currentText().section(" ", 0, 0).toUInt(), false,
			bitrateCB->currentText().section(" ", 0, 0).toUInt(),
			static_cast<ProjectRenderer::Depths>(depthCB->currentIndex()));
	ProjectRenderer* renderer = new ProjectRenderer(qs, os, m_ft, m_fileName);
	m_renderers.push_back(renderer);
	return renderer;
}

void exportProjectDialog::render(ProjectRenderer* renderer)
{

	if (renderer->isReady()) {
		connect(renderer, SIGNAL( progressChanged( int ) ), progressBar,
				SLOT( setValue( int ) ));
		connect(renderer, SIGNAL( progressChanged( int ) ), this,
				SLOT( updateTitleBar( int ) ));
		connect(renderer, SIGNAL( finished() ), this, SLOT( accept() ));
		connect(renderer, SIGNAL( finished() ), engine::mainWindow(),
				SLOT( resetWindowTitle() ));

		renderer->startProcessing();
	} else {
		accept();
	}
}

void exportProjectDialog::startBtnClicked()
{
	m_ft = ProjectRenderer::NumFileFormats;

	for( int i = 0; i < ProjectRenderer::NumFileFormats; ++i )
	{
		if( fileFormatCB->currentText() ==
			ProjectRenderer::tr(
				__fileEncodeDevices[i].m_description ) )
		{
			m_ft = __fileEncodeDevices[i].m_fileFormat;
			break;
		}
	}

	if( m_ft == ProjectRenderer::NumFileFormats )
	{
		QMessageBox::information( this, tr( "Error" ),
			tr( "Error while determining file-encoder device. "
				"Please try to choose a different output "
							"format." ) );
		reject();
		return;
	}

	startButton->setEnabled( false );
	progressBar->setEnabled( true );

	updateTitleBar( 0 );

	if (m_multi_export==true)
	{
		multi_render();
	}
	else
	{
		render(prep_render());
	}
}




void exportProjectDialog::updateTitleBar( int _prog )
{
	engine::mainWindow()->setWindowTitle(
					tr( "Rendering: %1%" ).arg( _prog ) );
}



#include "moc_export_project_dialog.cxx"


/* vim: set tw=0 noexpandtab: */
