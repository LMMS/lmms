/*
 * LcdSpinBox.h - class LcdSpinBox, an improved QLCDNumber
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef LCD_SPINBOX_H
#define LCD_SPINBOX_H

#include "LcdWidget.h"
#include "AutomatableModelView.h"


class LMMS_EXPORT LcdSpinBox : public LcdWidget, public IntModelView
{
	Q_OBJECT
public:
	LcdSpinBox( int numDigits, QWidget* parent, const QString& name = QString() );

	LcdSpinBox( int numDigits, const QString& style, QWidget* parent, const QString& name = QString() );

	virtual ~LcdSpinBox() = default;

	void modelChanged() override
	{
		ModelView::modelChanged();
		update();
	}

	/*! Sets an offset which is always added to value of model so we can
	    display values in a user-friendly way if they internally start at 0 */
	void setDisplayOffset( int offset )
	{
		m_displayOffset = offset;
	}

	/*! \brief Returns internal offset for displaying values */
	int displayOffset() const
	{
		return m_displayOffset;
	}


public slots:
	virtual void update();


protected:
	void contextMenuEvent( QContextMenuEvent * _me ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void mouseMoveEvent( QMouseEvent * _me ) override;
	void mouseReleaseEvent( QMouseEvent * _me ) override;
	void wheelEvent( QWheelEvent * _we ) override;
	void mouseDoubleClickEvent( QMouseEvent * _me ) override;

private:
	bool m_mouseMoving;
	QPoint m_origMousePos;
	int m_displayOffset;
	void enterValue();

signals:
	void manualChange();

} ;

typedef IntModel LcdSpinBoxModel;

#endif
