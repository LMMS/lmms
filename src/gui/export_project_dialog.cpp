#ifndef SINGLE_SOURCE_COMPILE

/*
 * export_project_dialog.cpp - implementation of dialog for exporting project
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtGui/QMessageBox>


#include "export_project_dialog.h"
#include "engine.h"
#include "main_window.h"
#include "project_renderer.h"


exportProjectDialog::exportProjectDialog( const QString & _file_name,
							QWidget * _parent ) :
	QDialog( _parent ),
	Ui::ExportProjectDialog(),
	m_fileName( _file_name ),
	m_renderer( NULL )
{
	setupUi( this );
	setWindowTitle( tr( "Export project to %1" ).arg( 
					QFileInfo( _file_name ).fileName() ) );

	for( int i = 0; i < projectRenderer::NumFileFormats; ++i )
	{
		if( __fileEncodeDevices[i].m_getDevInst != NULL )
		{
			fileFormatCB->addItem( projectRenderer::tr(
				__fileEncodeDevices[i].m_description ) );
		}
	}

	connect( startButton, SIGNAL( clicked() ),
			this, SLOT( startBtnClicked() ) );

}




exportProjectDialog::~exportProjectDialog()
{
	delete m_renderer;
}




void exportProjectDialog::reject( void )
{
	if( m_renderer == NULL )
	{
		accept();
	}
	else
	{
		m_renderer->abortProcessing();
	}
}




void exportProjectDialog::closeEvent( QCloseEvent * _ce )
{
	if( m_renderer != NULL && m_renderer->isRunning() )
	{
		m_renderer->abortProcessing();
	}
	QDialog::closeEvent( _ce );
}




void exportProjectDialog::startBtnClicked( void )
{
	projectRenderer::ExportFileFormats ft = projectRenderer::NumFileFormats;

	for( int i = 0; i < projectRenderer::NumFileFormats; ++i )
	{
		if( fileFormatCB->currentText() ==
			projectRenderer::tr(
				__fileEncodeDevices[i].m_description ) )
		{
			ft = __fileEncodeDevices[i].m_fileFormat;
			break;
		}
	}

	if( ft == projectRenderer::NumFileFormats )
	{
		QMessageBox::information( this, tr( "Error" ),
			tr( "Error while determining file-encoder device. "
				"Please try to choose a different output "
							"format." ) );
		reject();
		return;
	}

	startButton->setEnabled( FALSE );
	progressBar->setEnabled( TRUE );

	updateTitleBar( 0 );

	mixer::qualitySettings qs = mixer::qualitySettings(
		static_cast<mixer::qualitySettings::Interpolation>(
					interpolationCB->currentIndex() ),
		static_cast<mixer::qualitySettings::Oversampling>(
					oversamplingCB->currentIndex() ),
					sampleExactControllersCB->isChecked(),
					aliasFreeOscillatorsCB->isChecked() );

	projectRenderer::outputSettings os = projectRenderer::outputSettings(
		samplerateCB->currentText().section( " ", 0, 0 ).toUInt(),
		FALSE,
		bitrateCB->currentText().section( " ", 0, 0 ).toUInt(),
		static_cast<projectRenderer::Depths>(
						depthCB->currentIndex() ) );

	m_renderer = new projectRenderer( qs, os, ft, m_fileName );
	if( m_renderer->isReady() )
	{
		connect( m_renderer, SIGNAL( progressChanged( int ) ),
				progressBar, SLOT( setValue( int ) ) );
		connect( m_renderer, SIGNAL( progressChanged( int ) ),
				this, SLOT( updateTitleBar( int ) ) );
		connect( m_renderer, SIGNAL( finished() ),
				this, SLOT( accept() ) );
		connect( m_renderer, SIGNAL( finished() ),
			engine::getMainWindow(), SLOT( resetWindowTitle() ) );

		m_renderer->startProcessing();
	}
	else
	{
		accept();
	}
}




void exportProjectDialog::updateTitleBar( int _prog )
{
	engine::getMainWindow()->setWindowTitle(
					tr( "Rendering: %1%" ).arg( _prog ) );
}



#include "moc_export_project_dialog.cxx"


#endif
