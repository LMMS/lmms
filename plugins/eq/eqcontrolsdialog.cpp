/*
 * eqcontrolsdialog.cpp - defination of EqControlsDialog class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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


#include "eqcontrolsdialog.h"
#include "eqcontrols.h"
#include "embed.h"
#include "Fader.h"
#include "eqfader.h"
#include "Engine.h"
#include "AutomatableButton.h"
#include "QWidget"
#include "MainWindow.h"
#include "LedCheckbox.h"




EqControlsDialog::EqControlsDialog(EqControls *controls) :
	EffectControlDialog( controls )

{
	m_controls = controls;
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 350, 275 );

	m_parameterWidget = new EqParameterWidget( this );
	m_parameterWidget->move( 50, 5 );
//	m_inSpec = new EqSpectrumView( controls->m_effect, this);

	setBand( 0, &controls->m_hpActiveModel, &controls->m_hpFeqModel, &controls->m_hpResModel, 0, QColor(173, 115, 57), tr( "HP" ) ,0,0);
	setBand( 1, &controls->m_lowShelfActiveModel, &controls->m_lowShelfFreqModel, &controls->m_lowShelfResModel, &controls->m_lowShelfGainModel, QColor(255, 0, 0), tr( "Low Shelf" ), &controls->m_lowShelfPeakL , &controls->m_lowShelfPeakR );
	setBand( 2, &controls->m_para1ActiveModel, &controls->m_para1FreqModel, &controls->m_para1ResModel, &controls->m_para1GainModel, QColor(255, 173, 115), tr( "Peak 1" ), &controls->m_para1PeakL, &controls->m_para1PeakR );
	setBand( 3, &controls->m_para2ActiveModel, &controls->m_para2FreqModel, &controls->m_para2ResModel, &controls->m_para2GainModel, QColor(255, 255, 0), tr( "Peak 2" ), &controls->m_para2PeakL, &controls->m_para2PeakR );
	setBand( 4, &controls->m_para3ActiveModel, &controls->m_para3FreqModel, &controls->m_para3ResModel, &controls->m_para3GainModel, QColor(0, 255, 0), tr( "Peak 3" ), &controls->m_para3PeakL, &controls->m_para3PeakR );
	setBand( 5, &controls->m_para4ActiveModel, &controls->m_para4FreqModel, &controls->m_para4ResModel, &controls->m_para4GainModel, QColor(0, 186, 255), tr( "Peak 4" ), &controls->m_para4PeakL, &controls->m_para4PeakR );
	setBand( 6, &controls->m_highShelfActiveModel, &controls->m_highShelfFreqModel, &controls->m_highShelfResModel, &controls->m_highShelfGainModel, QColor(222, 0, 222 ), tr( "High Shelf" ), &controls->m_highShelfPeakL, &controls->m_highShelfPeakR );
	setBand( 7, &controls->m_lpActiveModel, &controls->m_lpFreqModel, &controls->m_lpResModel, 0, QColor(156, 156, 156 ), tr( "LP" ) ,0,0);
	int cw = width()/8; //the chanel width in pixels
	int ko = ( cw * 0.5 ) - ((new Knob( knobBright_26, 0 ))->width() * 0.5 );

	m_inGainFader = new EqFader( &controls->m_inGainModel, tr( "In Gain" ), this,  &controls->m_inPeakL, &controls->m_inPeakR);
	m_inGainFader->move( 10, 5 );

	m_outGainFader = new EqFader( &controls->m_outGainModel, tr( "Out Gain" ), this, &controls->m_outPeakL, &controls->m_outPeakR );
	m_outGainFader->move( 315, 5 );
	//gain faders

	int fo = (cw * 0.5) - (m_outGainFader->width() * 0.5 );

	for( int i = 1; i < m_parameterWidget->bandCount() - 1; i++)
	{
		m_gainFader = new EqFader( m_parameterWidget->getBandModels(i)->gain, tr( "" ), this ,m_parameterWidget->getBandModels( i )->peakL, m_parameterWidget->getBandModels( i )->peakR );
		m_gainFader->move( cw * i + fo , 123 );
		m_gainFader->setMinimumHeight(80);
		m_gainFader->resize(m_gainFader->width() , 80);
	}

	for( int i = 0; i < m_parameterWidget->bandCount() ; i++)
	{
		m_resKnob = new Knob( knobBright_26, this );
		m_resKnob->move(cw * i + ko , 205 );
		m_resKnob->setVolumeKnob(false);
		m_resKnob->setModel( m_parameterWidget->getBandModels( i )->res );

		m_freqKnob = new Knob( knobBright_26, this );
		m_freqKnob->move(cw * i + ko, 235 );
		m_freqKnob->setVolumeKnob( false );
		m_freqKnob->setModel( m_parameterWidget->getBandModels( i )->freq );

		m_activeBox = new LedCheckBox( m_parameterWidget->getBandModels( i )->name , this );
		m_activeBox->move( cw * i + fo + 3, 260 );
		m_activeBox->setModel( m_parameterWidget->getBandModels( i )->active );
	}

	//hp filter type

	m_hp12Box = new LedCheckBox( tr( "12dB" ), this );
	m_hp12Box->move( cw*0 + ko, 175 );
	m_hp12Box->setModel( &controls->m_hp12Model );
	m_hp24Box = new LedCheckBox( tr( "24dB" ), this );
	m_hp24Box->move( cw*0 + ko, 155 );
	m_hp24Box->setModel( &controls->m_hp24Model );

	m_hp48Box = new LedCheckBox( tr( "48dB" ), this );
	m_hp48Box->move( cw*0 + ko, 135 );
	m_hp48Box->setModel( &controls->m_hp48Model );

	//LP filter type

	m_lp12Box = new LedCheckBox( tr( "12dB"), this );
	m_lp12Box->move( cw*7 + ko -5 , 175 );
	m_lp12Box->setModel( &controls->m_lp12Model );
	m_lp24Box = new LedCheckBox( tr( "24dB"), this );
	m_lp24Box->move( cw*7 + ko - 5, 155 );
	m_lp24Box->setModel( &controls->m_lp24Model );
	m_lp48Box = new LedCheckBox( tr( "48dB"), this );
	m_lp48Box->move( cw*7 + ko - 5, 135 );
	m_lp48Box->setModel( &controls->m_lp48Model );

	automatableButtonGroup *lpBtnGrp = new automatableButtonGroup(this,tr ( "lp grp" ) );
	lpBtnGrp->addButton(m_lp12Box);
	lpBtnGrp->addButton(m_lp24Box );
	lpBtnGrp->addButton(m_lp48Box );
	connect( m_lp12Box, SIGNAL( clicked() ), lpBtnGrp , SLOT( updateButtons() ) );
	connect( m_lp24Box, SIGNAL( clicked() ), lpBtnGrp , SLOT( updateButtons() ) );
	connect( m_lp48Box, SIGNAL( clicked() ), lpBtnGrp , SLOT( updateButtons() ) );
	connect( &controls->m_lp12Model, SIGNAL( dataChanged() ), lpBtnGrp, SLOT( updateButtons() ) );
	connect( &controls->m_lp24Model, SIGNAL( dataChanged() ), lpBtnGrp, SLOT( updateButtons() ) );
	connect( &controls->m_lp48Model, SIGNAL( dataChanged() ), lpBtnGrp, SLOT( updateButtons() ) );



	automatableButtonGroup *hpBtnGrp = new automatableButtonGroup( this, tr( "hp grp" ) );
	hpBtnGrp->addButton(m_hp12Box );
	hpBtnGrp->addButton(m_hp24Box );
	hpBtnGrp->addButton(m_hp48Box );
	connect( m_hp12Box, SIGNAL ( clicked() ), hpBtnGrp, SLOT( updateButtons() ) );
	connect( m_hp24Box, SIGNAL ( clicked() ), hpBtnGrp, SLOT( updateButtons() ) );
	connect( m_hp48Box, SIGNAL ( clicked() ), hpBtnGrp, SLOT( updateButtons() ) );
	connect( &controls->m_hp12Model, SIGNAL( dataChanged() ), hpBtnGrp, SLOT( updateButtons() ) );
	connect( &controls->m_hp24Model, SIGNAL( dataChanged() ), hpBtnGrp, SLOT( updateButtons() ) );
	connect( &controls->m_hp48Model, SIGNAL( dataChanged() ), hpBtnGrp, SLOT( updateButtons() ) );




	//Analize Box
	m_analyzeBox = new LedCheckBox( tr( "Analyze" ), this );
	m_analyzeBox->move( cw*1 + ko + 5, 10 );
	m_analyzeBox->setModel( &controls->m_analyzeModel );

}

void EqControlsDialog::mouseDoubleClickEvent(QMouseEvent *event)
{
	m_originalHeight = parentWidget()->height() == 150 ? m_originalHeight : parentWidget()->height() ;
	parentWidget()->setFixedHeight( parentWidget()->height() == m_originalHeight ? 150 : m_originalHeight  );
	update();
}
