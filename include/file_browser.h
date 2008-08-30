/*
 * file_browser.h - include file for fileBrowser
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


#ifndef _FILE_BROWSER_H
#define _FILE_BROWSER_H

#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtGui/QTreeWidget>


#include "side_bar_widget.h"


class QColorGroup;
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


public slots:
	void reloadTree( void );


private:
	virtual void keyPressEvent( QKeyEvent * _ke );

	void addItems( const QString & _path );

	listView * m_l;

	QString m_directories;
	QString m_filter;


} ;




class listView : public QTreeWidget
{
	Q_OBJECT
public:
	listView( QWidget * _parent );
	virtual ~listView();


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _e );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );


private:
	bool m_mousePressed;
	QPoint m_pressPos;

	playHandle * m_previewPlayHandle;
	QMutex m_pphMutex;

	fileItem * m_contextMenuItem;

	void openInNewInstrumentTrack( trackContainer * _tc );


private slots:
	void activateListItem( QTreeWidgetItem * _item, int _column );
	void openInNewInstrumentTrackBBE( void );
	void openInNewInstrumentTrackSE( void );
	void sendToActiveInstrumentTrack( void );
	void updateDirectory( QTreeWidgetItem * _item );

} ;




class directory : public QTreeWidgetItem
{
public:
	directory( const QString & _filename, const QString & _path,
						const QString & _filter );

	void update( void );

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

	inline void addDirectory( const QString & _dir )
	{
		m_directories.push_back( _dir );
	}


private:
	void initPixmapStuff( void );

	bool addItems( const QString & _path );


	static QPixmap * s_folderPixmap;
	static QPixmap * s_folderOpenedPixmap;
	static QPixmap * s_folderLockedPixmap;

	QStringList m_directories;
	QString m_filter;

} ;




class fileItem : public QTreeWidgetItem
{
public:
	fileItem( QTreeWidget * _parent, const QString & _name,
							const QString & _path );
	fileItem( QTreeWidgetItem * _parent, const QString & _name,
							const QString & _path );

	inline QString fullName( void ) const
	{
		return( QDir::cleanPath( m_path ) + QDir::separator() +
								text( 0 ) );
	}

	enum FileTypes
	{
		ProjectFile,
		PresetFile,
		SpecialPresetFile,
		SampleFile,
		MidiFile,
		FlpFile,
		UnknownFile,
		NumFileTypes
	} ;
	
	inline FileTypes type( void )
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
	
	QString m_path;
	FileTypes m_type;
} ;



#endif
