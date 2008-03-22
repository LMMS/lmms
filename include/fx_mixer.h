/*
 * fx_mixer.h - effect-mixer for LMMS
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _FX_MIXER_H
#define _FX_MIXER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtGui/QWidget>

#include "types.h"
#include "mv_base.h"
#include "mixer.h"
#include "journalling_object.h"


const int NumFxChannels = 64;

class fader;
class fxLine;
class effectRackView;
class pixmapButton;
struct fxChannel;


class fxMixer : public journallingObject, public model
{
public:
	fxMixer();
	virtual ~fxMixer();

	void mixToChannel( const surroundSampleFrame * _buf, fx_ch_t _ch );
	void processChannel( fx_ch_t _ch );

	void prepareMasterMix( void );
	const surroundSampleFrame * masterMix( void );


	void clear( void );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const
	{
		return( "fxmixer" );
	}


private:
	fxChannel * m_fxChannels[NumFxChannels+1];	// +1 = master


	friend class mixerWorkerThread;
	friend class fxMixerView;

} ;


class fxMixerView : public QWidget, public modelView
{
	Q_OBJECT
public:
	fxMixerView();
	virtual ~fxMixerView();

	fxLine * currentFxLine( void )
	{
		return( m_currentFxLine );
	}
	void setCurrentFxLine( fxLine * _line );

	void clear( void );


private slots:
	void updateFaders( void );


private:
	struct fxChannelView
	{
		fxLine * m_fxLine;
		effectRackView * m_rackView;
		pixmapButton * m_muteButton;
		pixmapButton * m_soloButton;
		fader * m_fader;
	} ;

	fxChannelView m_fxChannelViews[NumFxChannels+1];

	QWidget * m_fxRackArea;
	fxLine * m_currentFxLine;

} ;

#endif
