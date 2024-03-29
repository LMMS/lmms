/*
 * NStateButton.h - declaration of class nStateButton
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_NSTATE_BUTTON_H
#define LMMS_GUI_NSTATE_BUTTON_H

#include <QPixmap>
#include <QVector>
#include <QPair>

#include "ToolButton.h"

namespace lmms::gui
{


class NStateButton : public ToolButton
{
	Q_OBJECT
public:
	NStateButton( QWidget * _parent );
	~NStateButton() override;
	void addState( const QPixmap & _pixmap, const QString & _tooltip = "" );

	inline void setGeneralToolTip( const QString & _tooltip )
	{
		m_generalToolTip = _tooltip;
	}

	inline int state() const
	{
		return( m_curState );
	}


public slots:
	void changeState(int state);

signals:
	void changedState(int state);

protected:
	void mousePressEvent(QMouseEvent* me) override;

private:
	QVector<QPair<QPixmap, QString>> m_states;
	QString m_generalToolTip;

	int m_curState;
};

} // namespace lmms::gui

#endif // LMMS_GUI_NSTATE_BUTTON_H
