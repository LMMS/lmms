/*
 * automatable_button.h - class automatableButton, the base for all buttons
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUTOMATABLE_BUTTON_H
#define _AUTOMATABLE_BUTTON_H

#include <QtGui/QPushButton>

#include "AutomatableModelView.h"


class automatableButtonGroup;


class EXPORT automatableButton : public QPushButton, public BoolModelView
{
	Q_OBJECT
public:
	automatableButton( QWidget * _parent, const QString & _name 
			= QString::null );
	virtual ~automatableButton();

	inline void setCheckable( bool _on )
	{
		QPushButton::setCheckable( _on );
		model()->setJournalling( _on );
	}

	virtual void modelChanged();


public slots:
	virtual void update();
	virtual void toggle();
	virtual void setChecked( bool _on )
	{
		// QPushButton::setChecked is called in update-slot
		model()->setValue( _on );
	}


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );


private:
	automatableButtonGroup * m_group;


	friend class automatableButtonGroup;

	using QPushButton::setChecked;
	using QPushButton::isChecked;
} ;



class EXPORT automatableButtonGroup : public QWidget, public IntModelView
{
	Q_OBJECT
public:
	automatableButtonGroup( QWidget * _parent, const QString & _name
			= QString::null );
	virtual ~automatableButtonGroup();

	void addButton( automatableButton * _btn );
	void removeButton( automatableButton * _btn );

	void activateButton( automatableButton * _btn );

	virtual void modelChanged();


private slots:
	void updateButtons();


private:
	QList<automatableButton *> m_buttons;

} ;



#endif
