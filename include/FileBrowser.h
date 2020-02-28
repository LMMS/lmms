/*
 * FileBrowser.h - include file for FileBrowser
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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


#ifndef FILE_BROWSER_H
#define FILE_BROWSER_H

#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QTreeWidget>


#include "SideBarWidget.h"


class QLineEdit;

class FileItem;
class InstrumentTrack;
class FileBrowserTreeWidget;
class PlayHandle;
class TrackContainer;



class FileBrowser : public SideBarWidget
{
	Q_OBJECT
public:
	FileBrowser( const QString & directories, const QString & filter,
			const QString & title, const QPixmap & pm,
			QWidget * parent, bool dirs_as_items = false, bool recurse = false );
	virtual ~FileBrowser() = default;

private slots:
	void reloadTree( void );
	void expandItems( QTreeWidgetItem * item=NULL, QList<QString> expandedDirs = QList<QString>() );
	// call with item=NULL to filter the entire tree
	bool filterItems( const QString & filter, QTreeWidgetItem * item=NULL );
	void giveFocusToFilter();

private:
	void keyPressEvent( QKeyEvent * ke ) override;

	void addItems( const QString & path );

	FileBrowserTreeWidget * m_fileBrowserTreeWidget;

	QLineEdit * m_filterEdit;

	QString m_directories;
	QString m_filter;

	bool m_dirsAsItems;
	bool m_recurse;

} ;




class FileBrowserTreeWidget : public QTreeWidget
{
	Q_OBJECT
public:
	FileBrowserTreeWidget( QWidget * parent );
	virtual ~FileBrowserTreeWidget() = default;

	//! This method returns a QList with paths (QString's) of all directories
	//! that are expanded in the tree.
	QList<QString> expandedDirs( QTreeWidgetItem * item = nullptr ) const;


protected:
	void contextMenuEvent( QContextMenuEvent * e ) override;
	void mousePressEvent( QMouseEvent * me ) override;
	void mouseMoveEvent( QMouseEvent * me ) override;
	void mouseReleaseEvent( QMouseEvent * me ) override;


private:
	void handleFile( FileItem * fi, InstrumentTrack * it );
	void openInNewInstrumentTrack( TrackContainer* tc );


	bool m_mousePressed;
	QPoint m_pressPos;

	PlayHandle* m_previewPlayHandle;
	QMutex m_pphMutex;

	FileItem * m_contextMenuItem;


private slots:
	void activateListItem( QTreeWidgetItem * item, int column );
	void openInNewInstrumentTrackBBE( void );
	void openInNewInstrumentTrackSE( void );
	void sendToActiveInstrumentTrack( void );
	void updateDirectory( QTreeWidgetItem * item );

} ;




class Directory : public QTreeWidgetItem
{
public:
	Directory( const QString & filename, const QString & path,
						const QString & filter );

	void update( void );

	inline QString fullName( QString path = QString() )
	{
		if( path.isEmpty() )
		{
			path = m_directories[0];
		}
		if( ! path.isEmpty() )
		{
			path += QDir::separator();
		}
		return( QDir::cleanPath( path + text( 0 ) ) +
							QDir::separator() );
	}

	inline void addDirectory( const QString & dir )
	{
		m_directories.push_back( dir );
	}


private:
	void initPixmaps( void );

	bool addItems( const QString & path );


	static QPixmap * s_folderPixmap;
	static QPixmap * s_folderOpenedPixmap;
	static QPixmap * s_folderLockedPixmap;

	QStringList m_directories;
	QString m_filter;

	int m_dirCount;

} ;




class FileItem : public QTreeWidgetItem
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


	FileItem( QTreeWidget * parent, const QString & name,
							const QString & path );
	FileItem( const QString & name, const QString & path );

	QString fullName() const
	{
		return QFileInfo(m_path, text(0)).absoluteFilePath();
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
	static QString extension( const QString & file );


private:
	void initPixmaps( void );
	void determineFileType( void );

	static QPixmap * s_projectFilePixmap;
	static QPixmap * s_presetFilePixmap;
	static QPixmap * s_sampleFilePixmap;
	static QPixmap * s_soundfontFilePixmap;
	static QPixmap * s_vstPluginFilePixmap;
	static QPixmap * s_midiFilePixmap;
	static QPixmap * s_unknownFilePixmap;

	QString m_path;
	FileTypes m_type;
	FileHandling m_handling;

} ;


#endif
