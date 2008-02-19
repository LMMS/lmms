#ifndef SINGLE_SOURCE_COMPILE

/*
 * export_project_dialog.cpp - implementation of dialog for exporting project
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QProgressBar>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QCloseEvent>
#include <QtGui/QApplication>


#include "export_project_dialog.h"
#include "song.h"
#include "main_window.h"
#include "combobox.h"
#include "led_checkbox.h"
#include "embed.h"
#include "engine.h"

#include "audio_file_wave.h"
#include "audio_file_ogg.h"


extern QString file_to_render;


fileEncodeDevice fileEncodeDevices[] =
{

	{ WaveFile, QT_TRANSLATE_NOOP( "exportProjectDialog",
					"Uncompressed Wave-File (*.wav)" ),
					".wav", &audioFileWave::getInst },
#ifdef HAVE_VORBIS_CODEC_H
	{ OggFile, QT_TRANSLATE_NOOP( "exportProjectDialog",
					"Compressed OGG-File (*.ogg)" ),
					".ogg", &audioFileOgg::getInst },
#endif
	// ... insert your own file-encoder-infos here... may be one day the
	// user can add own encoders inside the program...

	{ NullFile, NULL, NULL, NULL }

} ;



const int LABEL_MARGIN = 6;
const int LABEL_X = 10;
const int LABEL_WIDTH = 48;
const int TYPE_STUFF_Y = 10;
const int TYPE_HEIGHT = 22;
const int TYPE_COMBO_WIDTH = 256;
const int KBPS_STUFF_Y = TYPE_STUFF_Y + TYPE_HEIGHT + LABEL_MARGIN + 6;
const int KBPS_HEIGHT = TYPE_HEIGHT;
const int KBPS_COMBO_WIDTH = 64;
const int HQ_MODE_CB_Y = KBPS_STUFF_Y + KBPS_HEIGHT + LABEL_MARGIN;
const int HQ_MODE_CB_HEIGHT = TYPE_HEIGHT;
const int HQ_MODE_CB_WIDTH = 300;
const int CTL_BUTTONS_Y = HQ_MODE_CB_Y + HQ_MODE_CB_HEIGHT + LABEL_MARGIN + 6;
const int CTL_BUTTONS_HEIGHT = 30;
const int CTL_BUTTONS_WIDTH = 120;
const int HOURGLASS_X = LABEL_X;
const int HOURGLASS_Y = 24;
const int HOURGLASS_WIDTH = 44;
const int HOURGLASS_HEIGHT = 56;

const int EPB_X = LABEL_X + 60;
const int EPB_Y = HOURGLASS_Y;
const int EPB_WIDTH = 260;
const int EPB_HEIGHT = 24;
const int CANCEL_X_WHILE_EXPORT = 136;
const int CANCEL_Y_WHILE_EXPORT = EPB_Y + EPB_HEIGHT + 32;


Sint16 exportProjectDialog::s_availableBitrates[] =
{
	64,
	80,
	96,
	112,
	128,
	160,
	192,
	256,
	320,
	-1
};


// TODO: rewrite that crap using layouts!!

exportProjectDialog::exportProjectDialog( const QString & _file_name,
							QWidget * _parent ) :
	QDialog( _parent ),
	m_typeModel( new comboBoxModel( /* this */ ) ),
	m_kbpsModel( new comboBoxModel( /* this */ ) ),
	m_vbrEnabledModel( new boolModel( /* this */ ) ),
	m_hqmEnabledModel( new boolModel( /* this */ ) ),
	m_fileName( _file_name ),
	m_deleteFile( FALSE )
{
	m_fileType = getFileTypeFromExtension( "." +
						QFileInfo( _file_name
							).completeSuffix() );
	setWindowTitle( tr( "Export project to %1" ).arg( 
					QFileInfo( _file_name ).fileName() ) );

	// type-ui-stuff
	m_typeLbl = new QLabel( tr( "Type:" ), this );
	m_typeLbl->setGeometry( LABEL_X, TYPE_STUFF_Y, LABEL_WIDTH,
								TYPE_HEIGHT );

	Uint8 idx = 0;
	while( fileEncodeDevices[idx].m_fileType != NullFile )
	{
		m_typeModel->addItem(
				tr( fileEncodeDevices[idx].m_description ) );
		++idx;
	}
	m_typeModel->setValue( m_typeModel->findText( tr(
			fileEncodeDevices[m_fileType].m_description ) ) );

	m_typeCombo = new comboBox( this );
	m_typeCombo->setGeometry( LABEL_X + LABEL_WIDTH+LABEL_MARGIN,
					TYPE_STUFF_Y, TYPE_COMBO_WIDTH,
								TYPE_HEIGHT );
	m_typeCombo->setModel( m_typeModel );
/*	connect( m_typeCombo, SIGNAL( activated( const QString & ) ), this,
				SLOT( changedType( const QString & ) ) );*/


	// kbps-ui-stuff
	m_kbpsLbl = new QLabel( tr( "kbps:" ), this );
	m_kbpsLbl->setGeometry( LABEL_X, KBPS_STUFF_Y, LABEL_WIDTH,
								KBPS_HEIGHT );

	idx = 0;
	while( s_availableBitrates[idx] != -1 )
	{
		m_kbpsModel->addItem( QString::number(
						s_availableBitrates[idx] ) );
		++idx;
	}
	m_kbpsModel->setValue( m_kbpsModel->findText(
						QString::number( 128 ) ) );

	m_kbpsCombo = new comboBox( this );
	m_kbpsCombo->setModel( m_kbpsModel );
	m_kbpsCombo->setGeometry( LABEL_X + LABEL_WIDTH + LABEL_MARGIN,
						KBPS_STUFF_Y, KBPS_COMBO_WIDTH,
								KBPS_HEIGHT );


	m_vbrCb = new ledCheckBox( tr( "variable bitrate" ), this );
	m_vbrCb->setModel( m_vbrEnabledModel );
	m_vbrCb->setGeometry( LABEL_X + LABEL_WIDTH + 3 * LABEL_MARGIN +
				KBPS_COMBO_WIDTH, KBPS_STUFF_Y + 3, 190, 20 );
	m_vbrCb->setChecked( TRUE );


	m_hqmCb = new ledCheckBox( tr( "use high-quality-mode (recommened)" ),
									this );
	m_hqmCb->setModel( m_hqmEnabledModel );
	m_hqmCb->setGeometry( LABEL_X, HQ_MODE_CB_Y + 3, HQ_MODE_CB_WIDTH,
							HQ_MODE_CB_HEIGHT );
	m_hqmCb->setChecked( TRUE );


	m_exportBtn = new QPushButton( embed::getIconPixmap( "apply" ),
							tr( "Export" ), this );
	m_exportBtn->setGeometry( LABEL_X + LABEL_WIDTH + LABEL_MARGIN,
					CTL_BUTTONS_Y, CTL_BUTTONS_WIDTH,
							CTL_BUTTONS_HEIGHT );
	connect( m_exportBtn, SIGNAL( clicked() ), this,
						SLOT( exportBtnClicked() ) );

	m_cancelBtn = new QPushButton( embed::getIconPixmap( "cancel" ),
							tr( "Cancel" ), this );
	m_cancelBtn->setGeometry( LABEL_X + LABEL_WIDTH + 2 * LABEL_MARGIN +
					CTL_BUTTONS_WIDTH, CTL_BUTTONS_Y,
					CTL_BUTTONS_WIDTH, CTL_BUTTONS_HEIGHT );
	connect( m_cancelBtn, SIGNAL( clicked() ), this,
						SLOT( cancelBtnClicked() ) );

}




