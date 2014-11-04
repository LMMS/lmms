/*
 * EffectSelectDialog.h - dialog to choose effect plugin
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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


namespace Ui { class EffectSelectDialog; }


class EffectSelectDialog : public QDialog
{
	Q_OBJECT
public:
	EffectSelectDialog( QWidget * _parent );
	virtual ~EffectSelectDialog();

	Effect * instantiateSelectedPlugin( EffectChain * _parent );


protected slots:
	void acceptSelection();
	void rowChanged( const QModelIndex &, const QModelIndex & );
	void updateSelection();


private:
	Ui::EffectSelectDialog * ui;

	Plugin::DescriptorList m_pluginDescriptors;
	EffectKeyList m_effectKeys;
	EffectKey m_currentSelection;

	QStandardItemModel m_sourceModel;
	QSortFilterProxyModel m_model;
	QWidget * m_descriptionWidget;

} ;



#endif
