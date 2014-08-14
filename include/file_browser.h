/*
 * file_browser.h - include file for fileBrowser
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QTreeWidget>


#include "SideBarWidget.h"


class QLineEdit;

class fileItem;
class InstrumentTrack;
class fileBrowserTreeWidget;
class PlayHandle;
class TrackContainer;



class fileBrowser : public SideBarWidget
{
	Q_OBJECT
public:
	fileBrowser( const QString & _directories, const QString & _filter,
			const QString & _title, const QPixmap & _pm,
			QWidget * _parent, bool _dirs_as_items = false );
	virtual ~fileBrowser();


public slots:
	void filterItems( const QString & _filter );
	void reloadTree( void );


private:
	bool filterItems( QTreeWidgetItem * _item, const QString & _filter );
	virtual void keyPressEvent( QKeyEvent * _ke );

	void addItems( const QString & _path );

	fileBrowserTreeWidget * m_l;

	QLineEdit * m_filterEdit;

	QString m_directories;
	QString m_filter;

	bool m_dirsAsItems;

} ;




class fileBrowserTreeWidget : public QTreeWidget
{
	Q_OBJECT
public:
	fileBrowserTreeWidget( QWidget * _parent );
	virtual ~fileBrowserTreeWidget();


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _e );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );


private:
	void handleFile( fileItem * _fi, InstrumentTrack * _it );
	void openInNewInstrumentTrack( TrackContainer* tc );


	bool m_mousePressed;
	QPoint m_pressPos;

	PlayHandle* m_previewPlayHandle;
	QMutex m_pphMutex;

	fileItem * m_contextMenuItem;


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
		if( _path != QString::null )
		{
			_path += QDir::separator();
		}
		return( QDir::cleanPath( _path + text( 0 ) ) +
							QDir::separator() );
	}

	inline void addDirectory( const QString & _dir )
	{
		m_directories.push_back( _dir );
	}


private:
	void initPixmaps( void );

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
	enum FileTypes
	{
		ProjectFile,
		PresetFile,
		SampleFile,
		SoundFontFile,
		PatchFile,
		MidiFile,
		FlpFile,
		VstPluginFile,
		UnknownFile,
		NumFileTypes
	} ;

	enum FileHandling
	{
		NotSupported,
		LoadAsProject,
		LoadAsPreset,
		LoadByPlugin,
		ImportAsProject
	} ;


	fileItem( QTreeWidget * _parent, const QString & _name,
							const QString & _path );
	fileItem( const QString & _name, const QString & _path );

	inline QString fullName( void ) const
	{
		return( QDir::cleanPath( m_path ) + QDir::separator() +
								text( 0 ) );
	}

	inline FileTypes type( void ) const
	{
		return( m_type );
	}

	inline FileHandling handling( void ) const
	{
		return( m_handling );
	}

	QString extension( void );
	static QString extension( const QString & _file );


private:
	void initPixmaps( void );
	void determineFileType( void );

	static QPixmap * s_projectFilePixmap;
	static QPixmap * s_presetFilePixmap;
	static QPixmap * s_sampleFilePixmap;
	static QPixmap * s_soundfontFilePixmap;
	static QPixmap * s_vstPluginFilePixmap;
	static QPixmap * s_midiFilePixmap;
	static QPixmap * s_flpFilePixmap;
	static QPixmap * s_unknownFilePixmap;

	QString m_path;
	FileTypes m_type;
	FileHandling m_handling;

} ;


#endif
