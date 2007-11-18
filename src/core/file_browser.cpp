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


#include "file_browser.h"


#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QPushButton>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>


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
	m_directories( _directories ),
	m_filter( _filter )
{
	setWindowTitle( tr( "Browser" ) );
	m_l = new listView( contentParent() );
	addContentWidget( m_l );

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
	QStringList paths = m_directories.split( '*' );
	for( QStringList::iterator it = paths.begin(); it != paths.end(); ++it )
	{
		addItems( *it );
	}
}




void fileBrowser::addItems( const QString & _path )
{
	QDir cdir( _path );
	QStringList files = cdir.entryList( QDir::Dirs, QDir::Name );
	for( QStringList::const_iterator it = files.constBegin();
						it != files.constEnd(); ++it )
	{
		QString cur_file = *it;
		if( cur_file[0] != '.' &&
			isDirWithContent( _path + QDir::separator() + cur_file,
								m_filter ) )
		{
			bool orphan = TRUE;
			for( int i = 0; i < m_l->topLevelItemCount(); ++i )
			{
				directory * d = dynamic_cast<directory *>(
						m_l->topLevelItem( i ) );
				if( d == NULL || cur_file < d->text( 0 ) )
				{
					m_l->insertTopLevelItem( i,
						new directory( cur_file, _path,
								m_filter ) );
					orphan = FALSE;
					break;
				}
				else if( cur_file == d->text( 0 ) )
				{
					d->addDirectory( _path );
					orphan = FALSE;
					break;
				}
			}
			if( orphan )
			{
				m_l->addTopLevelItem( new directory( cur_file,
							_path, m_filter ) );
			}
		}
	}

	files = cdir.entryList( QDir::Files, QDir::Name );
	for( QStringList::const_iterator it = files.constBegin();
						it != files.constEnd(); ++it )
	{
		QString cur_file = *it;
		if( cur_file[0] != '.'
#warning TODO: add match here
#ifdef QT4
// TBD
#else
//			&& QDir::match( m_filter, cur_file.lower() )
#endif
	)
		{
			// TODO: don't insert instead of removing, order changed
			// remove existing file-items
			QList<QTreeWidgetItem *> existing = m_l->findItems(
					cur_file, Qt::MatchFixedString );
			if( !existing.empty() )
			{
				delete existing.front();
			}
			(void) new fileItem( m_l, cur_file, _path );
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
#warning TODO: add match here
				&& QDir::match( _filter, cur_file.toLower() )
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








listView::listView( QWidget * _parent ) :
	QTreeWidget( _parent ),
	m_mousePressed( FALSE ),
	m_pressPos(),
	m_previewPlayHandle( NULL ),
	m_pphMutex(),
	m_contextMenuItem( NULL )
{
	setColumnCount( 1 );
	setHeaderLabel( tr( "Files" ) );
	setSortingEnabled( FALSE );

	setFont( pointSizeF( font(), 7.5f ) );

	connect( this, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ),
			SLOT( activateListItem( QTreeWidgetItem *, int ) ) );
	connect( this, SIGNAL( itemCollapsed( QTreeWidgetItem * ) ),
				SLOT( updateDirectory( QTreeWidgetItem * ) ) );
	connect( this, SIGNAL( itemExpanded( QTreeWidgetItem * ) ),
				SLOT( updateDirectory( QTreeWidgetItem * ) ) );
}




listView::~listView()
{
}




void listView::activateListItem( QTreeWidgetItem * _item, int _column )
{
	fileItem * f = dynamic_cast<fileItem *>( _item );
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




void listView::sendToActiveInstrumentTrack( void )
{
	if( engine::getMainWindow()->workspace() == NULL )
	{
		return;
	}

	// get all windows opened in the workspace
	QList<QMdiSubWindow*> pl = engine::getMainWindow()->workspace()->subWindowList(
						QMdiArea::StackingOrder );
	QListIterator<QMdiSubWindow *> w( pl );
	w.toBack();
	// now we travel through the window-list until we find an
	// instrument-track
	while( w.hasPrevious() )
	{
		instrumentTrack * ct = dynamic_cast<instrumentTrack *>(
								w.previous()->widget() );
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
	}
}




void listView::openInNewInstrumentTrack( trackContainer * _tc )
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




void listView::openInNewInstrumentTrackBBE( void )
{
	openInNewInstrumentTrack( engine::getBBEditor() );
}




void listView::openInNewInstrumentTrackSE( void )
{
	openInNewInstrumentTrack( engine::getSongEditor() );
}




void listView::updateDirectory( QTreeWidgetItem * _item )
{
	directory * dir = dynamic_cast<directory *>( _item );
	if( dir != NULL )
	{
		dir->update();
	}
}




void listView::contextMenuEvent( QContextMenuEvent * _e )
{
	fileItem * f = dynamic_cast<fileItem *>( itemAt( _e->pos() ) );
	if( f != NULL && ( f->type() == fileItem::SAMPLE_FILE ||
				f->type() == fileItem::PRESET_FILE ) )
	{
		m_contextMenuItem = f;
		QMenu contextMenu( this );
		contextMenu.addAction( tr( "Send to active instrument-track" ),
						this,
					SLOT( sendToActiveInstrumentTrack() ) );
		contextMenu.addAction( tr( "Open in new instrument-track/"
								"Song-Editor" ),
						this,
					SLOT( openInNewInstrumentTrackSE() ) );
		contextMenu.addAction( tr( "Open in new instrument-track/"
								"B+B Editor" ),
						this,
					SLOT( openInNewInstrumentTrackBBE() ) );
		contextMenu.exec( _e->globalPos() );
		m_contextMenuItem = NULL;
	}
}




void listView::mousePressEvent( QMouseEvent * _me )
{
	QTreeWidget::mousePressEvent( _me );
	if( _me->button() != Qt::LeftButton )
	{
		return;
	}

	QTreeWidgetItem * i = itemAt( _me->pos() );
        if ( i )
	{
		// TODO: Restrict to visible selection
//		if ( _me->x() > header()->cellPos( header()->mapToActual( 0 ) )
//			+ treeStepSize() * ( i->depth() + ( rootIsDecorated() ?
//						1 : 0 ) ) + itemMargin() ||
//				_me->x() < header()->cellPos(
//						header()->mapToActual( 0 ) ) )
//		{
			m_pressPos = _me->pos();
			m_mousePressed = TRUE;
//		}
	}

	fileItem * f = dynamic_cast<fileItem *>( i );
	if( f != NULL )
	{
		if( !m_pphMutex.tryLock() )
		{
			return;
		}
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
			qApp->processEvents( QEventLoop::AllEvents );
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
		m_pphMutex.unlock();
	}
}




void listView::mouseMoveEvent( QMouseEvent * _me )
{
	if( m_mousePressed == TRUE &&
		( m_pressPos - _me->pos() ).manhattanLength() >
					QApplication::startDragDistance() )
	{
		mouseReleaseEvent( NULL );
		fileItem * f = dynamic_cast<fileItem *>( itemAt( m_pressPos ) );
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




void listView::mouseReleaseEvent( QMouseEvent * _me )
{
	if( !m_pphMutex.tryLock() )
	{
		return;
	}

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
				m_pphMutex.unlock();
				return;
			}
		}
		engine::getMixer()->removePlayHandle( m_previewPlayHandle );
		m_previewPlayHandle = NULL;
	}
	m_pphMutex.unlock();
}







QPixmap * directory::s_folderPixmap = NULL;
QPixmap * directory::s_folderOpenedPixmap = NULL;
QPixmap * directory::s_folderLockedPixmap = NULL;


directory::directory( const QString & _name, const QString & _path,
						const QString & _filter ) :
	m_directories( _path ),
	m_filter( _filter )
{
	initPixmapStuff();

	setText( 0, _name );
	setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );

	if( !QDir( fullName() ).isReadable() )
	{
		setIcon( 0, *s_folderLockedPixmap );
	}
	else
	{
		setIcon( 0, *s_folderPixmap );
	}
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
}




void directory::update( void )
{
	if( !isExpanded() )
	{
		setIcon( 0, *s_folderPixmap );
		return;
	}

	setIcon( 0, *s_folderOpenedPixmap );
	if( !childCount() )
	{
		for( QStringList::iterator it = m_directories.begin();
					it != m_directories.end(); ++it )
		{
			int top_index = childCount();
			if( addItems( fullName( *it ) ) &&
				( *it ).contains(
					configManager::inst()->dataDir() ) )
			{
				QTreeWidgetItem * sep = new QTreeWidgetItem;
				sep->setText( 0, listView::tr(
						"--- Factory files ---" ) );
				sep->setIcon( 0, embed::getIconPixmap(
							"factory_files" ) );
				insertChild( top_index, sep );
			}
		}
	}
}




bool directory::addItems( const QString & _path )
{
	QDir thisDir( _path );
	if( !thisDir.isReadable() )
	{
		return( FALSE );
	}

	treeWidget()->setUpdatesEnabled( FALSE );

	bool added_something = FALSE;

	QStringList files = thisDir.entryList( QDir::Dirs, QDir::Name );
	for( QStringList::const_iterator it = files.constBegin();
						it != files.constEnd(); ++it )
	{
		QString cur_file = *it;
		if( cur_file[0] != '.' && fileBrowser::isDirWithContent(
				thisDir.absolutePath() + QDir::separator() +
							cur_file, m_filter ) )
		{
			bool orphan = TRUE;
			for( int i = 0; i < childCount(); ++i )
			{
				directory * d = dynamic_cast<directory *>(
								child( i ) );
				if( d == NULL || cur_file < d->text( 0 ) )
				{
					insertChild( i, new directory( cur_file,
							_path, m_filter ) );
					orphan = FALSE;
					break;
				}
				else if( cur_file == d->text( 0 ) )
				{
					d->addDirectory( _path );
					orphan = FALSE;
					break;
				}
			}
			if( orphan )
			{
				addChild( new directory( cur_file, _path,
								m_filter ) );
			}

			added_something = TRUE;
		}
	}

	files = thisDir.entryList( QDir::Files, QDir::Name );
	for( QStringList::const_iterator it = files.constBegin();
						it != files.constEnd(); ++it )
	{
		QString cur_file = *it;
		if( cur_file[0] != '.'
				&& thisDir.match( m_filter, cur_file.toLower() )
			/*QDir::match( FILE_FILTER, cur_file )*/ )
		{
			(void) new fileItem( this, cur_file, _path );
			added_something = TRUE;
		}
	}

	treeWidget()->setUpdatesEnabled( TRUE );

	return( added_something );
}




QPixmap * fileItem::s_projectFilePixmap = NULL;
QPixmap * fileItem::s_presetFilePixmap = NULL;
QPixmap * fileItem::s_sampleFilePixmap = NULL;
QPixmap * fileItem::s_midiFilePixmap = NULL;
QPixmap * fileItem::s_flpFilePixmap = NULL;
QPixmap * fileItem::s_unknownFilePixmap = NULL;


fileItem::fileItem( QTreeWidget * _parent, const QString & _name,
						const QString & _path ) :
	QTreeWidgetItem( _parent ),
	m_path( _path )
{
	setText( 0, _name );
	determineFileType();
	initPixmapStuff();
}




fileItem::fileItem( QTreeWidgetItem * _parent, const QString & _name,
						const QString & _path ) :
	QTreeWidgetItem( _parent ),
	m_path( _path )
{
	setText( 0, _name );
	determineFileType();
	initPixmapStuff();
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
		case PROJECT_FILE:
			setIcon( 0, *s_projectFilePixmap );
			break;
		case PRESET_FILE:
			setIcon( 0, *s_presetFilePixmap );
			break;
		case SAMPLE_FILE:
			setIcon( 0, *s_sampleFilePixmap );
			break;
		case MIDI_FILE:
			setIcon( 0, *s_midiFilePixmap );
			break;
		case FLP_FILE:
			setIcon( 0, *s_flpFilePixmap );
			break;
		case UNKNOWN:
		default:
			setIcon( 0, *s_unknownFilePixmap );
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
	return( QFileInfo( _file ).suffix().toLower() );
}




#include "file_browser.moc"


#endif
