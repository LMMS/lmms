/*
 * ladspa_description.h - LADSPA plugin description
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#ifndef _LADSPA_DESCRIPTION_H
#define _LADSPA_DESCRIPTION_H


#include <QtGui/QWidget>

#include "ladspa_manager.h"


class QListWidgetItem;
class QScrollArea;


class ladspaDescription : public QWidget
{
	Q_OBJECT
public:
	ladspaDescription( QWidget * _parent, ladspaPluginType _type );
	virtual ~ladspaDescription();


signals:
	void doubleClicked( const ladspa_key_t & );


private:
	QScrollArea * m_scrollArea;

	QList<ladspa_key_t> m_pluginKeys;
	ladspa_key_t m_currentSelection;

	void update( const ladspa_key_t & _key );


private slots:
	void rowChanged( int _pluginIndex );
	void onDoubleClicked( QListWidgetItem * _item );

} ;




#endif