exportProjectDialog::~exportProjectDialog()
{
}




// little help-function for getting file-type from a file-extension (only for
// registered file-encoders)
ExportFileTypes exportProjectDialog::getFileTypeFromExtension(
							const QString & _ext )
{
	int idx = 0;
	while( fileEncodeDevices[idx].m_fileType != NullFile )
	{
		if( QString( fileEncodeDevices[idx].m_extension ) == _ext )
		{
			return( fileEncodeDevices[idx].m_fileType );
		}
		++idx;
	}

	return( WaveFile );	// default
}




void exportProjectDialog::keyPressEvent( QKeyEvent * _ke )
{
	if( _ke->key() == Qt::Key_Escape )
	{
		if( engine::getSong()->exporting() == FALSE )
		{
			accept();
		}
		else
		{
			abortProjectExport();
		}
	}
}




void exportProjectDialog::closeEvent( QCloseEvent * _ce )
{
	if( engine::getSong()->exporting() == TRUE )
	{
		abortProjectExport();
		_ce->ignore();
	}
	else
	{
		QDialog::closeEvent( _ce );
	}
}




void exportProjectDialog::changedType( const QString & _new_type )
{
	int idx = 0;
	while( fileEncodeDevices[idx].m_fileType != NullFile )
	{
		if( tr( fileEncodeDevices[idx].m_description ) == _new_type )
		{
			m_fileType = fileEncodeDevices[idx].m_fileType;
			break;
		}
		++idx;
	}
}




