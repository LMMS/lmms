/*
 * AutomatableButton.h - class automatableButton, the base for all buttons
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef AUTOMATABLE_BUTTON_H
#define AUTOMATABLE_BUTTON_H

#include <QPushButton>

#include "AutomatableModelView.h"


class automatableButtonGroup;


class LMMS_EXPORT AutomatableButton : public QPushButton, public BoolModelView
{
	Q_OBJECT
public:
	AutomatableButton( QWidget * _parent, const QString & _name
			= QString() );
	virtual ~AutomatableButton();

	inline void setCheckable( bool _on )
	{
		QPushButton::setCheckable( _on );
		model()->setJournalling( _on );
	}

	void modelChanged() override;


public slots:
	virtual void update();
	virtual void toggle();
	virtual void setChecked( bool _on )
	{
		// QPushButton::setChecked is called in update-slot
		model()->setValue( _on );
	}


protected:
	void contextMenuEvent( QContextMenuEvent * _me ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void mouseReleaseEvent( QMouseEvent * _me ) override;


private:
	automatableButtonGroup * m_group;


	friend class automatableButtonGroup;

	using QPushButton::setChecked;
	using QPushButton::isChecked;
} ;



class LMMS_EXPORT automatableButtonGroup : public QWidget, public IntModelView
{
	Q_OBJECT
public:
	automatableButtonGroup( QWidget * _parent, const QString & _name
			= QString() );
	virtual ~automatableButtonGroup();

	void addButton( AutomatableButton * _btn );
	void removeButton( AutomatableButton * _btn );

	void activateButton( AutomatableButton * _btn );

	void modelChanged() override;


private slots:
	void updateButtons();


private:
	QList<AutomatableButton *> m_buttons;

} ;



#endif
