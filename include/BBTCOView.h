/*
 * BBTCOView.h
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
 

#ifndef BB_TCO_VIEW_H
#define BB_TCO_VIEW_H

#include "BBTCO.h"

#include <QStaticText>


class BBTCOView : public TrackContentObjectView
{
	Q_OBJECT
public:
	BBTCOView( TrackContentObject * _tco, TrackView * _tv );
	virtual ~BBTCOView() = default;


public slots:
	void update() override;

protected slots:
	void openInBBEditor();
	void resetName();
	void changeName();


protected:
	void paintEvent( QPaintEvent * pe ) override;
	void mouseDoubleClickEvent( QMouseEvent * _me ) override;
	void constructContextMenu( QMenu * ) override;


private:
	BBTCO * m_bbTCO;
	QPixmap m_paintPixmap;
	
	QStaticText m_staticTextName;
} ;



#endif