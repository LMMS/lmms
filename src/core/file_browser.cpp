/*
 * file_browser.cpp - implementation of the project-, preset- and
 *                    sample-file-browser
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QPushButton>
#include <QKeyEvent>
#include <QMenu>

#else

#include <qpushbutton.h>
#include <qpopupmenu.h>

#endif


#include "file_browser.h"
#include "song_editor.h"
#include "bb_editor.h"
#include "embed.h"
#include "channel_track.h"
#include "mmp.h"
#include "preset_preview_play_handle.h"
#include "sample_play_handle.h"
#include "debug.h"
#include "gui_templates.h"
#include "instrument.h"



fileBrowser::fileBrowser( const QString & _path, const QString & _filter,
			const QString & _title, const QPixmap & _pm,
							QWidget * _parent ) :
	sideBarWidget( _title, _pm, _parent ),
	m_contextMenuItem( NULL ),
	m_path( _path ),
	m_filter( _filter ),
	m_previewPlayHandle( NULL )
{
	setWindowTitle( tr( "Browser" ) );
	m_l = new Q3ListView( contentParent() );
	addContentWidget( m_l );

#ifdef QT4
	connect( m_l, SIGNAL( mouseButtonPressed( int, Q3ListViewItem *,
							const QPoint &, int ) ),
			this, SLOT( itemPressed( int, Q3ListViewItem *,
						const QPoint &, int ) ) );
	connect( m_l, SIGNAL( mouseButtonClicked( int, Q3ListViewItem *,
							const QPoint &, int ) ),
			this, SLOT( itemReleased( int, Q3ListViewItem *,
						const QPoint &, int ) ) );
	connect( m_l, SIGNAL( doubleClicked( Q3ListViewItem *,
							const QPoint &, int ) ),
			this, SLOT( itemDoubleClicked( Q3ListViewItem *,
						const QPoint &, int ) ) );

	connect( m_l, SIGNAL( contextMenuRequested( Q3ListViewItem *,
							const QPoint &, int ) ),
			this, SLOT( contextMenuRequest( Q3ListViewItem *,
						const QPoint &, int ) ) );
#else
	connect( m_l, SIGNAL( mouseButtonPressed( int, QListViewItem *,
							const QPoint &, int ) ),
			this, SLOT( itemPressed( int, QListViewItem *,
						const QPoint &, int ) ) );
	connect( m_l, SIGNAL( mouseButtonClicked( int, QListViewItem *,
							const QPoint &, int ) ),
			this, SLOT( itemReleased( int, QListViewItem *,
						const QPoint &, int ) ) );
/*	connect( m_l, SIGNAL( pressed( QListViewItem * ) ),
			this, SLOT( itemClicked( QListViewItem * ) ) );*/
	connect( m_l, SIGNAL( doubleClicked( QListViewItem *,
							const QPoint &, int ) ),
			this, SLOT( itemDoubleClicked( QListViewItem *,
						const QPoint &, int ) ) );

	connect( m_l, SIGNAL( contextMenuRequested( QListViewItem *,
							const QPoint &, int ) ),
			this, SLOT( contextMenuRequest( QListViewItem *,
						const QPoint &, int ) ) );
#endif
	connect( m_l, SIGNAL( selectionChanged() ), this,
						SLOT( selectionChanged() ) );

	m_l->addColumn( tr( "Files" ) );
	m_l->setTreeStepSize( 12 );
	m_l->setDefaultRenameAction( Q3ListView::Accept );
	m_l->setSorting( -1 );
	//setColumnWidthMode (0, Manual);
	//setColumnWidth (0, 196);
	m_l->setShowToolTips( TRUE );
	//m_l->setGeometry (0, 0, 200, 600);

	m_l->setFont( pointSize<8>( m_l->font() ) );


	QPushButton * reload_btn = new QPushButton( embed::getIconPixmap(
			"reload" ), tr( "Reload (F5)" ), contentParent() );
	addContentWidget( reload_btn );
	connect( reload_btn, SIGNAL( clicked() ), this, SLOT( reloadTree() ) );

	reloadTree();
	show();
}




