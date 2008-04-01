/*
 * effect_view.h - view-component for an effect
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2007-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _EFFECT_VIEW_H
#define _EFFECT_VIEW_H

#include "automatable_model.h"
#include "plugin_view.h"
#include "effect.h"

class QGroupBox;
class QLabel;
class QPushButton;
class QMdiSubWindow;

class audioPort;
class effectControlDialog;
class knob;
class ledCheckBox;
class tempoSyncKnob;
class track;


class effectView : public pluginView
{
	Q_OBJECT
public:
	effectView( effect * _model, QWidget * _parent );
	virtual ~effectView();
	
	inline effect * getEffect( void )
	{
		return( castModel<effect>() );
	}
	inline const effect * getEffect( void ) const
	{
		return( castModel<effect>() );
	}


public slots:
	void editControls( void );
	void moveUp( void );
	void moveDown( void );
	void deletePlugin( void );
	void displayHelp( void );
	void closeEffects( void );

	
signals:
	void moveUp( effectView * _plugin );
	void moveDown( effectView * _plugin );
	void deletePlugin( effectView * _plugin );


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void modelChanged( void );


private:
	QPixmap m_bg;
	ledCheckBox * m_bypass;
	knob * m_wetDry;
	tempoSyncKnob * m_autoQuit;
	knob * m_gate;
	QMdiSubWindow * m_subWindow;
	effectControlDialog * m_controlView;
	bool m_show;

} ;

#endif
