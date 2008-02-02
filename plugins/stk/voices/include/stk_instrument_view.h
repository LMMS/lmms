/*
 * stk_instrument_view.h - base class for stk gui interfaces
 *
 * Copyright (c) 2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * 
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
#ifndef _STK_INSTRUMENT_VIEW_H
#define _STK_INSTRUMENT_VIEW_H

#include <QtCore/QDir>
#include <QtGui/QMessageBox>

#include "config_mgr.h"
#include "instrument_view.h"
#include "led_checkbox.h"
#include "tempo_sync_knob.h"


template <class INSTRUMENT>
class stkInstrumentView: public instrumentView
{
public:
	stkInstrumentView( INSTRUMENT * _instrument, QWidget * _parent );
	virtual ~stkInstrumentView();
	
protected:
	virtual void modelChanged( void );
	
	QWidget * m_topView;
	ledCheckBox * m_monophonic;
	tempoSyncKnob * m_portamento;
	knob * m_bend;
	knob * m_bendRange;
	ledCheckBox * m_velocitySensitiveLPF;
	knob * m_velocitySensitiveQ;
	knob * m_volume;
	knob * m_pan;
	ledCheckBox * m_releaseTriggered;
	ledCheckBox * m_randomizeAttack;
	tempoSyncKnob * m_randomizeLength;
	knob * m_randomizeVelocityAmount;
	knob * m_randomizeFrequencyAmount;
	knob * m_spread;
};


template <class INSTRUMENT>
stkInstrumentView<INSTRUMENT>::stkInstrumentView( INSTRUMENT * _instrument, 
							QWidget * _parent ):
	instrumentView( _instrument, _parent )
{
	bool filesMissing =
			!QDir( configManager::inst()->stkDir() ).exists() ||
			!QFileInfo( configManager::inst()->stkDir() + 
					QDir::separator() +
					"sinewave.raw" ).exists();

	if( filesMissing )
	{
		QMessageBox::information( 0, tr( "Missing files" ),
				tr( "Your Stk-installation seems to be "
					"incomplete. Please make sure "
					"the full Stk-package is installed!" ),
	 QMessageBox::Ok );
	}

	_instrument->setMissingFile( filesMissing );
	
	m_topView = new QWidget( this );
	m_topView->setGeometry( 6, 51, 237, 100 );
	
	m_monophonic = new ledCheckBox( tr( "" ), this );
	m_monophonic->move( 115, 206 );

	m_portamento = new tempoSyncKnob( knobSmall_17, this, 
						tr( "Portamento" ) );
	m_portamento->setHintText( tr( "Portamento:" ) + " ", "ms" );
	m_portamento->move( 19, 218 );
	
	m_bend = new knob( knobSmall_17, this, tr( "Bend" ) );
	m_bend->setHintText( tr( "Bend:" ) + " ", " half steps" );
	m_bend->move( 12, 190 );
	
	m_velocitySensitiveLPF = new ledCheckBox( tr( "" ), this );
	m_velocitySensitiveLPF->move( 13, 165 );
	
	m_velocitySensitiveQ = new knob( knobSmall_17, this, tr( "Q" ) );
	m_velocitySensitiveQ->setHintText( tr( "Q:" ) + " ", "" );
	m_velocitySensitiveQ->move( 74, 164 );
	
	m_bendRange = new knob( knobSmall_17, this, tr( "Range" ) );
	m_bendRange->setHintText( tr( "Range:" ) + " ", 
					" " + tr( "half steps" ) );
	m_bendRange->move( 55, 190 );
	
	m_volume = new knob( knobSmall_17, this, tr( "Volume" ) );
	m_volume->setHintText( tr( "Volume:" ) + " ", "" );
	m_volume->move( 158, 16 );
	
	m_pan = new knob( knobSmall_17, this, tr( "Pan" ) );
	m_pan->setHintText( tr( "Pan:" ) + " ", "" );
	m_pan->move( 181, 16 );
	
	m_releaseTriggered = new ledCheckBox( tr( "" ), this );
	m_releaseTriggered->move( 115, 222 );
	
	m_randomizeAttack = new ledCheckBox( tr( "" ), this );
	m_randomizeAttack->move( 110, 166 );
	
	m_randomizeLength = new tempoSyncKnob( knobSmall_17, this, 
							tr( "Length" ) );
	m_randomizeLength->setHintText( tr( "Length:" ) + " ", "ms" );
	m_randomizeLength->move( 197, 180 );
	
	m_randomizeVelocityAmount = new knob( knobSmall_17, this, 
							tr( "Velocity" ) );
	m_randomizeVelocityAmount->setHintText( tr( "Velocity:" ) + " ", "" );
	m_randomizeVelocityAmount->move( 123, 180 );
	
	m_randomizeFrequencyAmount = new knob( knobSmall_17, this, 
							tr( "Frequency" ) );
	m_randomizeFrequencyAmount->setHintText( tr( "Frequency:" ) + " ", 
						" " + tr( "half steps" ) );
	m_randomizeFrequencyAmount->move( 158, 180 );
	
	m_spread = new knob( knobSmall_17, this, tr( "Spread" ) );
	m_spread->setHintText( tr( "Spread:" ) + " ", "" );
	m_spread->move( 212, 16 );
}




template <class INSTRUMENT>
stkInstrumentView<INSTRUMENT>::~stkInstrumentView()
{
}




template <class INSTRUMENT>
void stkInstrumentView<INSTRUMENT>::modelChanged()
{
	INSTRUMENT * inst = castModel<INSTRUMENT>();
	m_monophonic->setModel( inst->model()->monophonic() );
	m_portamento->setModel( inst->model()->portamento() );
	m_bend->setModel( inst->model()->bend() );
	m_bendRange->setModel( inst->model()->bendRange() );
	m_velocitySensitiveLPF->setModel( inst->model()->velocitySensitiveLPF() );
	m_velocitySensitiveQ->setModel( inst->model()->velocitySensitiveQ() );
	m_volume->setModel( inst->model()->volume() );
	m_pan->setModel( inst->model()->pan() );
	m_releaseTriggered->setModel( inst->model()->releaseTriggered() );
	m_randomizeAttack->setModel( inst->model()->randomizeAttack() );
	m_randomizeLength->setModel( inst->model()->randomizeLength() );
	m_randomizeVelocityAmount->setModel( inst->model()->randomizeVelocityAmount() );
	m_randomizeFrequencyAmount->setModel( inst->model()->randomizeFrequencyAmount() );
	m_spread->setModel( inst->model()->spread() );
}


#endif
