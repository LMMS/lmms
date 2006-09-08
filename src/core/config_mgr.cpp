#ifndef SINGLE_SOURCE_COMPILE

/*
 * config_mgr.cpp - implementation of class configManager
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtGui/QMessageBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QFileDialog>
#include <QtGui/QRadioButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QApplication>

#else

#include <qdir.h>
#include <qdom.h>
#include <qfile.h>
#include <qmessagebox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qfiledialog.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qapplication.h>

#define absolutePath absPath
#define addButton insert

#ifndef __USE_XOPEN_EXTENDED
#define __USE_XOPEN_EXTENDED
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#endif


#include "config_mgr.h"
#include "embed.h"
#include "gui_templates.h"



void mkPath( const QString & _path )
{
#ifdef QT4
	// simple clean solution with Qt4...
	QDir().mkpath( _path );
#else
	// ...but Qt3 needs additional code...
	QDir d( _path );
	vlist<QString> dirs;
	dirs.push_front( _path );
	QString dir = _path;
	while( !QDir( dir ).exists() )
	{
		dir = dir.section( '/', 0, -2 );
		dirs.push_front( dir );
	}
	while( !dirs.isEmpty() )
	{
		d.mkdir( dirs.front() );
		d.setPath( dirs.front() );
		dirs.erase( dirs.begin() );
	}
#endif
}



/*
void linkFile( const QString & _src, const QString & _dst )
{
#ifdef QT4
	// simple clean solution with Qt4...
	QFile::link( _src, _dst );
#else
	// ...but Qt3 needs additional (unportable) code...
	symlink( _src.ascii(), _dst.ascii() );
#endif
}




void copyFile( const QString & _src, const QString & _dst )
{
#ifdef QT4
	// simple clean solution with Qt4...
	QFile::copy( _src, _dst );
#else
	// ...but Qt3 needs additional code...
	QFile in( _src );
	QFile out( _dst );
	in.open( IO_ReadOnly );
	out.open( IO_WriteOnly );
	char buffer[1024];
	while( !in.atEnd() )
	{
		Q_LONG read = in.readBlock( buffer, 1024 );
		if( read == -1 )
		{
			break;
		}
		if( out.writeBlock( buffer, read ) == -1 )
		{
			break;
		}
	}
#endif
}
*/



configManager * configManager::s_instanceOfMe = NULL;


configManager::configManager( void ) :
	QDialog(),
	m_lmmsRcFile( QDir::home().absolutePath() + "/.lmmsrc.xml" ),
	m_workingDir( QDir::home().absolutePath() + "/lmms" ),
#if QT_VERSION >= 0x030200
	m_dataDir( qApp->applicationDirPath().section( '/', 0, -2 ) +
							"/share/lmms/" ),
#else
	// hardcode since qt < 3.2 doesn't know something like
	// applicationDirPath and implementing own function would be senseless
	// since real support for qt < 3.2 is senseless too ;-)
	m_dataDir( "/usr/share/lmms/" ),
#endif
	m_artworkDir( defaultArtworkDir() ),
#if QT_VERSION >= 0x030200
	m_pluginDir( qApp->applicationDirPath().section( '/', 0, -2 ) +
							"/lib/lmms/" ),
#else
	// hardcode since qt < 3.2 doesn't know something like
	// applicationDirPath and implementing own function would be senseless
	// since real support for qt < 3.2 is senseless too ;-)
	m_pluginDir( "/usr/lib/lmms" ),
#endif
	m_vstDir( QDir::home().absolutePath() ),
	m_flDir( QDir::home().absolutePath() ),
	m_currentPage( 0 )
{
}




configManager::~configManager()
{
	saveConfigFile();
}




