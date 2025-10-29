/*
 * eqcontrols.cpp - defination of EqControls class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#include "EqControls.h"


#include "EqControlsDialog.h"
#include "EqEffect.h"


namespace lmms
{


EqControls::EqControls( EqEffect *effect ) :
	EffectControls( effect ),
	m_effect( effect ),
	m_inGainModel(0.f, -60.f, 20.f, 0.01f, this, tr("Input gain")),
	m_outGainModel(-0.f, -60.f, 20.f, 0.01f, this, tr("Output gain")),
	m_lowShelfGainModel( 0.f, -18, 18, 0.001f, this, tr("Low-shelf gain")),
	m_para1GainModel(0.f, -18, 18, 0.001f, this, tr("Peak 1 gain")),
	m_para2GainModel(0.f, -18, 18, 0.001f, this, tr("Peak 2 gain")),
	m_para3GainModel(0.f, -18, 18, 0.001f, this, tr("Peak 3 gain")),
	m_para4GainModel(0.f, -18, 18, 0.001f, this, tr("Peak 4 gain")),
	m_highShelfGainModel(0.f, -18, 18, 0.001f, this, tr("High-shelf gain")),
	m_hpResModel(0.707f,0.003f, 10.f, 0.001f, this, tr("HP res")),
	m_lowShelfResModel(0.707f, 0.55f, 10.f, 0.001f, this, tr("Low-shelf res")),
	m_para1BwModel(0.3f, 0.1f, 4, 0.001f, this, tr("Peak 1 BW")),
	m_para2BwModel(0.3f, 0.1f, 4, 0.001f, this, tr("Peak 2 BW")),
	m_para3BwModel(0.3f, 0.1f, 4, 0.001f, this, tr("Peak 3 BW")),
	m_para4BwModel(0.3f, 0.1f, 4, 0.001f, this, tr("Peak 4 BW")),
	m_highShelfResModel(0.707f, 0.55f, 10.f, 0.001f, this, tr("High-shelf res")),
	m_lpResModel(0.707f,0.003f, 10.f, 0.001f, this, tr("LP res")),
	m_hpFeqModel(31.f, 20.f, 20000, 0.001f, this, tr("HP freq")),
	m_lowShelfFreqModel(80.f, 20.f, 20000, 0.001f, this, tr("Low-shelf freq")),
	m_para1FreqModel(120.f, 20.f, 20000, 0.001f, this, tr("Peak 1 freq")),
	m_para2FreqModel(250.f, 20.f, 20000, 0.001f, this, tr("Peak 2 freq")),
	m_para3FreqModel(2000.f, 20.f, 20000, 0.001f, this, tr("Peak 3 freq")),
	m_para4FreqModel(4000.f, 20.f, 20000, 0.001f, this, tr("Peak 4 freq")),
	m_highShelfFreqModel(12000.f, 20.f, 20000, 0.001f, this, tr("High-shelf freq")),
	m_lpFreqModel(18000.f, 20.f, 20000, 0.001f, this, tr("LP freq")),
	m_hpActiveModel( false, this , tr( "HP active" ) ),
	m_lowShelfActiveModel( false, this , tr( "Low-shelf active" ) ),
	m_para1ActiveModel( false, this , tr( "Peak 1 active" ) ),
	m_para2ActiveModel( false, this , tr( "Peak 2 active" ) ),
	m_para3ActiveModel( false, this , tr( "Peak 3 active" ) ),
	m_para4ActiveModel( false, this , tr( "Peak 4 active" ) ),
	m_highShelfActiveModel( false, this , tr( "High-shelf active" ) ),
	m_lpActiveModel( false, this , tr( "LP active" ) ),
	m_lp12Model( false, this , tr( "LP 12" ) ),
	m_lp24Model( false, this , tr( "LP 24" ) ),
	m_lp48Model( false, this , tr( "LP 48" ) ),
	m_hp12Model( false, this , tr( "HP 12" ) ),
	m_hp24Model( false, this , tr( "HP 24" ) ),
	m_hp48Model( false, this , tr( "HP 48" ) ),
	m_lpTypeModel( 0,0,2, this, tr( "Low-pass type" ) ) ,
	m_hpTypeModel( 0,0,2, this, tr( "High-pass type" ) ),
	m_analyseInModel( true, this , tr( "Analyse IN" ) ),
	m_analyseOutModel( true, this, tr( "Analyse OUT" ) )
{
	m_hpFeqModel.setScaleLogarithmic( true );
	m_lowShelfFreqModel.setScaleLogarithmic( true );
	m_para1FreqModel.setScaleLogarithmic( true );
	m_para2FreqModel.setScaleLogarithmic( true );
	m_para3FreqModel.setScaleLogarithmic( true );
	m_para4FreqModel.setScaleLogarithmic( true );
	m_highShelfFreqModel.setScaleLogarithmic( true );
	m_lpFreqModel.setScaleLogarithmic( true );
	m_para1GainModel.setScaleLogarithmic( true );
	m_inPeakL = 0;
	m_inPeakR = 0;
	m_outPeakL = 0;
	m_outPeakR = 0;
	m_lowShelfPeakL = 0; m_lowShelfPeakR = 0;
	m_para1PeakL = 0; m_para1PeakR = 0;
	m_para2PeakL = 0; m_para2PeakR = 0;
	m_para3PeakL = 0; m_para3PeakR = 0;
	m_para4PeakL = 0; m_para4PeakR = 0;
	m_highShelfPeakL = 0; m_highShelfPeakR = 0;
	m_inProgress = false;
	m_inGainModel.setScaleLogarithmic( true );
}




void EqControls::loadSettings( const QDomElement &_this )
{
	m_inGainModel.loadSettings( _this, "Inputgain" );
	m_outGainModel.loadSettings( _this, "Outputgain" );
	m_lowShelfGainModel.loadSettings( _this , "Lowshelfgain" );
	m_para1GainModel.loadSettings( _this, "Peak1gain" );
	m_para2GainModel.loadSettings( _this, "Peak2gain" );
	m_para3GainModel.loadSettings( _this, "Peak3gain" );
	m_para4GainModel.loadSettings( _this, "Peak4gain" );
	m_highShelfGainModel.loadSettings( _this , "HighShelfgain" );
	m_hpResModel.loadSettings( _this ,"HPres" );
	m_lowShelfResModel.loadSettings( _this, "LowShelfres" );
	m_para1BwModel.loadSettings( _this ,"Peak1bw" );
	m_para2BwModel.loadSettings( _this ,"Peak2bw" );
	m_para3BwModel.loadSettings( _this ,"Peak3bw" );
	m_para4BwModel.loadSettings( _this ,"Peak4bw" );
	m_highShelfResModel.loadSettings( _this, "HighShelfres" );
	m_lpResModel.loadSettings( _this, "LPres" );
	m_hpFeqModel.loadSettings( _this, "HPfreq" );
	m_lowShelfFreqModel.loadSettings( _this, "LowShelffreq" );
	m_para1FreqModel.loadSettings( _this, "Peak1freq" );
	m_para2FreqModel.loadSettings( _this, "Peak2freq" );
	m_para3FreqModel.loadSettings( _this, "Peak3freq" );
	m_para4FreqModel.loadSettings( _this, "Peak4freq" );
	m_highShelfFreqModel.loadSettings( _this, "Highshelffreq" );
	m_lpFreqModel.loadSettings( _this, "LPfreq" );
	m_hpActiveModel.loadSettings( _this, "HPactive" );
	m_lowShelfActiveModel.loadSettings( _this, "Lowshelfactive" );
	m_para1ActiveModel.loadSettings( _this, "Peak1active" );
	m_para2ActiveModel.loadSettings( _this, "Peak2active" );
	m_para3ActiveModel.loadSettings( _this, "Peak3active" );
	m_para4ActiveModel.loadSettings( _this, "Peak4active" );
	m_highShelfActiveModel.loadSettings( _this, "Highshelfactive" );
	m_lpActiveModel.loadSettings( _this, "LPactive" );
	m_lp12Model.loadSettings( _this , "LP12" );
	m_lp24Model.loadSettings( _this , "LP24" );
	m_lp48Model.loadSettings( _this , "LP48" );
	m_hp12Model.loadSettings( _this , "HP12" );
	m_hp24Model.loadSettings( _this , "HP24" );
	m_hp48Model.loadSettings( _this , "HP48" );
	m_lpTypeModel.loadSettings( _this, "LP" );
	m_hpTypeModel.loadSettings( _this, "HP" );
	m_analyseInModel.loadSettings( _this, "AnalyseIn" );
	m_analyseOutModel.loadSettings( _this, "AnalyseOut" );
}

gui::EffectControlDialog* EqControls::createView()
{
	return new gui::EqControlsDialog( this );
}




void EqControls::saveSettings( QDomDocument &doc, QDomElement &parent )
{
	m_inGainModel.saveSettings( doc, parent, "Inputgain" );
	m_outGainModel.saveSettings( doc, parent, "Outputgain");
	m_lowShelfGainModel.saveSettings( doc, parent , "Lowshelfgain" );
	m_para1GainModel.saveSettings( doc, parent, "Peak1gain" );
	m_para2GainModel.saveSettings( doc, parent, "Peak2gain" );
	m_para3GainModel.saveSettings( doc, parent, "Peak3gain" );
	m_para4GainModel.saveSettings( doc, parent, "Peak4gain" );
	m_highShelfGainModel.saveSettings( doc, parent, "HighShelfgain" );
	m_hpResModel.saveSettings( doc, parent ,"HPres" );
	m_lowShelfResModel.saveSettings( doc, parent, "LowShelfres" );
	m_para1BwModel.saveSettings( doc, parent,"Peak1bw" );
	m_para2BwModel.saveSettings( doc, parent,"Peak2bw" );
	m_para3BwModel.saveSettings( doc, parent,"Peak3bw" );
	m_para4BwModel.saveSettings( doc, parent,"Peak4bw" );
	m_highShelfResModel.saveSettings( doc, parent, "HighShelfres" );
	m_lpResModel.saveSettings( doc, parent, "LPres" );
	m_hpFeqModel.saveSettings( doc, parent, "HPfreq" );
	m_lowShelfFreqModel.saveSettings( doc, parent, "LowShelffreq" );
	m_para1FreqModel.saveSettings( doc, parent, "Peak1freq" );
	m_para2FreqModel.saveSettings( doc, parent, "Peak2freq" );
	m_para3FreqModel.saveSettings( doc, parent, "Peak3freq" );
	m_para4FreqModel.saveSettings( doc, parent, "Peak4freq" );
	m_highShelfFreqModel.saveSettings( doc, parent, "Highshelffreq" );
	m_lpFreqModel.saveSettings( doc, parent, "LPfreq" );
	m_hpActiveModel.saveSettings( doc, parent, "HPactive" );
	m_lowShelfActiveModel.saveSettings( doc, parent, "Lowshelfactive" );
	m_para1ActiveModel.saveSettings( doc, parent, "Peak1active" );
	m_para2ActiveModel.saveSettings( doc, parent, "Peak2active" );
	m_para3ActiveModel.saveSettings( doc, parent, "Peak3active" );
	m_para4ActiveModel.saveSettings( doc, parent, "Peak4active" );
	m_highShelfActiveModel.saveSettings( doc, parent, "Highshelfactive" );
	m_lpActiveModel.saveSettings( doc, parent, "LPactive" );
	m_lp12Model.saveSettings( doc, parent, "LP12" );
	m_lp24Model.saveSettings( doc, parent, "LP24" );
	m_lp48Model.saveSettings( doc, parent, "LP48" );
	m_hp12Model.saveSettings( doc, parent, "HP12" );
	m_hp24Model.saveSettings( doc, parent, "HP24" );
	m_hp48Model.saveSettings( doc, parent, "HP48" );
	m_lpTypeModel.saveSettings( doc, parent, "LP" );
	m_hpTypeModel.saveSettings( doc, parent, "HP" );
	m_analyseInModel.saveSettings( doc, parent, "AnalyseIn" );
	m_analyseOutModel.saveSettings( doc, parent, "AnalyseOut" );
}


} // namespace lmms
