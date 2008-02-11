/*
 * file_browser.h - include file for fileBrowser
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


#ifndef _FILE_BROWSER_H
#define _FILE_BROWSER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "qt3support.h"

#ifdef QT4

#include <QtCore/QDir>
#include <QtCore/QMutex>

#else

#include <qlistview.h>
#include <qdir.h>
#include <qmutex.h>

#define cleanPath cleanDirPath

#endif


#include "side_bar_widget.h"


#ifndef QT3
class QListViewItem;
#else
class Q3ListViewItem;
#endif
class QPixmap;

class fileItem;
class listView;
class playHandle;
class trackContainer;



class fileBrowser : public sideBarWidget
{
	Q_OBJECT
public:
	fileBrowser( const QString & _directories, const QString & _filter,
			const QString & _title, const QPixmap & _pm,
							QWidget * _parent );
	virtual ~fileBrowser();

	static bool isDirWithContent( const QString & _path,
						const QString & _filter );


public slots:
	void reloadTree( void );


protected slots:
	void contextMenuRequest( Q3ListViewItem * _i, const QPoint & _pos,
								int _col );
	void contextMenuRequest( QListViewItem * _i, const QPoint & _pos,
								int _col );
	void sendToActiveInstrumentTrack( void );
	void openInNewInstrumentTrackSE( void );
	void openInNewInstrumentTrackBBE( void );


private:
	virtual void keyPressEvent( QKeyEvent * _ke );

	void addItems( const QString & _path );
	void openInNewInstrumentTrack( trackContainer * _tc );

	listView * m_l;
	fileItem * m_contextMenuItem;

	QString m_directories;
	QString m_filter;


} ;




class listView : public Q3ListView
{
	Q_OBJECT
public:
	listView( QWidget * _parent );
	virtual ~listView();


protected:
	virtual void contentsMouseDoubleClickEvent( QMouseEvent * _me );
	virtual void contentsMousePressEvent( QMouseEvent * _me );
	virtual void contentsMouseMoveEvent( QMouseEvent * _me );
	virtual void contentsMouseReleaseEvent( QMouseEvent * _me );


private:
	bool m_mousePressed;
	QPoint m_pressPos;

	playHandle * m_previewPlayHandle;
	QMutex m_pphMutex;

} ;




class directory :
#ifndef QT3
	public Q3ListViewItem
// trick moc
#if 0
		,
#endif
#else
	public QListViewItem
#endif
{
public:
	directory( Q3ListView * _parent, const QString & _filename,
			const QString & _path, const QString & _filter );
	directory( directory * _parent, const QString & _filename,
			const QString & _path, const QString & _filter );

	void setOpen( bool );
	void setup( void );

	inline QString fullName( QString _path = QString::null )
	{
		if( _path == QString::null )
		{
			_path = m_directories[0];
		}
		return( QDir::cleanPath( _path + QDir::separator() +
								text( 0 ) ) +
							QDir::separator() );
	}

	inline const QPixmap * pixmap( int ) const
	{
		return( m_pix );
	}

	inline void addDirectory( const QString & _dir )
	{
		m_directories.push_back( _dir );
	}


private:
	void initPixmapStuff( void );
	void FASTCALL setPixmap( const QPixmap * _px );

	bool addItems( const QString & _path );


	static QPixmap * s_folderPixmap;
	static QPixmap * s_folderOpenedPixmap;
	static QPixmap * s_folderLockedPixmap;

	directory * m_p;
	const QPixmap * m_pix;
	QStringList m_directories;
	QString m_filter;

} ;




class fileItem :
#ifndef QT3
	public Q3ListViewItem
// trick moc
#if 0
		,
#endif
#else
	public QListViewItem
#endif
{
public:
	fileItem( Q3ListView * _parent, const QString & _name,
							const QString & _path );
#ifndef QT3
	fileItem( Q3ListViewItem * _parent, const QString & _name,
							const QString & _path );
#else
	fileItem( QListViewItem * _parent, const QString & _name,
							const QString & _path );
#endif

	QString fullName( void ) const
	{
		return( QDir::cleanPath( m_path ) + QDir::separator() +
								text( 0 ) );
	}
	inline const QPixmap * pixmap( int ) const
	{
		return( m_pix );
	}

	enum fileTypes
	{
		PROJECT_FILE, PRESET_FILE, SAMPLE_FILE, MIDI_FILE, FLP_FILE,
		UNKNOWN
	} ;
	
	inline fileTypes type( void )
	{
		return( m_type );
	}

	QString extension( void );
	static QString extension( const QString & _file );


private:
	void initPixmapStuff( void );
	void determineFileType( void );

	static QPixmap * s_projectFilePixmap;
	static QPixmap * s_presetFilePixmap;
	static QPixmap * s_sampleFilePixmap;
	static QPixmap * s_midiFilePixmap;
	static QPixmap * s_flpFilePixmap;
	static QPixmap * s_unknownFilePixmap;
	
	QPixmap * m_pix;
	QString m_path;
	fileTypes m_type;
} ;


#ifdef QT3
#undef cleanPath
#endif


#endif