fileBrowser::~fileBrowser()
{
}




void fileBrowser::reloadTree( void )
{
	m_l->clear();
	QDir cdir( m_path );
	QStringList files = cdir.entryList( QDir::NoFilter, QDir::Name );

	// TODO: after dropping qt3-support we can use QStringList's iterator
	// which makes it possible to travel through the list in reverse
	// direction

	for( csize i = 0; i < files.size(); ++i )
	{
		QString cur_file = files[files.size() - i - 1];
		if( cur_file[0] != '.' &&
				!QFileInfo( m_path + "/" + cur_file ).isDir()
#ifdef QT4
// TBD
#else
			&& QDir::match( m_filter, cur_file.lower() )
#endif
	)
		{
			(void) new fileItem( m_l, cur_file, m_path );
		}
	}

	for( csize i = 0; i < files.size(); ++i )
	{
		QString cur_file = files[files.size() - i - 1];
		if( cur_file[0] != '.' &&
			QFileInfo( m_path + "/" + cur_file ).isDir() )
		{
			(void) new directory( m_l, cur_file, m_path, m_filter );
		}
	}
}




void fileBrowser::keyPressEvent( QKeyEvent * _ke )
{
	if( _ke->key() == Qt::Key_F5 )
	{
		reloadTree();
	}
	else
	{
		_ke->ignore();
	}
}




#ifdef QT4
void fileBrowser::itemPressed( int _btn, Q3ListViewItem * i, const QPoint &, int )
#else
void fileBrowser::itemPressed( int _btn, QListViewItem * i, const QPoint &, int )
#endif
{
	fileItem * f = dynamic_cast<fileItem *>( i );
	if( f != NULL && _btn == Qt::LeftButton )
	{
		if( m_previewPlayHandle != NULL )
		{
			mixer::inst()->removePlayHandle( m_previewPlayHandle );
			m_previewPlayHandle = NULL;
		}
		if( f->type() == fileItem::SAMPLE_FILE )
		{
			samplePlayHandle * s = new samplePlayHandle(
								f->fullName() );
			s->setDoneMayReturnTrue( FALSE );
			m_previewPlayHandle = s;
		}
		else if( f->type() == fileItem::PRESET_FILE )
		{
			m_previewPlayHandle = new presetPreviewPlayHandle(
								f->fullName() );
		}
		if( m_previewPlayHandle != NULL )
		{
			mixer::inst()->addPlayHandle( m_previewPlayHandle );
		}
	}
}




#ifdef QT4
void fileBrowser::itemReleased( int, Q3ListViewItem * i, const QPoint &, int )
#else
void fileBrowser::itemReleased( int, QListViewItem * i, const QPoint &, int )
#endif
{
	selectionChanged();
}




void fileBrowser::selectionChanged( void )
{
	if( m_previewPlayHandle != NULL )
	{
		// if there're samples shorter than 3 seconds, we don't
		// stop them if the user releases mouse-button...
		samplePlayHandle * s = dynamic_cast<samplePlayHandle *>(
							m_previewPlayHandle );
		if( s != NULL )
		{
			if( s->totalFrames() - s->framesDone() <=
				static_cast<Uint32>(
					mixer::inst()->sampleRate() * 3 ) )
			{
				s->setDoneMayReturnTrue( TRUE );
				m_previewPlayHandle = NULL;
				return;
			}
		}
		mixer::inst()->removePlayHandle( m_previewPlayHandle );
		m_previewPlayHandle = NULL;
	}
}




