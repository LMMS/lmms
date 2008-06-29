/*
 * peak_controller.h - peak-controller class
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#ifndef _PEAK_CONTROLLER_H
#define _PEAK_CONTROLLER_H

#include <QtGui/QWidget>

#include "mv_base.h"
#include "automatable_model.h"
#include "controller.h"
#include "controller_dialog.h"

class automatableButtonGroup;
class knob;
class peakControllerEffect;

typedef QVector<peakControllerEffect *> peakControllerEffectVector;


class EXPORT peakController : public controller
{
	Q_OBJECT
public:
	peakController( model * _parent,
		peakControllerEffect *_peak_effect = NULL );


	virtual ~peakController();

	virtual QString publicName() const
	{
		return "Peak Controller";
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );
	virtual QString nodeName( void ) const;

	static peakControllerEffectVector s_effects;
	static int s_lastEffectId;

public slots:
	virtual controllerDialog * createDialog( QWidget * _parent );

protected:

	// The internal per-controller get-value function
	virtual float value( int _offset );

	/*
slots:
	void trigger();

	*/

	peakControllerEffect * m_peakEffect;

protected slots:

	friend class peakControllerDialog;
};



class peakControllerDialog : public controllerDialog
{
	Q_OBJECT
public:
	peakControllerDialog( controller * _controller, QWidget * _parent );
	virtual ~peakControllerDialog();

public slots:
	//void editControls( void );
	//void deletePlugin( void );
	//void displayHelp( void );
	//void closeEffects( void );

	
signals:


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void modelChanged( void );

	peakController * m_peakController;

} ;

#endif
