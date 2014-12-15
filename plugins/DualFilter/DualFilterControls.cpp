/*
 * DualFilterControls.cpp - controls for dual filter effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtXml/QDomElement>

#include "DualFilterControls.h"
#include "DualFilter.h"
#include "engine.h"
#include "song.h"
#include "basic_filters.h"
#include "embed.h"

DualFilterControls::DualFilterControls( DualFilterEffect* effect ) :
	EffectControls( effect ),
	m_effect( effect ),

	m_enabled1Model( true, this, tr( "Filter 1 enabled" ) ),
	m_filter1Model( this, tr( "Filter 1 type" ) ),
	m_cut1Model( 7000.0f, 1.0f, 20000.0f, 1.0f, this, tr( "Cutoff 1 frequency" ) ),
	m_res1Model( 0.5, basicFilters<0>::minQ(), 10.0, 0.01, this, tr( "Q/Resonance 1" ) ),
	m_gain1Model( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Gain 1" ) ),

	m_mixModel( 0.0f, -1.0f, 1.0f, 0.01f, this, tr( "Mix" ) ),

	m_enabled2Model( true, this, tr( "Filter 2 enabled" ) ),
	m_filter2Model( this, tr( "Filter 2 type" ) ),
	m_cut2Model( 7000.0f, 1.0f, 20000.0f, 1.0f, this, tr( "Cutoff 2 frequency" ) ),
	m_res2Model( 0.5, basicFilters<0>::minQ(), 10.0, 0.01, this, tr( "Q/Resonance 2" ) ),
	m_gain2Model( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Gain 2" ) )
{
	m_filter1Model.addItem( tr( "LowPass" ), new PixmapLoader( "filter_lp" ) );
	m_filter1Model.addItem( tr( "HiPass" ), new PixmapLoader( "filter_hp" ) );
	m_filter1Model.addItem( tr( "BandPass csg" ), new PixmapLoader( "filter_bp" ) );
	m_filter1Model.addItem( tr( "BandPass czpg" ), new PixmapLoader( "filter_bp" ) );
	m_filter1Model.addItem( tr( "Notch" ), new PixmapLoader( "filter_notch" ) );
	m_filter1Model.addItem( tr( "Allpass" ), new PixmapLoader( "filter_ap" ) );
	m_filter1Model.addItem( tr( "Moog" ), new PixmapLoader( "filter_lp" ) );
	m_filter1Model.addItem( tr( "2x LowPass" ), new PixmapLoader( "filter_2lp" ) );
	m_filter1Model.addItem( tr( "RC LowPass 12dB" ), new PixmapLoader( "filter_lp" ) );
	m_filter1Model.addItem( tr( "RC BandPass 12dB" ), new PixmapLoader( "filter_bp" ) );
	m_filter1Model.addItem( tr( "RC HighPass 12dB" ), new PixmapLoader( "filter_hp" ) );
	m_filter1Model.addItem( tr( "RC LowPass 24dB" ), new PixmapLoader( "filter_lp" ) );
	m_filter1Model.addItem( tr( "RC BandPass 24dB" ), new PixmapLoader( "filter_bp" ) );
	m_filter1Model.addItem( tr( "RC HighPass 24dB" ), new PixmapLoader( "filter_hp" ) );
	m_filter1Model.addItem( tr( "Vocal Formant Filter" ), new PixmapLoader( "filter_hp" ) );

	m_filter2Model.addItem( tr( "LowPass" ), new PixmapLoader( "filter_lp" ) );
	m_filter2Model.addItem( tr( "HiPass" ), new PixmapLoader( "filter_hp" ) );
	m_filter2Model.addItem( tr( "BandPass csg" ), new PixmapLoader( "filter_bp" ) );
	m_filter2Model.addItem( tr( "BandPass czpg" ), new PixmapLoader( "filter_bp" ) );
	m_filter2Model.addItem( tr( "Notch" ), new PixmapLoader( "filter_notch" ) );
	m_filter2Model.addItem( tr( "Allpass" ), new PixmapLoader( "filter_ap" ) );
	m_filter2Model.addItem( tr( "Moog" ), new PixmapLoader( "filter_lp" ) );
	m_filter2Model.addItem( tr( "2x LowPass" ), new PixmapLoader( "filter_2lp" ) );
	m_filter2Model.addItem( tr( "RC LowPass 12dB" ), new PixmapLoader( "filter_lp" ) );
	m_filter2Model.addItem( tr( "RC BandPass 12dB" ), new PixmapLoader( "filter_bp" ) );
	m_filter2Model.addItem( tr( "RC HighPass 12dB" ), new PixmapLoader( "filter_hp" ) );
	m_filter2Model.addItem( tr( "RC LowPass 24dB" ), new PixmapLoader( "filter_lp" ) );
	m_filter2Model.addItem( tr( "RC BandPass 24dB" ), new PixmapLoader( "filter_bp" ) );
	m_filter2Model.addItem( tr( "RC HighPass 24dB" ), new PixmapLoader( "filter_hp" ) );
	m_filter2Model.addItem( tr( "Vocal Formant Filter" ), new PixmapLoader( "filter_hp" ) );

	connect( engine::mixer(), SIGNAL( sampleRateChanged() ), this, SLOT( updateFilters() ) );
}



void DualFilterControls::updateFilters()
{
	// swap filters to new ones
	
	delete m_effect->m_filter1;
	delete m_effect->m_filter2;
	m_effect->m_filter1 = new basicFilters<2>( engine::mixer()->processingSampleRate() );
	m_effect->m_filter2 = new basicFilters<2>( engine::mixer()->processingSampleRate() );
	
	// flag filters as needing recalculation
	
	m_effect->m_filter1changed = true;
	m_effect->m_filter2changed = true;
}



void DualFilterControls::loadSettings( const QDomElement& _this )
{
	m_enabled1Model.loadSettings( _this, "enabled1" );
	m_filter1Model.loadSettings( _this, "filter1" );
	m_cut1Model.loadSettings( _this, "cut1" );
	m_res1Model.loadSettings( _this, "res1" );
	m_gain1Model.loadSettings( _this, "gain1" );

	m_mixModel.loadSettings( _this, "mix" );

	m_enabled2Model.loadSettings( _this, "enabled2" );
	m_filter2Model.loadSettings( _this, "filter2" );
	m_cut2Model.loadSettings( _this, "cut2" );
	m_res2Model.loadSettings( _this, "res2" );
	m_gain2Model.loadSettings( _this, "gain2" );
}




void DualFilterControls::saveSettings( QDomDocument& _doc, QDomElement& _this )
{
	m_enabled1Model.saveSettings( _doc, _this, "enabled1" );
	m_filter1Model.saveSettings( _doc, _this, "filter1" );
	m_cut1Model.saveSettings( _doc, _this, "cut1" );
	m_res1Model.saveSettings( _doc, _this, "res1" );
	m_gain1Model.saveSettings( _doc, _this, "gain1" );

	m_mixModel.saveSettings( _doc, _this, "mix" );

	m_enabled2Model.saveSettings( _doc, _this, "enabled2" );
	m_filter2Model.saveSettings( _doc, _this, "filter2" );
	m_cut2Model.saveSettings( _doc, _this, "cut2" );
	m_res2Model.saveSettings( _doc, _this, "res2" );
	m_gain2Model.saveSettings( _doc, _this, "gain2" );
}



#include "moc_DualFilterControls.cxx"

