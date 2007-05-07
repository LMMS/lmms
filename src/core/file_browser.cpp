#ifndef SINGLE_SOURCE_COMPILE

/*
 * file_browser.cpp - implementation of the project-, preset- and
 *                    sample-file-browser
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


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QPushButton>
#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QCursor>
#include <Qt3Support/Q3Header>

#else

#include <qpushbutton.h>
#include <qpopupmenu.h>
#include <qheader.h>
#include <qcursor.h>
#include <qworkspace.h>

#endif


#include "file_browser.h"
#include "bb_editor.h"
#include "config_mgr.h"
#include "debug.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "instrument.h"
#include "instrument_track.h"
#include "main_window.h"
#include "mmp.h"
#include "preset_preview_play_handle.h"
#include "sample_play_handle.h"
#include "song_editor.h"
#include "string_pair_drag.h"
#include "text_float.h"



fileBrowser::fileBrowser( const QString & _directories, const QString & _filter,
			const QString & _title, const QPixmap & _pm,
							QWidget * _parent ) :
	sideBarWidget( _title, _pm, _parent ),
	m_contextMenuItem( NULL ),
	m_directories( _directories ),
	m_filter( _filter )
{
	setWindowTitle( tr( "Browser" ) );
	m_l = new listView( contentParent() );
	addContentWidget( m_l );

#ifdef QT4
	connect( m_l, SIGNAL( contextMenuRequested( Q3ListViewItem *,
							const QPoint &, int ) ),
			this, SLOT( contextMenuRequest( Q3ListViewItem *,
						const QPoint &, int ) ) );
#else
	connect( m_l, SIGNAL( contextMenuRequested( QListViewItem *,
							const QPoint &, int ) ),
			this, SLOT( contextMenuRequest( QListViewItem *,
						const QPoint &, int ) ) );
#endif


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
#ifndef QT3
	QStringList paths = m_directories.split( '*' );
#else
	QStringList paths = QStringList::split( '*', m_directories );
#endif
	for( QStringList::iterator it = paths.begin(); it != paths.end(); ++it )
	{
		addItems( *it );
	}

	Q3ListViewItem * item = m_l->firstChild();
	bool resort = FALSE;

	// sort merged directories
	while( item != NULL )
	{
		directory * d = dynamic_cast<directory *>( item );
		if( d == NULL )
		{
			resort = TRUE;
		}
		else if( resort == TRUE )
		{
			Q3ListViewItem * i2 = m_l->firstChild();
			d->moveItem( i2 );
			i2->moveItem( d );
			directory * d2 = NULL;
			while( ( d2 = dynamic_cast<directory *>( i2 ) ) !=
									NULL )
			{
				if( d->text( 0 ) > d2->text( 0 ) )
				{
					d->moveItem( d2 );
				}
				i2 = i2->nextSibling();
			}
		}
		item = item->nextSibling();
	}
}




void fileBrowser::addItems( const QString & _path )
{
	QDir cdir( _path );
	QStringList files = cdir.entryList( QDir::Files, QDir::Name );

	// TODO: after dropping qt3-support we can use QStringList's iterator
	// which makes it possible to travel through the list in reverse
	// direction

	for( csize i = 0; i < files.size(); ++i )
	{
		QString cur_file = files[files.size() - i - 1];
		if( cur_file[0] != '.'
#ifdef QT4
// TBD
#else
			&& QDir::match( m_filter, cur_file.lower() )
#endif
	)
		{
			// remove existing file-items
			delete m_l->findItem( cur_file, 0 );
			(void) new fileItem( m_l, cur_file, _path );
		}
	}

	files = cdir.entryList( QDir::Dirs, QDir::Name );
	for( csize i = 0; i < files.size(); ++i )
	{
		QString cur_file = files[files.size() - i - 1];
		if( cur_file[0] != '.' &&
			isDirWithContent( _path + QDir::separator() + cur_file,
								m_filter ) )
		{
			QListViewItem * item = m_l->findItem( cur_file, 0 );
			if( item == NULL )
			{
				(void) new directory( m_l, cur_file, _path,
							      	m_filter );
			}
			else if( dynamic_cast<directory *>( item ) != NULL )
			{
				dynamic_cast<directory *>( item )->
							addDirectory( _path );
			}
		}
	}
}




bool fileBrowser::isDirWithContent( const QString & _path,
						const QString & _filter )
{
	QDir cdir( _path );
	QStringList files = cdir.entryList( QDir::Files, QDir::Unsorted );
	for( QStringList::iterator it = files.begin(); it != files.end(); ++it )
	{
		QString cur_file = *it;
		if( cur_file[0] != '.'
#ifdef QT4
// TBD
#else
			&& QDir::match( _filter, cur_file.lower() )
#endif
	)
		{
			return( TRUE );
		}
	}

	files = cdir.entryList( QDir::Dirs, QDir::Unsorted );
	for( QStringList::iterator it = files.begin(); it != files.end(); ++it )
	{
		QString cur_file = *it;
		if( cur_file[0] != '.' &&
			isDirWithContent( _path + QDir::separator() + cur_file,
								_filter ) )
		{
			return( TRUE );
		}
	}

	return( FALSE );
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
		contextMenu->addAction( tr( "Send to active instrument-track" ),
						this,
					SLOT( sendToActiveInstrumentTrack() ) );
		contextMenu->addAction( tr( "Open in new instrument-track/"
								"Song-Editor" ),
						this,
					SLOT( openInNewInstrumentTrackSE() ) );
		contextMenu->addAction( tr( "Open in new instrument-track/"
								"B+B Editor" ),
						this,
					SLOT( openInNewInstrumentTrackBBE() ) );
		contextMenu->exec( QCursor::pos() );
		m_contextMenuItem = NULL;
		delete contextMenu;
	}

}




void fileBrowser::sendToActiveInstrumentTrack( void )
{
	if( engine::getMainWindow()->workspace() == NULL )
	{
		return;
	}

	// get all windows opened in the workspace
	QWidgetList pl = engine::getMainWindow()->workspace()->windowList(
#if QT_VERSION >= 0x030200
						QWorkspace::StackingOrder
#endif
									);
#ifdef QT4
	QListIterator<QWidget *> w( pl );
	w.toBack();
	// now we travel through the window-list until we find an
	// instrument-track
	while( w.hasPrevious() )
	{
		instrumentTrack * ct = dynamic_cast<instrumentTrack *>(
								w.previous() );
#else
	QWidget * w = pl.last();
	// now we travel through the window-list until we find an
	// instrument-track
	while( w != NULL )
	{
		instrumentTrack * ct = dynamic_cast<instrumentTrack *>( w );
#endif
		if( ct != NULL && ct->isHidden() == FALSE )
		{
			// ok, it's an instrument-track, so we can apply the
			// sample or the preset
			engine::getMixer()->lock();
			if( m_contextMenuItem->type() == fileItem::SAMPLE_FILE )
			{
				instrument * afp = ct->loadInstrument(
						engine::sampleExtensions()
							[m_contextMenuItem
							->extension()] );
				if( afp != NULL )
				{
					afp->setParameter( "samplefile",
						m_contextMenuItem->fullName() );
				}
			}
			else if( m_contextMenuItem->type() ==
							fileItem::PRESET_FILE )
			{
				multimediaProject mmp(
						m_contextMenuItem->fullName() );
				ct->loadTrackSpecificSettings( mmp.content().
								firstChild().
								toElement() );
			}
			ct->toggledInstrumentTrackButton( TRUE );
			engine::getMixer()->unlock();
			break;
		}
#ifndef QT4
		w = pl.prev();
#endif
	}
}




void fileBrowser::openInNewInstrumentTrack( trackContainer * _tc )
{
	engine::getMixer()->lock();
	if( m_contextMenuItem->type() == fileItem::SAMPLE_FILE )
	{
		instrumentTrack * ct = dynamic_cast<instrumentTrack *>(
			track::create( track::INSTRUMENT_TRACK, _tc ) );
#ifdef LMMS_DEBUG
		assert( ct != NULL );
#endif
		instrument * afp = ct->loadInstrument(
					engine::sampleExtensions()
						[m_contextMenuItem
							->extension()] );
		if( afp != NULL )
		{
			afp->setParameter( "samplefile",
						m_contextMenuItem->fullName() );
		}
		ct->toggledInstrumentTrackButton( TRUE );
	}
	else if( m_contextMenuItem->type() == fileItem::PRESET_FILE )
	{
		multimediaProject mmp( m_contextMenuItem->fullName() );
		track * t = track::create( track::INSTRUMENT_TRACK, _tc );
		instrumentTrack * ct = dynamic_cast<instrumentTrack *>( t );
		if( ct != NULL )
		{
			ct->loadTrackSpecificSettings( mmp.content().
							firstChild().
							toElement() );
			ct->toggledInstrumentTrackButton( TRUE );
		}
	}
	engine::getMixer()->unlock();
}




void fileBrowser::openInNewInstrumentTrackSE( void )
{
	openInNewInstrumentTrack( engine::getSongEditor() );
}




void fileBrowser::openInNewInstrumentTrackBBE( void )
{
	openInNewInstrumentTrack( engine::getBBEditor() );
}








listView::listView( QWidget * _parent ) :
	Q3ListView( _parent ),
	m_mousePressed( FALSE ),
	m_pressPos(),
	m_previewPlayHandle( NULL )
{
	addColumn( tr( "Files" ) );
	setTreeStepSize( 12 );
	setSorting( -1 );
	setShowToolTips( TRUE );

	setFont( pointSizeF( font(), 7.5f ) );
}




listView::~listView()
{
}




void listView::contentsMouseDoubleClickEvent( QMouseEvent * _me )
{
	Q3ListView::contentsMouseDoubleClickEvent( _me );
	fileItem * f = dynamic_cast<fileItem *>( itemAt(
					contentsToViewport( _me->pos() ) ) );
	if( f == NULL )
	{
		return;
	}

	if( f->type() == fileItem::SAMPLE_FILE )
	{
		// samples are per default opened in bb-editor because they're
		// likely drum-samples etc.
		engine::getMixer()->lock();
		instrumentTrack * it = dynamic_cast<instrumentTrack *>(
				track::create( track::INSTRUMENT_TRACK,
						engine::getBBEditor() ) );
#ifdef LMMS_DEBUG
		assert( it != NULL );
#endif
		instrument * afp = it->loadInstrument(
				engine::sampleExtensions()[f->extension()] );
		if( afp != NULL )
		{
			afp->setParameter( "samplefile", f->fullName() );
		}
		it->toggledInstrumentTrackButton( TRUE );
		engine::getMixer()->unlock();
	}
	else if( f->type() == fileItem::PRESET_FILE )
	{
		// presets are per default opened in bb-editor
		multimediaProject mmp( f->fullName() );
		engine::getMixer()->lock();
		instrumentTrack * it = dynamic_cast<instrumentTrack *>(
				track::create( track::INSTRUMENT_TRACK,
						engine::getBBEditor() ) );
		if( it != NULL )
		{
			it->loadTrackSpecificSettings( mmp.content().
								firstChild().
								toElement() );
			it->toggledInstrumentTrackButton( TRUE );
		}
		engine::getMixer()->unlock();
	}
	else if( f->type() == fileItem::PROJECT_FILE )
	{
		if( engine::getSongEditor()->mayChangeProject() )
		{
			engine::getSongEditor()->loadProject( f->fullName() );
		}
	}
}




void listView::contentsMousePressEvent( QMouseEvent * _me )
{
	Q3ListView::contentsMousePressEvent( _me );
	if( _me->button() != Qt::LeftButton )
	{
		return;
	}

	QPoint p( contentsToViewport( _me->pos() ) );
	Q3ListViewItem * i = itemAt( p );
        if ( i )
	{
		if ( p.x() > header()->cellPos( header()->mapToActual( 0 ) ) +
			treeStepSize() * ( i->depth() + ( rootIsDecorated() ?
						1 : 0 ) ) + itemMargin() ||
				p.x() < header()->cellPos(
						header()->mapToActual( 0 ) ) )
		{
			m_pressPos = _me->pos();
			m_mousePressed = TRUE;
		}
	}

	fileItem * f = dynamic_cast<fileItem *>( i );
	if( f != NULL )
	{
		if( m_previewPlayHandle != NULL )
		{
			engine::getMixer()->removePlayHandle(
							m_previewPlayHandle );
			m_previewPlayHandle = NULL;
		}
		if( f->type() == fileItem::SAMPLE_FILE )
		{
			textFloat * tf = textFloat::displayMessage(
					tr( "Loading sample" ),
					tr( "Please wait, loading sample for "
								"preview..." ),
					embed::getIconPixmap( "sound_file",
								24, 24 ), 0 );
#ifdef QT4
			qApp->processEvents( QEventLoop::AllEvents );
#else
			qApp->processEvents();
#endif
			samplePlayHandle * s = new samplePlayHandle(
								f->fullName() );
			s->setDoneMayReturnTrue( FALSE );
			m_previewPlayHandle = s;
			delete tf;
		}
		else if( f->type() == fileItem::PRESET_FILE )
		{
			m_previewPlayHandle = new presetPreviewPlayHandle(
								f->fullName() );
		}
		if( m_previewPlayHandle != NULL )
		{
			engine::getMixer()->addPlayHandle(
							m_previewPlayHandle );
		}
	}
}




void listView::contentsMouseMoveEvent( QMouseEvent * _me )
{
	if( m_mousePressed == TRUE &&
		( m_pressPos - _me->pos() ).manhattanLength() >
					QApplication::startDragDistance() )
	{
		contentsMouseReleaseEvent( NULL );
		fileItem * f = dynamic_cast<fileItem *>( itemAt(
					contentsToViewport( m_pressPos ) ) );
		if( f != NULL )
		{
			switch( f->type() )
			{
				case fileItem::PRESET_FILE:
					new stringPairDrag( "presetfile",
								f->fullName(),
							embed::getIconPixmap(
								"preset_file" ),
									this );
					break;

				case fileItem::SAMPLE_FILE:
					new stringPairDrag( "samplefile",
								f->fullName(),
							embed::getIconPixmap(
								"sound_file" ),
									this );
					break;

				case fileItem::MIDI_FILE:
					new stringPairDrag( "midifile",
								f->fullName(),
							embed::getIconPixmap(
								"midi_file" ),
									this );
					break;

				default:
					break;
			}
		}
	}
}




void listView::contentsMouseReleaseEvent( QMouseEvent * _me )
{
	m_mousePressed = FALSE;
	if( m_previewPlayHandle != NULL )
	{
		// if there're samples shorter than 3 seconds, we don't
		// stop them if the user releases mouse-button...
		samplePlayHandle * s = dynamic_cast<samplePlayHandle *>(
							m_previewPlayHandle );
		if( s != NULL )
		{
			if( s->totalFrames() - s->framesDone() <=
				static_cast<f_cnt_t>(
					engine::getMixer()->sampleRate() * 3 ) )
			{
				s->setDoneMayReturnTrue( TRUE );
				m_previewPlayHandle = NULL;
				return;
			}
		}
		engine::getMixer()->removePlayHandle( m_previewPlayHandle );
		m_previewPlayHandle = NULL;
	}
}







QPixmap * directory::s_folderPixmap = NULL;
QPixmap * directory::s_folderOpenedPixmap = NULL;
QPixmap * directory::s_folderLockedPixmap = NULL;


directory::directory( directory * _parent, const QString & _name,
			const QString & _path, const QString & _filter ) :
	Q3ListViewItem( _parent, _name ),
	m_p( _parent ),
	m_pix( NULL ),
	m_directories( _path ),
	m_filter( _filter )
{
	initPixmapStuff();
}




directory::directory( Q3ListView * _parent, const QString & _name,
			const QString & _path, const QString & _filter ) :
	Q3ListViewItem( _parent, _name ),
	m_p( NULL ),
	m_pix( NULL ),
	m_directories( _path ),
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




void directory::setPixmap( const QPixmap * _px )
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
		for( QStringList::iterator it = m_directories.begin();
					it != m_directories.end(); ++it )
		{
			if( addItems( fullName( *it ) ) &&
				( *it ).contains(
					configManager::inst()->dataDir() ) )
			{
				( new QListViewItem( this,
		listView::tr( "--- Factory files ---" ) ) )->setPixmap( 0,
				embed::getIconPixmap( "factory_files" ) );
			}
		}
	}
	Q3ListViewItem::setOpen( _o );
}




void directory::setup( void )
{
	setExpandable( TRUE );
	Q3ListViewItem::setup();
}




bool directory::addItems( const QString & _path )
{
	QDir thisDir( _path );
	if( !thisDir.isReadable() )
	{
		//readable = FALSE;
		setExpandable( FALSE );
		return( FALSE );
	}

	listView()->setUpdatesEnabled( FALSE );

	bool added_something = FALSE;

	QStringList files = thisDir.entryList( QDir::Files, QDir::Name );
	for( csize i = 0; i < files.size(); ++i )
	{
		QString cur_file = files[files.size() - i - 1];
		if( cur_file[0] != '.'
#ifdef QT4
				&& thisDir.match( m_filter, cur_file.toLower() )
#else
				&& thisDir.match( m_filter, cur_file.lower() )
#endif
			/*QDir::match( FILE_FILTER, cur_file )*/ )
		{
			(void) new fileItem( this, cur_file, _path );
			added_something = TRUE;
		}
	}

	files = thisDir.entryList( QDir::Dirs, QDir::Name );
	for( csize i = 0; i < files.size(); ++i )
	{
		QString cur_file = files[files.size() - i - 1];
		if( cur_file[0] != '.' && fileBrowser::isDirWithContent(
#ifdef QT4
				thisDir.absolutePath() + QDir::separator() +
#else
					thisDir.absPath() + QDir::separator() +
#endif
							cur_file, m_filter ) )
		{
			new directory( this, cur_file, _path, m_filter );
			added_something = TRUE;
#if 0
			if( firstChild() == NULL )
			{
				continue;
			}
			bool moved = FALSE;
			QListViewItem * item = firstChild();
			while( item != NULL )
			{
				directory * cd =
					dynamic_cast<directory *>( item );
				if( cd != NULL )
				{
/*					if( moved == FALSE ||
						cd->text( 0 ) < cur_file )
					{*/
						printf( "move item %s after %s\n", d->text(0).ascii(), cd->text(0).ascii());
						d->moveItem( cd );
						moved = TRUE;
					//}
				}
				item = item->nextSibling();
			}
#endif
		}
	}

	listView()->setUpdatesEnabled( TRUE );

	return( added_something );
}




