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

#include <QCheckBox>
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
	/**
		Create a file browser side bar widget
		@param directories '*'-separated list of directories to search for.
			If a directory of factory files should be in the list it
			must be the last one (for the factory files delimiter to work)
		@param filter Filter as used in QDir::match
		@param recurse *to be documented*
	*/
	FileBrowser( const QString & directories, const QString & filter,
			const QString & title, const QPixmap & pm,
			QWidget * parent, bool dirs_as_items = false, bool recurse = false,
			const QString& userDir = "",
			const QString& factoryDir = "");

	virtual ~FileBrowser() = default;

private slots:
	void reloadTree( void );
	void expandItems( QTreeWidgetItem * item=nullptr, QList<QString> expandedDirs = QList<QString>() );
	// call with item=NULL to filter the entire tree
	bool filterItems( const QString & filter, QTreeWidgetItem * item=nullptr );
	void giveFocusToFilter();

private:
	void keyPressEvent( QKeyEvent * ke ) override;

	void addItems( const QString & path );

	FileBrowserTreeWidget * m_fileBrowserTreeWidget;

	QLineEdit * m_filterEdit;

	QString m_directories; //!< Directories to search, split with '*'
	QString m_filter; //!< Filter as used in QDir::match()

	bool m_dirsAsItems;
	bool m_recurse;

	void addContentCheckBox();
	QCheckBox* m_showUserContent = nullptr;
	QCheckBox* m_showFactoryContent = nullptr;
	QString m_userDir;
	QString m_factoryDir;
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
	void keyPressEvent( QKeyEvent * ke ) override;
	void keyReleaseEvent( QKeyEvent * ke ) override;
	void hideEvent( QHideEvent * he ) override;
	void focusOutEvent( QFocusEvent * fe ) override;


private:
	//! Start a preview of a file item
	void previewFileItem(FileItem* file);
	//! If a preview is playing, stop it.
	void stopPreview();

	void handleFile( FileItem * fi, InstrumentTrack * it );
	void openInNewInstrumentTrack( TrackContainer* tc, FileItem* item );


	bool m_mousePressed;
	QPoint m_pressPos;

	//! This should only be accessed or modified when m_pphMutex is held
	PlayHandle* m_previewPlayHandle;
	QMutex m_pphMutex;

	QList<QAction*> getContextActions(FileItem* item, bool songEditor);


private slots:
	void activateListItem( QTreeWidgetItem * item, int column );
	void openInNewInstrumentTrack( FileItem* item, bool songEditor );
	bool openInNewSampleTrack( FileItem* item );
	void sendToActiveInstrumentTrack( FileItem* item );
	void updateDirectory( QTreeWidgetItem * item );
	void openContainingFolder( FileItem* item );

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

	//! Directories that lead here
	//! Initially, this is just set to the current path of a directory
	//! If, however, you have e.g. 'TripleOscillator/xyz' in two of the
	//! file browser's search directories 'a' and 'b', this will have two
	//! entries 'a/TripleOscillator' and 'b/TripleOscillator'
	//! and 'xyz' in the tree widget
	QStringList m_directories;
	//! Filter as used in QDir::match()
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

	inline bool isTrack( void ) const
	{
		return m_handling == LoadAsPreset || m_handling == LoadByPlugin;
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
