/*
 * PixmapButton.h - declaration of class pixmapButton
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_PIXMAP_BUTTON_H
#define LMMS_GUI_PIXMAP_BUTTON_H

#include <QPixmap>

#include "AutomatableButton.h"

namespace lmms::gui
{


class LMMS_EXPORT PixmapButton : public AutomatableButton
{
	Q_OBJECT
public:
	PixmapButton( QWidget * _parent,
					const QString & _name = QString() );
	~PixmapButton() override = default;

	void setActiveGraphic( const QPixmap & _pm );
	void setInactiveGraphic( const QPixmap & _pm, bool _update = true );

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

signals:
	void doubleClicked();


protected:
	void paintEvent( QPaintEvent * _pe ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void mouseReleaseEvent( QMouseEvent * _me ) override;
	void mouseDoubleClickEvent( QMouseEvent * _me ) override;

private:
	bool isActive() const;

private:
	QPixmap m_activePixmap;
	QPixmap m_inactivePixmap;
	bool	m_pressed;

} ;


} // namespace lmms::gui

#endif // LMMS_GUI_PIXMAP_BUTTON_H
