/*
 * effect_select_dialog.h - dialog to choose effect plugin
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QDialog>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QStandardItemModel>

#include "effect_chain.h"
#include "effect.h"


class QLineEdit;
class QListView;
class QScrollArea;


class effectSelectDialog : public QDialog
{
	Q_OBJECT
public:
	effectSelectDialog( QWidget * _parent );
	virtual ~effectSelectDialog();

	effect * instantiateSelectedPlugin( effectChain * _parent );


public slots:
	void setSelection( const effectKey & _selection );
	void selectPlugin( void );
	
private:
	effectKey m_currentSelection;

} ;



class effectListWidget : public QWidget
{
	Q_OBJECT
public:
	effectListWidget( QWidget * _parent );

	virtual ~effectListWidget();
	
	inline effectKey getSelected( void )
	{
		return( m_currentSelection );
	}
	

signals:
	void highlighted( const effectKey & _key );
	void doubleClicked( const effectKey & _key );
	

protected slots:
	void rowChanged( const QModelIndex &, const QModelIndex & );
	void onDoubleClicked( const QModelIndex & );
	void updateSelection( void );


private:
	QVector<plugin::descriptor> m_pluginDescriptors;
	effectKeyList m_effectKeys;
	effectKey m_currentSelection;

	QLineEdit * m_filterEdit;
	QListView * m_pluginList;
	QStandardItemModel m_sourceModel;
	QSortFilterProxyModel m_model;
	QScrollArea * m_scrollArea;
	QWidget * m_descriptionWidget;

} ;

#endif
