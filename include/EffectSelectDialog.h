/*
 * EffectSelectDialog.h - dialog to choose effect plugin
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "EffectChain.h"
#include "Effect.h"


class QLineEdit;
class QListView;
class QScrollArea;


class EffectSelectDialog : public QDialog
{
	Q_OBJECT
public:
	EffectSelectDialog( QWidget * _parent );
	virtual ~EffectSelectDialog();

	Effect * instantiateSelectedPlugin( EffectChain * _parent );


public slots:
	void setSelection( const EffectKey & _selection );
	void selectPlugin();
	
private:
	EffectKey m_currentSelection;

} ;



class EffectListWidget : public QWidget
{
	Q_OBJECT
public:
	EffectListWidget( QWidget * _parent );

	virtual ~EffectListWidget();
	
	inline EffectKey getSelected()
	{
		return( m_currentSelection );
	}
	

signals:
	void highlighted( const EffectKey & _key );
	void doubleClicked( const EffectKey & _key );
	

protected slots:
	void rowChanged( const QModelIndex &, const QModelIndex & );
	void onDoubleClicked( const QModelIndex & );
	void updateSelection();


private:
	QVector<Plugin::Descriptor> m_pluginDescriptors;
	EffectKeyList m_effectKeys;
	EffectKey m_currentSelection;

	QLineEdit * m_filterEdit;
	QListView * m_pluginList;
	QStandardItemModel m_sourceModel;
	QSortFilterProxyModel m_model;
	QScrollArea * m_scrollArea;
	QWidget * m_descriptionWidget;

} ;

#endif
