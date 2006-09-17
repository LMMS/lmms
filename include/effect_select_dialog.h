/*
 * effect_select_dialog.h - dialog to choose effect plugin
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _EFFECT_SELECT_DIALOG_H
#define _EFFECT_SELECT_DIALOG_H

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QDialog>
#include <Qt3Support/Q3ListBox>
#define QListBoxItem Q3ListBoxItem

#else

#include <qdialog.h>
#include <qlistbox.h>

#define Q3ListBox QListBox

#endif

#include "engine.h"
#include "effect.h"


class effectSelectDialog : public QDialog, public engineObject
{
	Q_OBJECT
public:
	effectSelectDialog( QWidget * _parent, engine * _engine );
	virtual ~effectSelectDialog();

	effect * instantiateSelectedPlugin( void );

public slots:
	void showPorts( void );
	void setSelection( const effectKey & _selection );
	void selectPlugin( void );
	
private:
	effectKey m_currentSelection;

} ;



class effectList : public QWidget, public engineObject
{
	Q_OBJECT
public:
	effectList( QWidget * _parent, engine * _engine );

	virtual ~effectList();
	
	inline effectKey getSelected( void )
	{
		return( m_currentSelection );
	}
	

signals:
	void highlighted( const effectKey & _key );
	void addPlugin( const effectKey & _key );
	void doubleClicked( const effectKey & _key );
	

protected slots:
	void onHighlighted( int _plugin );
	void onAddButtonReleased();
	void onDoubleClicked( QListBoxItem * _item );
	

private:
	vvector<plugin::descriptor> m_pluginDescriptors;
	effectKeyList m_effectKeys;
	effectKey m_currentSelection;
	
	Q3ListBox * m_pluginList;
	QWidget * m_descriptionWidgetParent;
	QWidget * m_descriptionWidget;

} ;

#endif
