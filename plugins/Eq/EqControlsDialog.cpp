/*
 * eqcontrolsdialog.cpp - defination of EqControlsDialog class.
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


#include "EqControlsDialog.h"

#include <QWidget>

#include "AutomatableButton.h"
#include "embed.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "PixmapButton.h"

#include "EqControls.h"
#include "EqFader.h"
#include "EqParameterWidget.h"
#include "EqSpectrumView.h"

namespace lmms::gui
{


EqControlsDialog::EqControlsDialog( EqControls *controls ) :
	EffectControlDialog( controls ),
	m_controls( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 500, 500 );

	auto inSpec = new EqSpectrumView(&controls->m_inFftBands, this);
	inSpec->move( 26, 17 );
	inSpec->setColor( QColor( 77, 101, 242, 150 ) );

	auto outSpec = new EqSpectrumView(&controls->m_outFftBands, this);
	outSpec->setColor( QColor( 0, 255, 239, 150 ) );
	outSpec->move( 26, 17 );

	m_parameterWidget = new EqParameterWidget( this , controls );
	m_parameterWidget->move( 26, 17 );

	setBand( 0, &controls->m_hpActiveModel, &controls->m_hpFeqModel, &controls->m_hpResModel, 0, QColor(255 ,255, 255), tr( "HP" ) ,0,0, &controls->m_hp12Model, &controls->m_hp24Model, &controls->m_hp48Model,0,0,0);
	setBand( 1, &controls->m_lowShelfActiveModel, &controls->m_lowShelfFreqModel, &controls->m_lowShelfResModel, &controls->m_lowShelfGainModel, QColor(255 ,255, 255), tr( "Low-shelf" ), &controls->m_lowShelfPeakL , &controls->m_lowShelfPeakR,0,0,0,0,0,0 );
	setBand( 2, &controls->m_para1ActiveModel, &controls->m_para1FreqModel, &controls->m_para1BwModel, &controls->m_para1GainModel, QColor(255 ,255, 255), tr( "Peak 1" ), &controls->m_para1PeakL, &controls->m_para1PeakR,0,0,0,0,0,0 );
	setBand( 3, &controls->m_para2ActiveModel, &controls->m_para2FreqModel, &controls->m_para2BwModel, &controls->m_para2GainModel, QColor(255 ,255, 255), tr( "Peak 2" ), &controls->m_para2PeakL, &controls->m_para2PeakR,0,0,0,0,0,0 );
	setBand( 4, &controls->m_para3ActiveModel, &controls->m_para3FreqModel, &controls->m_para3BwModel, &controls->m_para3GainModel, QColor(255 ,255, 255), tr( "Peak 3" ), &controls->m_para3PeakL, &controls->m_para3PeakR,0,0,0,0,0,0 );
	setBand( 5, &controls->m_para4ActiveModel, &controls->m_para4FreqModel, &controls->m_para4BwModel, &controls->m_para4GainModel, QColor(255 ,255, 255), tr( "Peak 4" ), &controls->m_para4PeakL, &controls->m_para4PeakR,0,0,0,0,0,0 );
	setBand( 6, &controls->m_highShelfActiveModel, &controls->m_highShelfFreqModel, &controls->m_highShelfResModel, &controls->m_highShelfGainModel, QColor(255 ,255, 255), tr( "High-shelf" ), &controls->m_highShelfPeakL, &controls->m_highShelfPeakR,0,0,0,0,0,0 );
	setBand( 7, &controls->m_lpActiveModel, &controls->m_lpFreqModel, &controls->m_lpResModel, 0, QColor(255 ,255, 255), tr( "LP" ) ,0,0,0,0,0, &controls->m_lp12Model, &controls->m_lp24Model, &controls->m_lp48Model);

	QSize const faderSize(23, 80);

	auto GainFaderIn = new EqFader(&controls->m_inGainModel, tr("Input gain"), this, &controls->m_inPeakL, &controls->m_inPeakR);
	GainFaderIn->setFixedSize(faderSize);
	GainFaderIn->move( 23, 295 );
	GainFaderIn->setDisplayConversion( false );
	GainFaderIn->setHintText( tr( "Gain" ), "dBv");

	auto GainFaderOut = new EqFader(&controls->m_outGainModel, tr("Output gain"), this, &controls->m_outPeakL, &controls->m_outPeakR);
	GainFaderOut->setFixedSize(faderSize);
	GainFaderOut->move( 453, 295);
	GainFaderOut->setDisplayConversion( false );
	GainFaderOut->setHintText( tr( "Gain" ), "dBv" );

	// Gain Fader for each Filter exepts the pass filter
	int distance = 126;
	for( int i = 1; i < m_parameterWidget->bandCount() - 1; i++ )
	{
		auto gainFader = new EqFader(m_parameterWidget->getBandModels(i)->gain, tr(""), this,
			m_parameterWidget->getBandModels(i)->peakL, m_parameterWidget->getBandModels(i)->peakR);
		gainFader->setFixedSize(faderSize);
		gainFader->move( distance, 295 );
		distance += 44;
		gainFader->setMinimumHeight(80);
		gainFader->resize(gainFader->width() , 80);
		gainFader->setDisplayConversion( false );
		gainFader->setHintText( tr( "Gain") , "dB");
	}

	//Control Button and Knobs for each Band
	distance = 81;
	for( int i = 0; i < m_parameterWidget->bandCount() ; i++ )
	{
		auto resKnob = new Knob(KnobType::Bright26, this);
		resKnob->move( distance, 440 );
		resKnob->setVolumeKnob(false);
		resKnob->setModel( m_parameterWidget->getBandModels( i )->res );
		if(i > 1 && i < 6) { resKnob->setHintText( tr( "Bandwidth: " ) , tr( " Octave" ) ); }
		else { resKnob->setHintText(tr("Resonance: "), ""); }

		auto freqKnob = new Knob(KnobType::Bright26, this);
		freqKnob->move( distance, 396 );
		freqKnob->setVolumeKnob( false );
		freqKnob->setModel( m_parameterWidget->getBandModels( i )->freq );
		freqKnob->setHintText( tr( "Frequency:" ), "Hz" );

		// adds the Number Active buttons
		auto activeButton = new PixmapButton(this, nullptr);
		activeButton->setCheckable(true);
		activeButton->setModel( m_parameterWidget->getBandModels( i )->active );

		const auto iconActiveFileName = "bandLabel" + std::to_string(i + 1);
		const auto iconInactiveFileName = iconActiveFileName + "off";
		activeButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap(iconActiveFileName));
		activeButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(iconInactiveFileName));
		activeButton->move( distance - 2, 276 );
		activeButton->setModel( m_parameterWidget->getBandModels( i )->active );

		// Connects the knobs, Faders and buttons with the curve graphic
		QObject::connect( m_parameterWidget->getBandModels( i )->freq , SIGNAL( dataChanged() ), m_parameterWidget, SLOT ( updateHandle() ) );
		if ( m_parameterWidget->getBandModels( i )->gain ) QObject::connect( m_parameterWidget->getBandModels( i )->gain, SIGNAL( dataChanged() ), m_parameterWidget, SLOT ( updateHandle() ));
		QObject::connect( m_parameterWidget->getBandModels( i )->res, SIGNAL( dataChanged() ), m_parameterWidget , SLOT ( updateHandle() ) );
		QObject::connect( m_parameterWidget->getBandModels( i )->active, SIGNAL( dataChanged() ), m_parameterWidget , SLOT ( updateHandle() ) );

		m_parameterWidget->changeHandle( i );
		distance += 44;
	}


	// adds the buttons for Spectrum analyser on/off
	auto inSpecButton = new LedCheckBox(this);
	inSpecButton->setCheckable(true);
	inSpecButton->setModel( &controls->m_analyseInModel );
	inSpecButton->move( 172, 240 );
	auto outSpecButton = new LedCheckBox(this);
	outSpecButton->setCheckable(true);
	outSpecButton->setModel( &controls->m_analyseOutModel );
	outSpecButton->move( 302, 240 );

	//hp filter type
	auto hp12Button = new PixmapButton(this, nullptr);
	hp12Button->setModel( m_parameterWidget->getBandModels( 0 )->hp12 );
	hp12Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "12dB" ) );
	hp12Button->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap( "12dBoff" ) );
	hp12Button->move( 79, 298 );
	auto hp24Button = new PixmapButton(this, nullptr);
	hp24Button->setModel(m_parameterWidget->getBandModels( 0 )->hp24 );
	hp24Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "24dB" ) );
	hp24Button->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap( "24dBoff" ) );

	hp24Button->move( 79 , 328 );
	auto hp48Button = new PixmapButton(this, nullptr);
	hp48Button->setModel( m_parameterWidget->getBandModels(0)->hp48 );
	hp48Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "48dB" ) );
	hp48Button->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap( "48dBoff" ) );

	hp48Button->move( 79, 358 );
	//LP filter type
	auto lp12Button = new PixmapButton(this, nullptr);
	lp12Button->setModel( m_parameterWidget->getBandModels( 7 )->lp12 );
	lp12Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "12dB" ) );
	lp12Button->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap( "12dBoff" ) );

	lp12Button->move( 387, 298 );
	auto lp24Button = new PixmapButton(this, nullptr);
	lp24Button->setModel( m_parameterWidget->getBandModels( 7 )->lp24 );
	lp24Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "24dB" ) );
	lp24Button->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap( "24dBoff" ) );

	lp24Button->move( 387, 328 );

	auto lp48Button = new PixmapButton(this, nullptr);
	lp48Button->setModel( m_parameterWidget->getBandModels( 7 )->lp48 );
	lp48Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "48dB" ) );
	lp48Button->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap( "48dBoff" ) );

	lp48Button->move( 387, 358 );
	// the curve has to change its appearance
	QObject::connect( m_parameterWidget->getBandModels( 0 )->hp12 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateHandle()));
	QObject::connect( m_parameterWidget->getBandModels( 0 )->hp24 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateHandle()));
	QObject::connect( m_parameterWidget->getBandModels( 0 )->hp48 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateHandle()));

	QObject::connect( m_parameterWidget->getBandModels( 7 )->lp12 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateHandle()));
	QObject::connect( m_parameterWidget->getBandModels( 7 )->lp24 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateHandle()));
	QObject::connect( m_parameterWidget->getBandModels( 7 )->lp48 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateHandle()));

	auto lpBtnGrp = new AutomatableButtonGroup(this, tr("LP group"));
	lpBtnGrp->addButton( lp12Button );
	lpBtnGrp->addButton( lp24Button );
	lpBtnGrp->addButton( lp48Button );
	lpBtnGrp->setModel(&m_controls->m_lpTypeModel);

	auto hpBtnGrp = new AutomatableButtonGroup(this, tr("HP group"));
	hpBtnGrp->addButton( hp12Button );
	hpBtnGrp->addButton( hp24Button );
	hpBtnGrp->addButton( hp48Button );
	hpBtnGrp->setModel(&m_controls->m_hpTypeModel);
}




void EqControlsDialog::mouseDoubleClickEvent(QMouseEvent *event)
{
	m_originalHeight = parentWidget()->height() == 283 ? m_originalHeight : parentWidget()->height() ;
	parentWidget()->setFixedHeight( parentWidget()->height() == m_originalHeight ? 283 : m_originalHeight  );
	update();
}

EqBand* EqControlsDialog::setBand(int index, BoolModel* active, FloatModel* freq, FloatModel* res, FloatModel* gain, QColor color, QString name, float* peakL, float* peakR, BoolModel* hp12, BoolModel* hp24, BoolModel* hp48, BoolModel* lp12, BoolModel* lp24, BoolModel* lp48)
{
	EqBand *filterModels = m_parameterWidget->getBandModels( index );
	filterModels->active = active;
	filterModels->freq = freq;
	filterModels->res = res;
	filterModels->color = color;
	filterModels->gain = gain;
	filterModels->peakL = peakL;
	filterModels->peakR = peakR;
	filterModels->hp12 = hp12;
	filterModels->hp24 = hp24;
	filterModels->hp48 = hp48;
	filterModels->lp12 = lp12;
	filterModels->lp24 = lp24;
	filterModels->lp48 = lp48;
	return filterModels;
}


} // namespace lmms::gui
