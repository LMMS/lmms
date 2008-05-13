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
			bitrateCB->currentText().section( " ", 0, 0 ).toUInt() );

	m_renderer = new projectRenderer( qs, os,
		static_cast<projectRenderer::ExportFileTypes>(
						fileTypeCB->currentIndex() ),
								m_fileName );
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




void exportProjectDialog::updateTitleBar( int _prog )
{
	engine::getMainWindow()->setWindowTitle(
					tr( "Rendering: %1%" ).arg( _prog ) );
}



#include "export_project_dialog.moc"


#endif
