/*
 * PeakController.h - peak-controller class
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
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

#ifndef _PEAK_CONTROLLER_H
#define _PEAK_CONTROLLER_H

#include <QtGui/QWidget>

#include "Model.h"
#include "AutomatableModel.h"
#include "Controller.h"
#include "ControllerDialog.h"

class automatableButtonGroup;
class knob;
class PeakControllerEffect;

typedef QVector<PeakControllerEffect *> PeakControllerEffectVector;


class EXPORT PeakController : public Controller
{
	Q_OBJECT
public:
	PeakController( Model * _parent,
		PeakControllerEffect *_peak_effect = NULL );


	virtual ~PeakController();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );
	virtual QString nodeName() const;

	static void initGetControllerBySetting();
	static PeakController * getControllerBySetting( const QDomElement & _this );

	static PeakControllerEffectVector s_effects;


public slots:
	virtual ControllerDialog * createDialog( QWidget * _parent );
	void handleDestroyedEffect( ); 

protected:
	// The internal per-controller get-value function
	virtual float value( int _offset );

	PeakControllerEffect * m_peakEffect;

	friend class PeakControllerDialog;

private:
	//backward compatibility for <= 0.4.15
	static int m_getCount;
	static int m_loadCount;
	static bool m_buggedFile;
} ;



class PeakControllerDialog : public ControllerDialog
{
	Q_OBJECT
public:
	PeakControllerDialog( Controller * _controller, QWidget * _parent );
	virtual ~PeakControllerDialog();

protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void modelChanged();

	PeakController * m_peakController;

} ;

#endif