#ifdef QT4
void fileBrowser::itemDoubleClicked( Q3ListViewItem * i, const QPoint &, int )
#else
void fileBrowser::itemDoubleClicked( QListViewItem * i, const QPoint &, int )
#endif
{
	fileItem * f = dynamic_cast<fileItem *>( i );
	if( f != NULL )
	{
		if( f->type() == fileItem::SAMPLE_FILE )
		{
			// samples are per default opened in bb-editor because
			// they're likely drum-samples etc.
			channelTrack * ct = dynamic_cast<channelTrack *>(
						track::create(
							track::CHANNEL_TRACK,
							bbEditor::inst() ) );
#ifdef LMMS_DEBUG
			assert( ct != NULL );
#endif
			instrument * afp = ct->loadInstrument(
							"audiofileprocessor" );
			if( afp != NULL )
			{
				afp->setParameter( "audiofile", f->fullName() );
			}
			ct->toggledChannelButton( TRUE );
		}
		else if( f->type() == fileItem::PRESET_FILE )
		{
			// presets are per default opened in bb-editor
			multimediaProject mmp( f->fullName() );
			track * t = track::create( track::CHANNEL_TRACK,
						bbEditor::inst() );
			channelTrack * ct = dynamic_cast<channelTrack *>( t );
			if( ct != NULL )
			{
				ct->loadTrackSpecificSettings( mmp.content().
								firstChild().
								toElement() );
				ct->toggledChannelButton( TRUE );
			}
		}
		else if( f->type() == fileItem::SONG_FILE )
		{
			if( songEditor::inst()->mayChangeProject() == TRUE )
			{
				songEditor::inst()->loadProject(
								f->fullName() );
			}
		}
	}
}



#ifdef QT4
void fileBrowser::contextMenuRequest( Q3ListViewItem * i, const QPoint &, int )
#else
void fileBrowser::contextMenuRequest( QListViewItem * i, const QPoint &, int )
#endif
{
	fileItem * f = dynamic_cast<fileItem *>( i );
	if( f != NULL && ( f->type() == fileItem::SAMPLE_FILE ||
				f->type() == fileItem::PRESET_FILE ) )
	{
		m_contextMenuItem = f;
		QMenu * contextMenu = new QMenu( this );
		contextMenu->addAction( tr( "Send to active channel" ), this,
						SLOT( sendToActiveChannel() ) );
		contextMenu->addAction( tr( "Open in new channel/Song-Editor" ),
						this,
						SLOT( openInNewChannelSE() ) );
		contextMenu->addAction( tr( "Open in new channel/B+B Editor" ),
						this,
						SLOT( openInNewChannelBBE() ) );
		//contextMenu->addSeparator ();
		//contextMenu->addAction (tr("Rename"), this, SLOT(renameItem()));
		contextMenu->exec( QCursor::pos() );
		m_contextMenuItem = NULL;
		delete contextMenu;
	}

}




