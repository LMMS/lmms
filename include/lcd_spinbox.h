/*
 * lcd_spinbox.h - class lcdSpinBox, an improved QLCDNumber
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008-2009 Paul Giblock <pgib/at/users.sourceforge.net>
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


#ifndef _LCD_SPINBOX_H
#define _LCD_SPINBOX_H

#include <QtCore/QMap>
#include <QtGui/QWidget>

#include "AutomatableModelView.h"


class EXPORT lcdSpinBox : public QWidget, public IntModelView
{
	Q_OBJECT
public:
	lcdSpinBox( int _num_digits, QWidget * _parent, const QString & _name =
			QString::null );

	lcdSpinBox( int _num_digits, const QString & _lcd_style, 
			QWidget * _parent, const QString & _name = QString::null );

	virtual ~lcdSpinBox();

	void setLabel( const QString & _txt );

	inline void addTextForValue( int _val, const QString & _text )
	{
		m_textForValue[_val] = _text;
		update();
	}

	virtual void modelChanged()
	{
		ModelView::modelChanged();
		update();
	}


public slots:
	virtual void setEnabled( bool _on );
	virtual void setMarginWidth( int _width );
	virtual void update();


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void wheelEvent( QWheelEvent * _we );
	virtual void paintEvent( QPaintEvent * _me );
	
	virtual void updateSize();

private:

	static const int charsPerPixmap = 12;

	QMap<int, QString> m_textForValue;

	QString m_display;

	QString m_label;
	QPixmap * m_lcdPixmap;

	int m_cellWidth;
	int m_cellHeight;
	int m_numDigits;
	int m_marginWidth;

	QPoint m_origMousePos;


signals:
	void manualChange();

} ;

typedef IntModel lcdSpinBoxModel;

#endif
