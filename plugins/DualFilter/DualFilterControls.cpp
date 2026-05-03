/*
 * DualFilterControls.cpp - controls for dual filter effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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



#include "BasicFilters.h"
#include "DualFilterControls.h"
#include "DualFilter.h"
#include "embed.h"
#include "Engine.h"

namespace lmms
{


DualFilterControls::DualFilterControls( DualFilterEffect* effect ) :
	EffectControls( effect ),
	m_effect( effect ),

	m_enabled1Model( true, this, tr( "Filter 1 enabled" ) ),
	m_filter1Model( this, tr( "Filter 1 type" ) ),
	m_cut1Model( 7000.0f, 1.0f, 20000.0f, 1.0f, this, tr( "Cutoff frequency 1" ) ),
	m_res1Model(0.5f, BasicFilters<>::minQ(), 10.f, 0.01f, this, tr("Q/Resonance 1")),
	m_gain1Model( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Gain 1" ) ),

	m_mixModel( 0.0f, -1.0f, 1.0f, 0.01f, this, tr( "Mix" ) ),

	m_enabled2Model( true, this, tr( "Filter 2 enabled" ) ),
	m_filter2Model( this, tr( "Filter 2 type" ) ),
	m_cut2Model( 7000.0f, 1.0f, 20000.0f, 1.0f, this, tr( "Cutoff frequency 2" ) ),
	m_res2Model(0.5f, BasicFilters<>::minQ(), 10.f, 0.01f, this, tr("Q/Resonance 2")),
	m_gain2Model( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Gain 2" ) )
{
	m_filter1Model.addItem( tr( "Low-pass" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filter1Model.addItem( tr( "Hi-pass" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filter1Model.addItem( tr( "Band-pass csg" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filter1Model.addItem( tr( "Band-pass czpg" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filter1Model.addItem( tr( "Notch" ), std::make_unique<PixmapLoader>( "filter_notch" ) );
	m_filter1Model.addItem( tr( "All-pass" ), std::make_unique<PixmapLoader>( "filter_ap" ) );
	m_filter1Model.addItem( tr( "Moog" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filter1Model.addItem( tr( "2x Low-pass" ), std::make_unique<PixmapLoader>( "filter_2lp" ) );
	m_filter1Model.addItem( tr( "RC Low-pass 12 dB/oct" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filter1Model.addItem( tr( "RC Band-pass 12 dB/oct" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filter1Model.addItem( tr( "RC High-pass 12 dB/oct" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filter1Model.addItem( tr( "RC Low-pass 24 dB/oct" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filter1Model.addItem( tr( "RC Band-pass 24 dB/oct" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filter1Model.addItem( tr( "RC High-pass 24 dB/oct" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filter1Model.addItem( tr( "Vocal Formant" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filter1Model.addItem( tr( "2x Moog" ), std::make_unique<PixmapLoader>( "filter_2lp" ) );
	m_filter1Model.addItem( tr( "SV Low-pass" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filter1Model.addItem( tr( "SV Band-pass" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filter1Model.addItem( tr( "SV High-pass" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filter1Model.addItem( tr( "SV Notch" ), std::make_unique<PixmapLoader>( "filter_notch" ) );
	m_filter1Model.addItem( tr( "Fast Formant" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filter1Model.addItem( tr( "Tripole" ), std::make_unique<PixmapLoader>( "filter_lp" ) );

	m_filter2Model.addItem( tr( "Low-pass" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filter2Model.addItem( tr( "Hi-pass" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filter2Model.addItem( tr( "Band-pass csg" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filter2Model.addItem( tr( "Band-pass czpg" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filter2Model.addItem( tr( "Notch" ), std::make_unique<PixmapLoader>( "filter_notch" ) );
	m_filter2Model.addItem( tr( "All-pass" ), std::make_unique<PixmapLoader>( "filter_ap" ) );
	m_filter2Model.addItem( tr( "Moog" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filter2Model.addItem( tr( "2x Low-pass" ), std::make_unique<PixmapLoader>( "filter_2lp" ) );
	m_filter2Model.addItem( tr( "RC Low-pass 12 dB/oct" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filter2Model.addItem( tr( "RC Band-pass 12 dB/oct" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filter2Model.addItem( tr( "RC High-pass 12 dB/oct" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filter2Model.addItem( tr( "RC Low-pass 24 dB/oct" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filter2Model.addItem( tr( "RC Band-pass 24 dB/oct" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filter2Model.addItem( tr( "RC High-pass 24 dB/oct" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filter2Model.addItem( tr( "Vocal Formant" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filter2Model.addItem( tr( "2x Moog" ), std::make_unique<PixmapLoader>( "filter_2lp" ) );
	m_filter2Model.addItem( tr( "SV Low-pass" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filter2Model.addItem( tr( "SV Band-pass" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filter2Model.addItem( tr( "SV High-pass" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filter2Model.addItem( tr( "SV Notch" ), std::make_unique<PixmapLoader>( "filter_notch" ) );
	m_filter2Model.addItem( tr( "Fast Formant" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filter2Model.addItem( tr( "Tripole" ), std::make_unique<PixmapLoader>( "filter_lp" ) );

	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ), this, SLOT( updateFilters() ) );
}



void DualFilterControls::updateFilters()
{
	// swap filters to new ones
	
	delete m_effect->m_filter1;
	delete m_effect->m_filter2;
	m_effect->m_filter1 = new BasicFilters<2>( Engine::audioEngine()->outputSampleRate() );
	m_effect->m_filter2 = new BasicFilters<2>( Engine::audioEngine()->outputSampleRate() );
	
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


} // namespace lmms