void fileBrowser::sendToActiveChannel( void )
{
	if( lmmsMainWin::inst()->workspace() != NULL )
	{
		// get all windows opened in the workspace
		QWidgetList pl = lmmsMainWin::inst()->workspace()->windowList(
#if QT_VERSION >= 0x030200
						QWorkspace::StackingOrder
#endif
									);
#ifdef QT4
		QListIterator<QWidget *> w( pl );
		w.toBack();
		// now we travel through the window-list until we find a
		// channel-track
		while( w.hasPrevious() )
		{
			channelTrack * ct = dynamic_cast<channelTrack *>(
								w.previous() );
#else
		QWidget * w = pl.last();
		// now we travel through the window-list until we find a
		// channel-track
		while( w != NULL )
		{
			channelTrack * ct = dynamic_cast<channelTrack *>( w );
#endif
			if( ct != NULL && ct->isHidden() == FALSE )
			{
				// ok, it's a channel-track, so we can apply the
				// sample or the preset
				if( m_contextMenuItem->type() ==
							fileItem::SAMPLE_FILE )
				{
					instrument * afp = ct->loadInstrument(
							"audiofileprocessor" );
					if( afp != NULL )
					{
						afp->setParameter( "audiofile",
						m_contextMenuItem->fullName() );
					}
				}
				else if( m_contextMenuItem->type() ==
							fileItem::PRESET_FILE )
				{
					multimediaProject mmp(
						m_contextMenuItem->fullName() );
					ct->loadTrackSpecificSettings(
								mmp.content().
								firstChild().
								toElement() );
				}
				ct->toggledChannelButton( TRUE );
				break;
			}
#ifndef QT4
			w = pl.prev();
#endif
		}
	}
}




void fileBrowser::openInNewChannel( trackContainer * _tc )
{
	if( m_contextMenuItem->type() == fileItem::SAMPLE_FILE )
	{
		channelTrack * ct = dynamic_cast<channelTrack *>(
			track::create( track::CHANNEL_TRACK, _tc ) );
#ifdef LMMS_DEBUG
		assert( ct != NULL );
#endif
		instrument * afp = ct->loadInstrument( "audiofileprocessor" );
		if( afp != NULL )
		{
			afp->setParameter( "audiofile",
						m_contextMenuItem->fullName() );
		}
		ct->toggledChannelButton( TRUE );
	}
	else if( m_contextMenuItem->type() == fileItem::PRESET_FILE )
	{
		multimediaProject mmp( m_contextMenuItem->fullName() );
		track * t = track::create( track::CHANNEL_TRACK, _tc );
		channelTrack * ct = dynamic_cast<channelTrack *>( t );
		if( ct != NULL )
		{
			ct->loadTrackSpecificSettings( mmp.content().
							firstChild().
							toElement() );
			ct->toggledChannelButton( TRUE );
		}
	}
}




void fileBrowser::openInNewChannelSE( void )
{
	openInNewChannel( songEditor::inst() );
}




void fileBrowser::openInNewChannelBBE( void )
{
	openInNewChannel( bbEditor::inst() );
}




void fileBrowser::renameItem( void )
{
	m_contextMenuItem->startRename( 0 );
}







QPixmap * directory::s_folderPixmap = NULL;
QPixmap * directory::s_folderOpenedPixmap = NULL;
QPixmap * directory::s_folderLockedPixmap = NULL;


directory::directory( directory * _parent, const QString & _name,
			const QString & _path, const QString & _filter ) :
	Q3ListViewItem( _parent, _name ),
	m_p( _parent ),
	m_pix( NULL ),
	m_path( _path ),
	m_filter( _filter )
{
	initPixmapStuff();
}




directory::directory( Q3ListView * _parent, const QString & _name,
			const QString & _path, const QString & _filter ) :
	Q3ListViewItem( _parent, _name ),
	m_p( NULL ),
	m_pix( NULL ),
	m_path( _path ),
	m_filter( _filter )
{
	initPixmapStuff();
}





void directory::initPixmapStuff( void )
{
	if( s_folderPixmap == NULL )
	{
		s_folderPixmap = new QPixmap(
					embed::getIconPixmap( "folder" ) );
	}
	if( s_folderOpenedPixmap == NULL )
	{
		s_folderOpenedPixmap = new QPixmap(
				embed::getIconPixmap( "folder_opened" ) );
	}
	if( s_folderLockedPixmap == NULL )
	{
		s_folderLockedPixmap = new QPixmap(
				embed::getIconPixmap( "folder_locked" ) );
	}

	if( !QDir( fullName() ).isReadable() )
	{
		setPixmap( s_folderLockedPixmap );
	}
	else
	{
		setPixmap( s_folderPixmap );
	}
}




void directory::setPixmap( QPixmap * _px )
{
	m_pix = _px;
	setup();
	widthChanged( 0 ); 
	invalidateHeight();
	repaint();
}




void directory::setOpen( bool _o )
{
	if( _o )
	{
		setPixmap( s_folderOpenedPixmap );
	}
	else
	{
		setPixmap( s_folderPixmap );
	}

	if( _o && !childCount() )
	{
		QString s( fullName() );
		QDir thisDir( s );
		if( !thisDir.isReadable() )
		{
			//readable = FALSE;
			setExpandable( FALSE );
			return;
		}

		listView()->setUpdatesEnabled( FALSE );

		QStringList files = thisDir.entryList( QDir::NoFilter,
								QDir::Name );
		for( csize i = 0; i < files.size(); ++i )
		{
			QString cur_file = files[files.size()-i-1];
#ifdef QT4
			if( cur_file[0] != '.' && !QFileInfo(
						thisDir.absolutePath() + "/" +
							cur_file ).isDir() &&
				thisDir.match( m_filter, cur_file.toLower() )
				/*QDir::match( FILE_FILTER, cur_file )*/ )
#else
			if( cur_file[0] != '.' && !QFileInfo(
						thisDir.absPath() + "/" +
							cur_file ).isDir() &&
				thisDir.match( m_filter, cur_file.lower() )
				/*QDir::match( FILE_FILTER, cur_file )*/ )
#endif
			{
				(void) new fileItem( this, cur_file, s );
			}
		}
		for( csize i = 0; i < files.size(); ++i )
		{
			QString cur_file = files[files.size()-i-1];
#ifdef QT4
			if( cur_file[0] != '.' && QFileInfo(
						thisDir.absolutePath() + "/" +
							cur_file ).isDir() )
#else
			if( cur_file[0] != '.' && QFileInfo(
						thisDir.absPath() + "/" +
							cur_file ).isDir() )
#endif
			{
				(void) new directory( this, cur_file, s,
								m_filter );
			}
		}
		listView()->setUpdatesEnabled( TRUE );
	}
	Q3ListViewItem::setOpen( _o );
}




void directory::setup( void )
{
	setExpandable( TRUE );
	Q3ListViewItem::setup();
}







QPixmap * fileItem::s_songFilePixmap = NULL;
QPixmap * fileItem::s_presetFilePixmap = NULL;
QPixmap * fileItem::s_sampleFilePixmap = NULL;
QPixmap * fileItem::s_unknownFilePixmap = NULL;


fileItem::fileItem( Q3ListView * _parent, const QString & _name,
						const QString & _path ) :
	Q3ListViewItem( _parent, _name ),
	m_path( _path )
{
	determineFileType();
	initPixmapStuff();
	setDragEnabled( TRUE );
}




fileItem::fileItem( Q3ListViewItem * _parent, const QString & _name,
						const QString & _path ) :
	Q3ListViewItem( _parent, _name ),
	m_path( _path )
{
	determineFileType();
	initPixmapStuff();
	setDragEnabled( TRUE );
}




void fileItem::initPixmapStuff( void )
{
	if( s_songFilePixmap == NULL )
	{
		s_songFilePixmap = new QPixmap( embed::getIconPixmap(
						"project_file", 16, 16 ) );
	}
	if( s_presetFilePixmap == NULL )
	{
		s_presetFilePixmap = new QPixmap( embed::getIconPixmap(
						"preset_file", 16, 16 ) );
	}
	if( s_sampleFilePixmap == NULL )
	{
		s_sampleFilePixmap = new QPixmap( embed::getIconPixmap(
						"sound_file", 16, 16 ) );
	}
	if( s_unknownFilePixmap == NULL )
	{
		s_unknownFilePixmap = new QPixmap( embed::getIconPixmap(
						"unknown_file" ) );
	}

	switch( m_type )
	{
		case SONG_FILE: m_pix = s_songFilePixmap; break;
		case PRESET_FILE: m_pix = s_presetFilePixmap; break;
		case SAMPLE_FILE: m_pix = s_sampleFilePixmap; break;
		case UNKNOWN: m_pix = s_unknownFilePixmap; break;
	}
}




void fileItem::determineFileType( void )
{
#ifdef QT4
	QString ext = QFileInfo( fullName() ).suffix().toLower();
#else
	QString ext = QFileInfo( fullName() ).extension( FALSE ).toLower();
#endif
	if( ext == "mmp" || ext == "mpt" )
	{
		m_type = SONG_FILE;
	}
	else if( ext == "xml" )
	{
		multimediaProject::projectTypes t =
				multimediaProject::typeOfFile( fullName() );
		if( t == multimediaProject::SONG_PROJECT )
		{
			m_type = SONG_FILE;
		}
		else if( t == multimediaProject::CHANNEL_SETTINGS )
		{
			m_type = PRESET_FILE;
		}
		else
		{
			m_type = UNKNOWN;
		}
	}
	else if( ext == "csf" )
	{
		m_type = PRESET_FILE;
	}
	else if( ext == "wav" || ext == "ogg" || ext == "mp3" ||
			ext == "aiff" || ext == "aif" || ext == "voc" ||
			ext == "au" || ext == "raw" )
	{
		m_type = SAMPLE_FILE;
	}
	else
	{
		m_type = UNKNOWN;
	}
}





#include "file_browser.moc"