void exportProjectDialog::exportBtnClicked( void )
{
	int idx = 0;
	while( fileEncodeDevices[idx].m_fileType != NullFile )
	{
		if( fileEncodeDevices[idx].m_fileType == m_fileType )
		{
			break;
		}
		++idx;
	}

	if( fileEncodeDevices[idx].m_fileType == NullFile )
	{
		return;
	}

	bool success_ful = FALSE;
	audioFileDevice * dev = fileEncodeDevices[idx].m_getDevInst(
							DEFAULT_SAMPLE_RATE,
							DEFAULT_CHANNELS,
							success_ful,
							m_fileName,
						m_vbrCb->model()->value(),
					m_kbpsModel->currentText().toInt(),
					m_kbpsModel->currentText().toInt() - 64,
					m_kbpsModel->currentText().toInt() + 64,
					engine::getMixer() );
	if( success_ful == FALSE )
	{
		QMessageBox::information( this,
					tr( "Export failed" ),
					tr( "The project-export failed, "
						"because the output-file/-"
						"device could not be opened.\n"
						"Make sure, you have write "
						"access to the selected "
						"file/device!" ),
							QMessageBox::Ok );
		return;
	}


	setWindowTitle( tr( "Exporting project to %1" ).arg(
					QFileInfo( m_fileName ).fileName() ) );

	delete m_typeLbl;
	delete m_typeCombo;
	delete m_kbpsLbl;
	delete m_kbpsCombo;
	delete m_vbrCb;
	delete m_exportBtn;

	m_exportProgressBar = new QProgressBar( this );
	m_exportProgressBar->setGeometry( EPB_X, EPB_Y, EPB_WIDTH, EPB_HEIGHT );
	m_exportProgressBar->setMaximum( 100 );
	m_exportProgressBar->show();
	
	m_hourglassLbl = new QLabel( this );
	m_hourglassLbl->setPixmap( embed::getIconPixmap( "hourglass" ) );
	m_hourglassLbl->setGeometry( HOURGLASS_X, HOURGLASS_Y,
					HOURGLASS_WIDTH, HOURGLASS_HEIGHT );
	m_hourglassLbl->show();

	m_cancelBtn->move( CANCEL_X_WHILE_EXPORT, CANCEL_Y_WHILE_EXPORT );



	engine::getMixer()->setAudioDevice( dev, m_hqmCb->model()->value() );
	engine::getSong()->startExport();

	delete m_hqmCb;

	song::playPos & pp = engine::getSong()->getPlayPos(
							song::Mode_PlaySong );

	while( engine::getSong()->exportDone() == FALSE &&
				engine::getSong()->exporting() == TRUE
							&& !m_deleteFile )
	{
		dev->processNextBuffer();
		int pval = pp * 100 /
			( ( engine::getSong()->lengthInTacts() + 1 ) * 64 );
		m_exportProgressBar->setValue( pval );
		// update lmms-main-win-caption
		engine::getMainWindow()->setWindowTitle( tr( "Rendering:" )
					+ " " + QString::number( pval ) + "%" );
		// process paint-events etc.
		qApp->processEvents();
	}

	finishProjectExport();
}




void exportProjectDialog::cancelBtnClicked( void )
{
	// is song-export-thread active?
	if( engine::getSong()->exporting() == TRUE )
	{
		// then dispose abort of export
		abortProjectExport();
		return;
	}
	reject();
}




// called whenever there's a reason for aborting song-export (like user-input)
void exportProjectDialog::abortProjectExport( void )
{
	m_deleteFile = TRUE;
}




void exportProjectDialog::finishProjectExport( void )
{
	engine::getMixer()->restoreAudioDevice();

	// if the user aborted export-process, the file has to be deleted
	if( m_deleteFile )
	{
		QFile( m_fileName ).remove();
	}

	// restore window-title
	engine::getMainWindow()->resetWindowTitle(); 

	engine::getSong()->stopExport();

	// if we rendered file from command line, quit after export
	if( file_to_render != "" )
	{
		// qApp->quit(); - doesn't work for some reason...
		exit( 0 );
	}

	// let's close us...
	accept();
}




#include "export_project_dialog.moc"


#endif