QPixmap * fileItem::s_projectFilePixmap = NULL;
QPixmap * fileItem::s_presetFilePixmap = NULL;
QPixmap * fileItem::s_sampleFilePixmap = NULL;
QPixmap * fileItem::s_midiFilePixmap = NULL;
QPixmap * fileItem::s_flpFilePixmap = NULL;
QPixmap * fileItem::s_unknownFilePixmap = NULL;


fileItem::fileItem( Q3ListView * _parent, const QString & _name,
						const QString & _path ) :
	Q3ListViewItem( _parent, _name ),
	m_pix( NULL ),
	m_path( _path )
{
	determineFileType();
	initPixmapStuff();
	setDragEnabled( TRUE );
}




fileItem::fileItem( Q3ListViewItem * _parent, const QString & _name,
						const QString & _path ) :
	Q3ListViewItem( _parent, _name ),
	m_pix( NULL ),
	m_path( _path )
{
	determineFileType();
	initPixmapStuff();
	setDragEnabled( TRUE );
}




void fileItem::initPixmapStuff( void )
{
	if( s_projectFilePixmap == NULL )
	{
		s_projectFilePixmap = new QPixmap( embed::getIconPixmap(
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

	if( s_midiFilePixmap == NULL )
	{
		s_midiFilePixmap = new QPixmap( embed::getIconPixmap(
							"midi_file", 16, 16 ) );
	}

	if( s_flpFilePixmap == NULL )
	{
		s_flpFilePixmap = new QPixmap( embed::getIconPixmap(
							"midi_file", 16, 16 ) );
	}

	if( s_unknownFilePixmap == NULL )
	{
		s_unknownFilePixmap = new QPixmap( embed::getIconPixmap(
							"unknown_file" ) );
	}

	switch( m_type )
	{
		case PROJECT_FILE: m_pix = s_projectFilePixmap; break;
		case PRESET_FILE: m_pix = s_presetFilePixmap; break;
		case SAMPLE_FILE: m_pix = s_sampleFilePixmap; break;
		case MIDI_FILE: m_pix = s_midiFilePixmap; break;
		case FLP_FILE: m_pix = s_flpFilePixmap; break;
		case UNKNOWN:
		default:
			m_pix = s_unknownFilePixmap;
			break;
	}
}




void fileItem::determineFileType( void )
{
	QString ext = extension();
	if( ext == "mmp" || ext == "mpt" || ext == "mmpz" )
	{
		m_type = PROJECT_FILE;
	}
	else if( ext == "xml" )
	{
		multimediaProject::projectTypes t =
				multimediaProject::typeOfFile( fullName() );
		if( t == multimediaProject::SONG_PROJECT )
		{
			m_type = PROJECT_FILE;
		}
		else if( t == multimediaProject::INSTRUMENT_TRACK_SETTINGS )
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
	else if( engine::sampleExtensions().contains( ext ) )
	{
		m_type = SAMPLE_FILE;
	}
	else if( ext == "mid" )
	{
		m_type = MIDI_FILE;
	}
	else if( ext == "flp" )
	{
		m_type = FLP_FILE;
	}
	else
	{
		m_type = UNKNOWN;
	}
}




QString fileItem::extension( void )
{
	return( extension( fullName() ) );
}




QString fileItem::extension( const QString & _file )
{
#ifdef QT4
	return( QFileInfo( _file ).suffix().toLower() );
#else
	return( QFileInfo( _file ).extension( FALSE ).toLower() );
#endif
}




#include "file_browser.moc"


#endif
