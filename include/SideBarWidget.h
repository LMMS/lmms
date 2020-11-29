/*
 * SideBarWidget.h - base-class for all side-bar-widgets
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef SIDE_BAR_WIDGET_H
#define SIDE_BAR_WIDGET_H

#include <QPixmap>
#include <QVBoxLayout>
#include <QWidget>
#include <QPushButton>


class SideBarWidget : public QWidget
{
	Q_OBJECT
public:
	SideBarWidget( const QString & _title, const QPixmap & _icon,
							QWidget * _parent );
	virtual ~SideBarWidget();

	inline const QPixmap & icon() const
	{
		return m_icon;
	}
	inline const QString & title() const
	{
		return m_title;
	}

signals:
	void closeButtonClicked();

protected:
	void paintEvent( QPaintEvent * _pe ) override;
	void resizeEvent( QResizeEvent * _re ) override;
	void contextMenuEvent( QContextMenuEvent * ) override
	{
	}

	QWidget * contentParent()
	{
		return m_contents;
	}

	void addContentWidget( QWidget * _w )
	{
		m_layout->addWidget( _w );
	}

	void addContentLayout( QLayout * _l )
	{
		m_layout->addLayout( _l );
	}

private:
	QWidget * m_contents;
	QVBoxLayout * m_layout;
	QString m_title;
	QPixmap m_icon;
	QPushButton * m_closeBtn;
	const QSize m_buttonSize;

} ;

#endif
