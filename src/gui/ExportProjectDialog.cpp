/*
 * ExportProjectDialog.cpp - implementation of dialog for exporting project
 *
 * Copyright (c) 2004-2013 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

#include "ExportProjectDialog.h"
#include "Song.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "BBTrackContainer.h"
#include "BBTrack.h"


ExportProjectDialog::ExportProjectDialog( const QString & _file_name,
							QWidget * _parent, bool multi_export=false ) :
	QDialog( _parent ),
	Ui::ExportProjectDialog(),
	m_fileName( _file_name ),
	m_fileExtension(),
	m_multiExport( multi_export ),
	m_activeRenderer( NULL )
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




ExportProjectDialog::~ExportProjectDialog()
{

	for( RenderVector::ConstIterator it = m_renderers.begin();
							it != m_renderers.end(); ++it )
	{
		delete (*it);
	}
}




void ExportProjectDialog::reject()
{
	for( RenderVector::ConstIterator it = m_renderers.begin(); it != m_renderers.end(); ++it )
	{
		(*it)->abortProcessing();
	}

	if( m_activeRenderer ) {
		m_activeRenderer->abortProcessing();
	}

	QDialog::reject();
}



void ExportProjectDialog::accept()
{
	// If more to render, kick off next render job
	if( m_renderers.isEmpty() == false )
	{
		popRender();
	}
	else
	{
		// If done, then reset mute states
		while( m_unmuted.isEmpty() == false )
		{
			Track* restoreTrack = m_unmuted.back();
			m_unmuted.pop_back();
			restoreTrack->setMuted( false );
		}

		QDialog::accept();

	}
}




void ExportProjectDialog::closeEvent( QCloseEvent * _ce )
{
	for( RenderVector::ConstIterator it = m_renderers.begin(); it != m_renderers.end(); ++it )
	{
		if( (*it)->isRunning() )
		{
			(*it)->abortProcessing();
		}
	}

	if( m_activeRenderer && m_activeRenderer->isRunning() ) {
		m_activeRenderer->abortProcessing();
	}

	QDialog::closeEvent( _ce );
}



void ExportProjectDialog::popRender()
{
	if( m_multiExport && m_tracksToRender.isEmpty() == false )
	{
		Track* renderTrack = m_tracksToRender.back();
		m_tracksToRender.pop_back();

		// Set must states for song tracks
		for( TrackVector::ConstIterator it = m_unmuted.begin(); it != m_unmuted.end(); ++it )
		{
			if( (*it) == renderTrack )
			{
				(*it)->setMuted( false );
			}
			else
			{
				(*it)->setMuted( true );
			}
		}
	}


	// Pop next render job and start
	m_activeRenderer = m_renderers.back();
	m_renderers.pop_back();
	render( m_activeRenderer );
}



void ExportProjectDialog::multiRender()
{
	m_dirName = m_fileName;
	QString path = QDir(m_fileName).filePath("text.txt");

	int x = 1;

	const TrackContainer::TrackList & tl = Engine::getSong()->tracks();

	// Check for all unmuted tracks. Remember list.
	for( TrackContainer::TrackList::ConstIterator it = tl.begin();
							it != tl.end(); ++it )
	{
		Track* tk = (*it);
		Track::TrackTypes type = tk->type();
		// Don't mute automation tracks
		if ( tk->isMuted() == false &&
				( type == Track::InstrumentTrack || type == Track::SampleTrack ) )
		{
			m_unmuted.push_back(tk);
			QString nextName = tk->name();
			nextName = nextName.remove(QRegExp("[^a-zA-Z]"));
			QString name = QString( "%1_%2%3" ).arg( x++ ).arg( nextName ).arg( m_fileExtension );
			m_fileName = QDir(m_dirName).filePath(name);
			prepRender();
		}
		else if (! tk->isMuted() && type == Track::BBTrack )
		{
			m_unmutedBB.push_back(tk);
		}


	}

	const TrackContainer::TrackList t2 = Engine::getBBTrackContainer()->tracks();
	for( TrackContainer::TrackList::ConstIterator it = t2.begin(); it != t2.end(); ++it )
	{
		Track* tk = (*it);
		if ( tk->isMuted() == false )
		{
			m_unmuted.push_back(tk);
			QString nextName = tk->name();
			nextName = nextName.remove(QRegExp("[^a-zA-Z]"));
			QString name = QString( "%1_%2%3" ).arg( x++ ).arg( nextName ).arg( m_fileExtension );
			m_fileName = QDir(m_dirName).filePath(name);
			prepRender();
		}
	}


	m_tracksToRender = m_unmuted;

	popRender();
}



ProjectRenderer* ExportProjectDialog::prepRender()
{
	Mixer::qualitySettings qs =
			Mixer::qualitySettings(
					static_cast<Mixer::qualitySettings::Interpolation>(interpolationCB->currentIndex()),
					static_cast<Mixer::qualitySettings::Oversampling>(oversamplingCB->currentIndex()) );

	ProjectRenderer::OutputSettings os = ProjectRenderer::OutputSettings(
			samplerateCB->currentText().section(" ", 0, 0).toUInt(),
			false,
			bitrateCB->currentText().section(" ", 0, 0).toUInt(),
			static_cast<ProjectRenderer::Depths>( depthCB->currentIndex() ) );

	Engine::getSong()->setExportLoop( exportLoopCB->isChecked() );

	ProjectRenderer* renderer = new ProjectRenderer( qs, os, m_ft, m_fileName );

	m_renderers.push_back(renderer);

	return renderer;
}



void ExportProjectDialog::render( ProjectRenderer* renderer )
{

	if( renderer->isReady() )
	{
		connect( renderer, SIGNAL( progressChanged( int ) ), progressBar, SLOT( setValue( int ) ) );
		connect( renderer, SIGNAL( progressChanged( int ) ), this, SLOT( updateTitleBar( int ) )) ;
		connect( renderer, SIGNAL( finished() ), this, SLOT( accept() ) );
		connect( renderer, SIGNAL( finished() ), gui->mainWindow(), SLOT( resetWindowTitle() ) );

		renderer->startProcessing();
	}
	else
	{
		accept();
	}
}



void ExportProjectDialog::startBtnClicked()
{
	m_ft = ProjectRenderer::NumFileFormats;

	for( int i = 0; i < ProjectRenderer::NumFileFormats; ++i )
	{
		if( fileFormatCB->currentText() ==
			ProjectRenderer::tr(
				__fileEncodeDevices[i].m_description ) )
		{
			m_ft = __fileEncodeDevices[i].m_fileFormat;
			m_fileExtension = QString( QLatin1String( __fileEncodeDevices[i].m_extension ) );
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

	if (m_multiExport==true)
	{
		multiRender();
	}
	else
	{
		prepRender();
		popRender();
	}
}




void ExportProjectDialog::updateTitleBar( int _prog )
{
	gui->mainWindow()->setWindowTitle(
					tr( "Rendering: %1%" ).arg( _prog ) );
}