void configManager::createWidgets( void )
{
	m_mainLayout = new QHBoxLayout( this );
	m_mainLayout->setMargin( 0 );
	m_mainLayout->setSpacing( 10 );
	m_mainLayout->addSpacing( 8 );
#ifdef QT4
	setLayout( m_mainLayout );
#endif

	m_contentWidget = new QWidget( this );
	m_contentLayout = new QVBoxLayout( m_contentWidget );
	m_contentLayout->setMargin( 0 );
	m_contentLayout->setSpacing( 10 );
	m_contentLayout->addSpacing( 8 );
#ifdef QT4
	m_contentWidget->setLayout( m_contentLayout );
#endif
	m_mainLayout->addWidget( m_contentWidget );
	m_mainLayout->addSpacing( 8 );

	// wizard-init
	m_hbar = new QFrame( m_contentWidget );
	m_hbar->setFrameStyle( QFrame::Sunken + QFrame::HLine );
	m_hbar->setFixedHeight( 4 );
	m_title = new QLabel( m_contentWidget );
	m_title->setFixedHeight( 16 );
	QFont f = m_title->font();
	f.setBold( TRUE );
	m_title->setFont( pointSize<12>( f ) );


	QWidget * button_widget = new QWidget( m_contentWidget );
	button_widget->setFixedHeight( 40 );
	m_buttonLayout = new QHBoxLayout( button_widget );
	m_buttonLayout->setMargin( 0 );
	m_buttonLayout->setSpacing( 0 );
	m_buttonLayout->addStretch( 1 );
#ifdef QT4
	button_widget->setLayout( m_buttonLayout );
#endif

	m_cancelButton = new QPushButton( tr( "&Cancel" ), button_widget );
	m_backButton = new QPushButton( tr( "< &Back" ), button_widget );
	m_nextButton = new QPushButton( tr( "&Next >" ), button_widget );
	m_finishButton = new QPushButton( tr( "&Finish" ), button_widget );

	connect( m_cancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( m_backButton, SIGNAL( clicked() ), this,
						SLOT( backButtonClicked() ) );
	connect( m_nextButton, SIGNAL( clicked() ), this,
						SLOT( nextButtonClicked() ) );
	connect( m_finishButton, SIGNAL( clicked() ), this,
						SLOT( accept() ) );
	m_buttonLayout->addWidget( m_cancelButton );
	m_buttonLayout->addWidget( m_backButton );
	m_buttonLayout->addWidget( m_nextButton );
	m_buttonLayout->addWidget( m_finishButton );
	m_buttonLayout->addSpacing( 15 );

	m_contentLayout->addWidget( m_title );
	m_contentLayout->addWidget( m_hbar );
	m_contentLayout->addWidget( button_widget );
	m_contentLayout->addSpacing( 8 );

	// wizard-setup
	setWindowTitle( tr( "Setup LMMS" ) );
	setWindowIcon( embed::getIconPixmap( "wizard" ) );

	m_pageIntro = new QWidget( m_contentWidget );
	QHBoxLayout * intro_layout = new QHBoxLayout( m_pageIntro );
	intro_layout->setMargin( 0 );
	intro_layout->setSpacing( 15 );
#ifdef QT4
	m_pageIntro->setLayout( intro_layout );
#endif

	QLabel * intro_logo_lbl = new QLabel( m_pageIntro );
	intro_logo_lbl->setPixmap( embed::getIconPixmap( "wizard_intro" ) );
	intro_logo_lbl->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	intro_logo_lbl->setFixedSize( 240, 300 );

	QLabel * intro_txt_lbl = new QLabel( tr( "LMMS needs to be setup in "
						"order to run properly. "
						"This wizard will help you to "
						"setup your personal LMMS-"
						"installation.\n\n"
						"If you're unsure what to do "
						"at a step, just click on "
						"'Next'. LMMS will "
						"automatically select the best "
						"options for you.\n\n\n"
						"Now click on 'Next' to get to "
						"the next page." ),
								m_pageIntro );
#ifdef QT4
	intro_txt_lbl->setWordWrap( TRUE );
#else
	intro_txt_lbl->setAlignment( intro_txt_lbl->alignment() | WordBreak );
#endif

	intro_layout->addWidget( intro_logo_lbl );
	intro_layout->addWidget( intro_txt_lbl );


	m_pageWorkingDir = new QWidget( m_contentWidget );
	QHBoxLayout * workingdir_layout = new QHBoxLayout( m_pageWorkingDir );
	workingdir_layout->setMargin( 0 );
	workingdir_layout->setSpacing( 15 );

	QLabel * workingdir_logo_lbl = new QLabel( m_pageWorkingDir );
	workingdir_logo_lbl->setPixmap( embed::getIconPixmap(
							"wizard_workingdir" ) );
	workingdir_logo_lbl->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	workingdir_logo_lbl->setFixedSize( 240, 300 );

	QWidget * workingdir_content = new QWidget( m_pageWorkingDir );
	QVBoxLayout * workingdir_content_layout = new QVBoxLayout(
							workingdir_content );
	workingdir_content_layout->setMargin( 0 );
	workingdir_content_layout->setSpacing( 0 );

	QLabel * workingdir_txt_lbl = new QLabel(
				tr( "When working with LMMS there needs to "
					"be a working-directory.\nThis "
					"directory is used for storing your "
					"projects, presets, samples etc.\n\n\n"
					"Please select a directory:" ),
					workingdir_content );
#ifdef QT4
	workingdir_txt_lbl->setWordWrap( TRUE );
#else
	workingdir_txt_lbl->setAlignment( workingdir_txt_lbl->alignment() |
								WordBreak );
#endif

	QWidget * workingdir_input_fields = new QWidget( workingdir_content );
	QHBoxLayout * workingdir_input_fields_layout = new QHBoxLayout(
						workingdir_input_fields );
	workingdir_input_fields_layout->setSpacing( 10 );
	workingdir_input_fields_layout->setMargin( 0 );

	m_wdLineEdit = new QLineEdit( m_workingDir, workingdir_input_fields );
	connect( m_wdLineEdit, SIGNAL( textChanged( const QString & ) ), this,
				SLOT( setWorkingDir( const QString & ) ) );

	QPushButton * workingdir_select_btn = new QPushButton(
				embed::getIconPixmap( "project_open" ), "",
						workingdir_input_fields );
	workingdir_select_btn->setFixedSize( 24, 24 );
	connect( workingdir_select_btn, SIGNAL( clicked() ), this,
						SLOT( openWorkingDir() ) );

	workingdir_input_fields_layout->addWidget( m_wdLineEdit );
	workingdir_input_fields_layout->addWidget( workingdir_select_btn );

	workingdir_content_layout->addWidget( workingdir_txt_lbl );
	workingdir_content_layout->addWidget( workingdir_input_fields );

	workingdir_layout->addWidget( workingdir_logo_lbl );
	workingdir_layout->addWidget( workingdir_content );


/*
	// page for files-management
	m_pageFiles = new QWidget( m_contentWidget );
	QHBoxLayout * files_layout = new QHBoxLayout( m_pageFiles );
	files_layout->setSpacing( 15 );
	files_layout->setMargin( 0 );

	QWidget * files_content = new QWidget( m_pageFiles );
	QVBoxLayout * files_content_layout = new QVBoxLayout( files_content );
	files_content_layout->setSpacing( 10 );
	files_content_layout->setMargin( 0 );
#ifdef QT4
	files_content->setLayout( files_content_layout );
#endif

	QLabel * files_logo_lbl = new QLabel( m_pageFiles );
	files_logo_lbl->setPixmap( embed::getIconPixmap( "wizard_files" ) );
	files_logo_lbl->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	files_logo_lbl->setFixedSize( 240, 300 );

	QLabel * files_txt_lbl = new QLabel(
			tr( "For using the ready presets and samples of "
					"LMMS and enjoying the demo-songs the "
					"according files have to be copied or "
					"linked into your LMMS-working-"
					"directory.\nWhen copying files, you "
					"can modify them, but they need "
					"additional space in your working-"
					"directory. If you link files, you "
					"cannot modify them, but they need "
					"no extra space. So it's recommended "
					"to copy presets and demo-projects "
					"and link samples, which are bigger "
					"in size.\n" ), files_content );
#ifdef QT4
	files_txt_lbl->setWordWrap( TRUE );
#else
	files_txt_lbl->setAlignment( files_txt_lbl->alignment() | WordBreak );
#endif


	QWidget * samples_widget = new QWidget( files_content );
	QHBoxLayout * samples_layout = new QHBoxLayout( samples_widget );
	samples_layout->setSpacing( 5 );
	samples_layout->setMargin( 0 );

	QLabel * samples_pic_lbl = new QLabel( samples_widget );
	samples_pic_lbl->setPixmap( embed::getIconPixmap( "sound_file" ) );
	QLabel * samples_txt_lbl = new QLabel( tr( "samples:" ),
							samples_widget );
	samples_txt_lbl->setFixedWidth( 144 );

	QButtonGroup * samples_bg = new QButtonGroup( samples_widget );
#ifndef QT4
	samples_bg->hide();
#endif
	m_samplesCopyRB = new QRadioButton( tr( "copy" ), samples_widget );
	QRadioButton * samples_link_rb = new QRadioButton( tr( "link" ),
							samples_widget );
	samples_link_rb->setChecked( TRUE );
	samples_bg->addButton( m_samplesCopyRB );
	samples_bg->addButton( samples_link_rb );

	samples_layout->addWidget( samples_pic_lbl );
	samples_layout->addWidget( samples_txt_lbl );
	samples_layout->addWidget( m_samplesCopyRB );
	samples_layout->addSpacing( 15 );
	samples_layout->addWidget( samples_link_rb );
	samples_layout->addStretch( 400 );



	QWidget * presets_widget = new QWidget( files_content );
	QHBoxLayout * presets_layout = new QHBoxLayout( presets_widget );
	presets_layout->setSpacing( 5 );
	presets_layout->setMargin( 0 );

	QLabel * presets_pic_lbl = new QLabel( presets_widget );
	presets_pic_lbl->setPixmap( embed::getIconPixmap( "preset_file" ) );
	QLabel * presets_txt_lbl = new QLabel( tr( "presets:" ),
							presets_widget );
	presets_txt_lbl->setFixedWidth( 144 );

	QButtonGroup * presets_bg = new QButtonGroup( presets_widget );
#ifndef QT4
	presets_bg->hide();
#endif
	m_presetsCopyRB = new QRadioButton( tr( "copy" ), presets_widget );
	m_presetsCopyRB->setChecked( TRUE );
	QRadioButton * presets_link_rb = new QRadioButton( tr( "link" ),
							presets_widget );
	presets_bg->addButton( m_presetsCopyRB );
	presets_bg->addButton( presets_link_rb );

	presets_layout->addWidget( presets_pic_lbl );
	presets_layout->addWidget( presets_txt_lbl );
	presets_layout->addWidget( m_presetsCopyRB );
	presets_layout->addSpacing( 15 );
	presets_layout->addWidget( presets_link_rb );
	presets_layout->addStretch( 400 );


	
	QWidget * projects_widget = new QWidget( files_content );
	QHBoxLayout * projects_layout = new QHBoxLayout( projects_widget );
	projects_layout->setSpacing( 5 );
	projects_layout->setMargin( 0 );

	QLabel * projects_pic_lbl = new QLabel( projects_widget );
	projects_pic_lbl->setPixmap( embed::getIconPixmap( "project_file" ) );
	QLabel * projects_txt_lbl = new QLabel( tr( "demo projects:" ),
							projects_widget );
	projects_txt_lbl->setFixedWidth( 144 );

	QButtonGroup * projects_bg = new QButtonGroup( projects_widget );
#ifndef QT4
	projects_bg->hide();
#endif
	m_projectsCopyRB = new QRadioButton( tr( "copy" ), projects_widget );
	m_projectsCopyRB->setChecked( TRUE );
	QRadioButton * projects_link_rb = new QRadioButton( tr( "link" ),
							projects_widget );
	projects_bg->addButton( m_projectsCopyRB );
	projects_bg->addButton( projects_link_rb );

	projects_layout->addWidget( projects_pic_lbl );
	projects_layout->addWidget( projects_txt_lbl );
	projects_layout->addWidget( m_projectsCopyRB );
	projects_layout->addSpacing( 15 );
	projects_layout->addWidget( projects_link_rb );
	projects_layout->addStretch( 400 );




	files_content_layout->addWidget( files_txt_lbl );
	files_content_layout->addWidget( samples_widget );
	files_content_layout->addWidget( presets_widget );
	files_content_layout->addWidget( projects_widget );


	files_layout->addWidget( files_logo_lbl );
	files_layout->addWidget( files_content );
*/


	addPage( m_pageIntro, tr( "Welcome to LMMS" ) );
	addPage( m_pageWorkingDir, tr( "Select working directory" ) );
	//addPage( m_pageFiles, tr( "Copy or link files" ) );
	switchPage( static_cast<csize>( 0 ) );
}




void configManager::openWorkingDir( void )
{
#ifdef QT4
	QString new_dir = QFileDialog::getExistingDirectory( this,
					tr( "Choose LMMS working directory" ),
								m_workingDir );
#else
	QString new_dir = QFileDialog::getExistingDirectory( m_workingDir, 0, 0,
					tr( "Choose LMMS working directory" ),
									TRUE );
#endif
	if( new_dir != QString::null )
	{
		m_wdLineEdit->setText( new_dir );
	}
}




void configManager::setWorkingDir( const QString & _wd )
{
	m_workingDir = _wd;
}




void configManager::setVSTDir( const QString & _vd )
{
	m_vstDir = _vd;
}




void configManager::setArtworkDir( const QString & _ad )
{
	m_artworkDir = _ad;
}




void configManager::setFLDir( const QString & _fd )
{
	m_flDir = _fd;
}




void configManager::setLADSPADir( const QString & _fd )
{
#ifdef LADSPA_SUPPORT
	m_ladDir = _fd;
#endif
}




void configManager::setSTKDir( const QString & _fd )
{
#ifdef HAVE_STK_H
	m_stkDir = _fd;
#endif
}




void configManager::accept( void )
{
	if( m_workingDir.right( 1 ) != "/" )
	{
		m_workingDir += '/';
	}
	if( !QDir( m_workingDir ).exists() )
	{
		if( QMessageBox::
#if QT_VERSION >= 0x030200
				question
#else
				information
#endif
				( 0, tr( "Directory not existing" ),
						tr( "The directory you "
							"specified does not "
							"exist. Create it?" ),
					QMessageBox::Yes, QMessageBox::No )
			== QMessageBox::Yes )
		{
			mkPath( m_workingDir );
		}
		else
		{
			switchPage( m_pageWorkingDir );
			return;
		}
	}

	mkPath( userProjectsDir() );
	mkPath( userSamplesDir() );
	mkPath( userPresetsDir() );
/*	processFilesRecursively( m_dataDir + "samples/", m_workingDir +
								"samples/",
					m_samplesCopyRB->isChecked() ?
								&copyFile :
								&linkFile );
	processFilesRecursively( m_dataDir + "presets/", m_workingDir +
								"presets/",
					m_presetsCopyRB->isChecked() ?
								&copyFile :
								&linkFile );
	processFilesRecursively( m_dataDir + "projects/", m_workingDir +
								"projects/",
					m_projectsCopyRB->isChecked() ?
								&copyFile :
								&linkFile );*/
	saveConfigFile();

	QDialog::accept();
}




void configManager::backButtonClicked( void )
{
	switchPage( m_currentPage-1 );
}




void configManager::nextButtonClicked( void )
{
	switchPage( m_currentPage+1 );
}




void configManager::switchPage( csize _pg )
{
	if( m_currentPage >= 0 && m_currentPage < m_pages.size() )
	{
		m_pages[m_currentPage].first->hide();
#ifdef QT4
		m_contentLayout->removeWidget( m_pages[m_currentPage].first );
#else
#if QT_VERSION >= 0x030100
		m_contentLayout->remove( m_pages[m_currentPage].first );
#else
		m_pages[m_currentPage].first->hide();
#endif
#endif
	}
	if( _pg < m_pages.size() )
	{
		QWidget * p = m_pages[_pg].first;
		m_contentLayout->insertWidget( 2, p );
		p->show();
		p->setFocus();
		m_title->setText( m_pages[_pg].second );
		m_currentPage = _pg;
	}
	m_backButton->setEnabled( _pg > 0 );
	if( _pg == m_pages.size() - 1 )
	{
		m_nextButton->setEnabled( FALSE );
		m_finishButton->setEnabled( TRUE );
		m_finishButton->setDefault( TRUE );
	}
	else
	{
		m_nextButton->setEnabled( TRUE );
		m_nextButton->setDefault( TRUE );
		m_finishButton->setEnabled( FALSE );
	}
}




void configManager::switchPage( QWidget * _pg )
{
	for( csize i = 0; i < m_pages.size(); ++i )
	{
		if( m_pages[i].first == _pg )
		{
			switchPage( i );
			break;
		}
	}
}




void configManager::addPage( QWidget * _w, const QString & _title )
{
	_w->hide();
	m_pages.push_back( qMakePair( _w, _title ) );
}




const QString & configManager::value( const QString & _class,
					const QString & _attribute ) const
{
	if( m_settings.contains( _class ) )
	{
		for( stringPairVector::const_iterator it =
						m_settings[_class].begin();
					it != m_settings[_class].end(); ++it )
		{
			if( ( *it ).first == _attribute )
			{
				return( ( *it ).second );
			}
		}
	}
	static QString empty;
	return( empty );
}




void configManager::setValue( const QString & _class,
				const QString & _attribute,
				const QString & _value )
{
	if( m_settings.contains( _class ) )
	{
		for( stringPairVector::iterator it = m_settings[_class].begin();
					it != m_settings[_class].end(); ++it )
		{
			if( ( *it ).first == _attribute )
			{
				( *it ).second = _value;
				return;
			}
		}
	}
	// not in map yet, so we have to add it...
	m_settings[_class].push_back( qMakePair( _attribute, _value ) );
}




bool configManager::loadConfigFile( void )
{
	createWidgets();

	// read the XML file and create DOM tree
	QFile cfg_file( m_lmmsRcFile );
#ifdef QT4
	if( !cfg_file.open( QIODevice::ReadOnly ) )
#else
	if( !cfg_file.open( IO_ReadOnly ) )
#endif
	{
#ifdef QT4
		if( !( exec() && cfg_file.open( QIODevice::ReadOnly ) ) )
#else
		if( !( exec() && cfg_file.open( IO_ReadOnly ) ) )
#endif
		{
			return( FALSE );
		}
	}
	QDomDocument dom_tree;
	if( !dom_tree.setContent( &cfg_file ) )
	{
		QMessageBox::critical( 0, tr( "Error in configuration-file" ),
					tr( "Error while parsing "
						"configuration-file %1.\n"
						"The setup-wizard will be "
						"shown for reconfiguring LMMS."
						).arg( m_lmmsRcFile ) );
		cfg_file.close();
		if( exec() )
		{
			return( loadConfigFile() );
		}
		else
		{
			return( FALSE );
		}
	}
	cfg_file.close();


	// get the head information from the DOM
	QDomElement root = dom_tree.documentElement();

	QDomNode node = root.firstChild();

	// create the settings-map out of the DOM
	while( !node.isNull() )
	{
		if( node.isElement() && node.toElement().hasAttributes () )
		{
			stringPairVector attr;
			QDomNamedNodeMap node_attr =
						node.toElement().attributes();
			for( csize i = 0; i < node_attr.count(); ++i )
			{
				QDomNode n = node_attr.item( i );
				if( n.isAttr() )
				{
					attr.push_back( qMakePair(
							n.toAttr().name(),
							n.toAttr().value() ) );
				}
			}
			m_settings[node.nodeName()] = attr;
		}
		node = node.nextSibling();
	}

	if( value( "paths", "artwork" ) != "" )
	{
		m_artworkDir = value( "paths", "artwork" );
		if( QDir( m_artworkDir ).exists() == FALSE )
		{
			m_artworkDir = defaultArtworkDir();
		}
		if( m_artworkDir.right( 1 ) != "/" )
		{
			m_artworkDir += "/";
		}
	}
	m_workingDir = value( "paths", "workingdir" );
	m_vstDir = value( "paths", "vstdir" );
	m_flDir = value( "paths", "fldir" );
#ifdef LADSPA_SUPPORT
	m_ladDir = value( "paths", "laddir" );
#endif
#ifdef HAVE_STK_H
	m_stkDir = value( "paths", "stkdir" );
#endif

	if( m_vstDir == "" )
	{
		m_vstDir = QDir::home().absolutePath();
	}

	if( m_flDir == "" )
	{
		m_flDir = QDir::home().absolutePath();
	}

#ifdef LADSPA_SUPPORT
	if( m_ladDir == "" )
{
	m_ladDir = "/usr/lib/ladspa/:/usr/local/lib/ladspa/";
}
#endif

#ifdef HAVE_STK_H
	if( m_stkDir == "" )
{
	m_stkDir = "/usr/share/stk/rawwaves/";
}
#endif

	if( root.isElement() )
	{
		QString cfg_file_ver = root.toElement().attribute( "version" );
		if( ( cfg_file_ver.length() == 0 || cfg_file_ver != VERSION ) &&
				value( "app", "nowizard" ).toInt() == FALSE &&
			QMessageBox::
#if QT_VERSION >= 0x030200
				question
#else
				information
#endif
				( 0, tr( "Version mismatches" ),
					tr( "Accordingly to the information in "
						"your LMMS-configuration-file "
						"you seem\nto have run a "
						"different (probably older) "
						"version of LMMS before.\n"
						"It is recommended to run the "
						"setup-wizard again to ensure "
						"that\nthe latest samples, "
						"presets, demo-projects etc. "
						"are installed in your\n"
						"LMMS-working-directory. "
						"Run the setup-wizard now?" ),
					QMessageBox::Yes, QMessageBox::No )
			== QMessageBox::Yes )
		{
			if( exec() )
			{
				return( loadConfigFile() );
			}
		}
	}

	return( TRUE );
}




void configManager::saveConfigFile( void )
{
	setValue( "paths", "artwork", m_artworkDir );
	setValue( "paths", "workingdir", m_workingDir );
	setValue( "paths", "vstdir", m_vstDir );
	setValue( "paths", "fldir", m_flDir );
#ifdef LADSPA_SUPPORT
	setValue( "paths", "laddir", m_ladDir );
#endif
#ifdef HAVE_STK_H
	setValue( "paths", "stkdir", m_stkDir );
#endif

	QDomDocument doc( "lmms-config-file" );

	QDomElement lmms_config = doc.createElement( "lmms" );
	lmms_config.setAttribute( "version", VERSION );
	doc.appendChild( lmms_config );

	for( settingsMap::iterator it = m_settings.begin();
						it != m_settings.end(); ++it )
	{
		QDomElement n = doc.createElement( it.key() );
		for( stringPairVector::iterator it2 = ( *it ).begin();
						it2 != ( *it ).end(); ++it2 )
		{
			n.setAttribute( ( *it2 ).first, ( *it2 ).second );
		}
		lmms_config.appendChild( n );
	}

#if QT_VERSION >= 0x030100
	QString xml = "<?xml version=\"1.0\"?>\n" + doc.toString( 2 );
#else
	QString xml = "<?xml version=\"1.0\"?>\n" + doc.toString();
#endif

	QFile outfile( m_lmmsRcFile );
#ifdef QT4
	if( !outfile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
#else
	if( !outfile.open( IO_WriteOnly | IO_Truncate ) )
#endif
	{
		QMessageBox::critical( NULL, tr( "Could not save config-file" ),
					tr( "Could not save configuration "
						"file %1. You probably are not "
						"permitted to write to this "
						"file.\n"
						"Please make sure you have "
						"write-access to the file and "
						"try again." ).arg(
							m_lmmsRcFile  ) );
		return;
	}

#ifdef QT4
	outfile.write( xml.toUtf8().constData(), xml.length() );
#else
	QCString xml_utf8 = xml.utf8();
	outfile.writeBlock( xml_utf8.data(), xml_utf8.length() );
#endif
	outfile.close();
}



/*
void configManager::processFilesRecursively( const QString & _src_dir,
						const QString & _dst_dir,
	void( * _proc_func )( const QString & _src, const QString & _dst ) )
{
	mkPath( _dst_dir );
	QStringList files = QDir( _src_dir ).entryList();
	for( QStringList::iterator it = files.begin(); it != files.end(); ++it )
	{
		if( ( *it )[0] == '.' )
		{
			continue;
		}
		if( QFileInfo( _src_dir + *it ).isFile() )
		{
			_proc_func( _src_dir + *it, _dst_dir + *it );
		}
		else if( QFileInfo( _src_dir + *it ).isDir () )
		{
			processFilesRecursively( _src_dir + *it + "/",
							_dst_dir + *it + "/",
								_proc_func );
		}
	}
}
*/



#include "config_mgr.moc"
#undef absolutePath
#undef addButton

#endif
