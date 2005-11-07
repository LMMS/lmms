/*
 * file_browser.h - include file for fileBrowser
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _FILE_BROWSER_H
#define _FILE_BROWSER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "qt3support.h"

#ifdef QT4

#include <QDir>

#else

#include <qlistview.h>
#include <qdir.h>

#endif


#include "side_bar_widget.h"



class fileItem;
class trackContainer;
class QPixmap;
class playHandle;


class fileBrowser : public sideBarWidget
{
	Q_OBJECT
public:
	fileBrowser( const QString & _path, const QString & _filter,
			const QString & _title, const QPixmap & _pm,
							QWidget * _parent );
	virtual ~fileBrowser();


public slots:
	void reloadTree( void );


protected:
	void keyPressEvent( QKeyEvent * _ke );


protected slots:
#ifdef QT4
	void itemPressed( int btn, Q3ListViewItem * _i, const QPoint & _pos,
								int _col );
	void itemReleased( int btn, Q3ListViewItem * _i, const QPoint & _pos,
								int _col );
	void itemDoubleClicked( Q3ListViewItem * _i, const QPoint & _pos,
								int _col );
	void contextMenuRequest( Q3ListViewItem * _i, const QPoint & _pos,
								int _col );
#else
	void itemPressed( int btn, QListViewItem * _i, const QPoint & _pos,
								int _col );
	void itemReleased( int btn, QListViewItem * _i, const QPoint & _pos,
								int _col );
	void itemDoubleClicked( QListViewItem * _i, const QPoint & _pos,
								int _col );
	void contextMenuRequest( QListViewItem * _i, const QPoint & _pos,
								int _col );
#endif
	void selectionChanged( void );
	void sendToActiveChannel( void );
	void openInNewChannelSE( void );
	void openInNewChannelBBE( void );
	void renameItem( void );


private:
	void openInNewChannel( trackContainer * _tc );

	Q3ListView * m_l;
	fileItem * m_contextMenuItem;

	QString m_path;
	QString m_filter;

	playHandle * m_previewPlayHandle;

} ;



class directory : public Q3ListViewItem
{
public:
	directory( Q3ListView * _parent, const QString & _filename,
			const QString & _path, const QString & _filter );
	directory( directory * _parent, const QString & _filename,
			const QString & _path, const QString & _filter );

	void setOpen( bool );
	void setup( void );

	inline QString fullName( void )
	{
#ifdef QT4
		return( QDir::cleanPath( m_path + "/" + text( 0 ) + "/" ) );
#else
		return( QDir::cleanDirPath( m_path + "/" + text( 0 ) + "/" ) );
#endif
	}

	inline const QPixmap * pixmap( int ) const
	{
		return( m_pix );
	}


private:
	void initPixmapStuff( void );
	//using Q3ListViewItem::setPixmap;
	void FASTCALL setPixmap( QPixmap * _px );

	static QPixmap * s_folderPixmap;
	static QPixmap * s_folderOpenedPixmap;
	static QPixmap * s_folderLockedPixmap;

	directory * m_p;
	QPixmap * m_pix;
	QString m_path;
	QString m_filter;

} ;



class fileItem : public Q3ListViewItem
{
public:
	fileItem( Q3ListView * _parent, const QString & _name,
							const QString & _path );
	fileItem( Q3ListViewItem * _parent, const QString & _name,
							const QString & _path );

	inline QString fullName( void ) const
	{
		return( m_path + "/" + text( 0 ) );
	}
	inline const QPixmap * pixmap( int ) const
	{
		return( m_pix );
	}

	enum fileTypes
	{
		SONG_FILE, PRESET_FILE, SAMPLE_FILE, UNKNOWN
	} ;
	
	inline fileTypes type( void )
	{
		return( m_type );
	}


private:
	void initPixmapStuff( void );
	void determineFileType( void );

	static QPixmap * s_songFilePixmap;
	static QPixmap * s_presetFilePixmap;
	static QPixmap * s_sampleFilePixmap;
	static QPixmap * s_unknownFilePixmap;
	
	QPixmap * m_pix;
	QString m_path;
	fileTypes m_type;
} ;



#endif
