/* vibed_strings.h - 
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/yahoo/com>
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
#ifndef _VIBED_STRINGS_H
#define _VIBED_STRINGS_H

#include "instrument.h"
#include "sample_buffer.h"
#include "graph.h"
#include "pixmap_button.h"
#include "led_checkbox.h"
#include "impulse_editor.h"
#include "lcd_spinbox.h"
#include "nine_button_selector.h"

class knob;
class notePlayHandle;
class volumeKnob;


class vibed : public instrument
{
	Q_OBJECT
			
public:
	vibed( instrumentTrack * _channel_track );
	virtual ~vibed();

	virtual void FASTCALL playNote( notePlayHandle * _n,
						bool _try_parallelizing );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
				QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

public slots:
	void showString( Uint8 _string );
	void contextMenuEvent( QContextMenuEvent * );
	void displayHelp( void );
		
private:
	QList<knob*> m_pickKnobs;
	QList<knob*> m_pickupKnobs;
	QList<knob*> m_stiffnessKnobs;
	QList<volumeKnob*> m_volumeKnobs;
	QList<knob*> m_panKnobs;
	QList<knob*> m_detuneKnobs;
	QList<knob*> m_randomKnobs;
	QList<knob*> m_lengthKnobs;
	QList<impulseEditor*> m_editors;
	QList<ledCheckBox*> m_impulses;
	QList<nineButtonSelector*> m_harmonics;
	
	knob * m_pickKnob;
	knob * m_pickupKnob;
	knob * m_stiffnessKnob;
	volumeKnob * m_volumeKnob;
	knob * m_panKnob;
	knob * m_detuneKnob;
	knob * m_randomKnob;
	knob * m_lengthKnob;
	impulseEditor * m_editor;
	
	nineButtonSelector * m_stringSelector;
	nineButtonSelector * m_harmonic;
	
	ledCheckBox * m_impulse;
	
	int m_sampleLength;
} ;


#endif
