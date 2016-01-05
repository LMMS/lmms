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


#include "EqControlsDialog.h"
#include "EqControls.h"
#include "embed.h"
#include "Fader.h"
#include "EqFader.h"
#include "Engine.h"
#include "AutomatableButton.h"
#include "LedCheckbox.h"
#include <QGraphicsView>
#include <QLayout>
#include <QWidget>


EqControlsDialog::EqControlsDialog( EqControls *controls ) :
	EffectControlDialog( controls )

{
	m_controls = controls;
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "EqLayout1BG" ) );
	setPalette( pal );
	setFixedSize( 500, 500 );
	QGridLayout * mainLayout = new QGridLayout( this );

	m_inSpec = new EqSpectrumView( &controls->m_inFftBands, this );
	mainLayout->addWidget( m_inSpec, 0, 1, 1, 8 );
	m_inSpec->color = QColor( 238, 154, 120, 80 );

	m_outSpec = new EqSpectrumView( &controls->m_outFftBands, this );
	m_outSpec->color = QColor( 145, 205, 22, 80 );
	mainLayout->addWidget( m_outSpec, 0, 1, 1, 8 );

	m_parameterWidget = new EqParameterWidget( this , controls );
	mainLayout->addWidget( m_parameterWidget, 0, 1, 1, 8 );

	setBand( 0, &controls->m_hpActiveModel, &controls->m_hpFeqModel, &controls->m_hpResModel, 0, QColor(255 ,255, 255), tr( "HP" ) ,0,0, &controls->m_hp12Model, &controls->m_hp24Model, &controls->m_hp48Model,0,0,0);
	setBand( 1, &controls->m_lowShelfActiveModel, &controls->m_lowShelfFreqModel, &controls->m_lowShelfResModel, &controls->m_lowShelfGainModel, QColor(255 ,255, 255), tr( "Low Shelf" ), &controls->m_lowShelfPeakL , &controls->m_lowShelfPeakR,0,0,0,0,0,0 );
	setBand( 2, &controls->m_para1ActiveModel, &controls->m_para1FreqModel, &controls->m_para1BwModel, &controls->m_para1GainModel, QColor(255 ,255, 255), tr( "Peak 1" ), &controls->m_para1PeakL, &controls->m_para1PeakR,0,0,0,0,0,0 );
	setBand( 3, &controls->m_para2ActiveModel, &controls->m_para2FreqModel, &controls->m_para2BwModel, &controls->m_para2GainModel, QColor(255 ,255, 255), tr( "Peak 2" ), &controls->m_para2PeakL, &controls->m_para2PeakR,0,0,0,0,0,0 );
	setBand( 4, &controls->m_para3ActiveModel, &controls->m_para3FreqModel, &controls->m_para3BwModel, &controls->m_para3GainModel, QColor(255 ,255, 255), tr( "Peak 3" ), &controls->m_para3PeakL, &controls->m_para3PeakR,0,0,0,0,0,0 );
	setBand( 5, &controls->m_para4ActiveModel, &controls->m_para4FreqModel, &controls->m_para4BwModel, &controls->m_para4GainModel, QColor(255 ,255, 255), tr( "Peak 4" ), &controls->m_para4PeakL, &controls->m_para4PeakR,0,0,0,0,0,0 );
	setBand( 6, &controls->m_highShelfActiveModel, &controls->m_highShelfFreqModel, &controls->m_highShelfResModel, &controls->m_highShelfGainModel, QColor(255 ,255, 255), tr( "High Shelf" ), &controls->m_highShelfPeakL, &controls->m_highShelfPeakR,0,0,0,0,0,0 );
	setBand( 7, &controls->m_lpActiveModel, &controls->m_lpFreqModel, &controls->m_lpResModel, 0, QColor(255 ,255, 255), tr( "LP" ) ,0,0,0,0,0, &controls->m_lp12Model, &controls->m_lp24Model, &controls->m_lp48Model);

	m_inGainFader = new EqFader( &controls->m_inGainModel, tr( "In Gain" ), this,
							&controls->m_inPeakL, &controls->m_inPeakR );

	mainLayout->addWidget( m_inGainFader, 0, 0 );
	m_inGainFader->setDisplayConversion( false );
	m_inGainFader->setHintText( tr( "Gain" ), "dBv");

	m_outGainFader = new EqFader( &controls->m_outGainModel, tr( "Out Gain" ), this,
							&controls->m_outPeakL, &controls->m_outPeakR );
	mainLayout->addWidget( m_outGainFader, 0, 9 );
	m_outGainFader->setDisplayConversion( false );
	m_outGainFader->setHintText( tr( "Gain" ), "dBv" );

	// Gain Fader for each Filter exepts the pass filter
	for( int i = 1; i < m_parameterWidget->bandCount() - 1; i++ )
	{
		m_gainFader = new EqFader( m_parameterWidget->getBandModels( i )->gain, tr( "" ), this,
								   m_parameterWidget->getBandModels( i )->peakL, m_parameterWidget->getBandModels( i )->peakR );
		mainLayout->addWidget( m_gainFader, 2, i+1 );
		mainLayout->setAlignment( m_gainFader, Qt::AlignHCenter );
		m_gainFader->setMinimumHeight(80);
		m_gainFader->resize(m_gainFader->width() , 80);
		m_gainFader->setDisplayConversion( false );
		m_gainFader->setHintText( tr( "Gain") , "dB");
	}
	
	//Control Button and Knobs for each Band
	for( int i = 0; i < m_parameterWidget->bandCount() ; i++ )
	{
		m_resKnob = new Knob( knobBright_26, this );
		mainLayout->setRowMinimumHeight( 4, 33 );
		mainLayout->addWidget( m_resKnob, 5, i+1 );
		mainLayout->setAlignment( m_resKnob, Qt::AlignHCenter );
		m_resKnob->setVolumeKnob(false);
		m_resKnob->setModel( m_parameterWidget->getBandModels( i )->res );
		if(i > 1 && i < 6) { m_resKnob->setHintText( tr( "Bandwidth: " ) , " Octave" ); }
		else { m_resKnob->setHintText( tr( "Resonance : " ) , "" ); }

		m_freqKnob = new Knob( knobBright_26, this );
		mainLayout->addWidget( m_freqKnob, 3, i+1 );
		mainLayout->setAlignment( m_freqKnob, Qt::AlignHCenter );
		m_freqKnob->setVolumeKnob( false );
		m_freqKnob->setModel( m_parameterWidget->getBandModels( i )->freq );
		m_freqKnob->setHintText( tr( "Frequency:" ), "Hz" );

		// adds the Number Active buttons
		m_activeBox = new PixmapButton( this, NULL );
		m_activeBox->setCheckable(true);
		m_activeBox->setModel( m_parameterWidget->getBandModels( i )->active );

		QString iconActiveFileName = "bandLabel" + QString::number(i+1) + "on";
		QString iconInactiveFileName = "bandLabel" + QString::number(i+1);
		m_activeBox->setActiveGraphic( PLUGIN_NAME::getIconPixmap( iconActiveFileName.toLatin1() ) );
		m_activeBox->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( iconInactiveFileName.toLatin1() ) );

		mainLayout->addWidget( m_activeBox, 1, i+1 );
		mainLayout->setAlignment( m_activeBox, Qt::AlignHCenter );
		m_activeBox->setModel( m_parameterWidget->getBandModels( i )->active );

		// adds the symbols active buttons
		m_activeBox = new PixmapButton( this, NULL );
		m_activeBox->setCheckable(true);
		m_activeBox->setModel( m_parameterWidget->getBandModels( i )->active );
		switch (i)
		{
		case 0:
			m_activeBox->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveHP" ) );
			m_activeBox->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveHPoff" ) );
			break;
		case 1:
			m_activeBox->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveLS" ) );
			m_activeBox->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveLSoff" ) );
			break;
		case 6:
			m_activeBox->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveHS" ) );
			m_activeBox->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveHSoff" ) );
			break;
		case 7:
			m_activeBox->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveLP" ) );
			m_activeBox->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveLPoff" ) );
			break;
		default:
			m_activeBox->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "ActivePeak" ) );
			m_activeBox->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "ActivePeakoff" ) );
		}

		mainLayout->addWidget( m_activeBox, 7, i+1 );
		mainLayout->setAlignment( m_activeBox, Qt::AlignHCenter);
		m_activeBox->setModel( m_parameterWidget->getBandModels( i )->active );

		// Connects the knobs, Faders and buttons with the curve graphic
		QObject::connect( m_parameterWidget->getBandModels( i )->freq , SIGNAL( dataChanged() ), m_parameterWidget, SLOT ( updateView() ) );
		if ( m_parameterWidget->getBandModels( i )->gain ) QObject::connect( m_parameterWidget->getBandModels( i )->gain, SIGNAL( dataChanged() ), m_parameterWidget, SLOT ( updateView() ));
		QObject::connect( m_parameterWidget->getBandModels( i )->res, SIGNAL( dataChanged() ), m_parameterWidget , SLOT ( updateView() ) );
		QObject::connect( m_parameterWidget->getBandModels( i )->active, SIGNAL( dataChanged() ), m_parameterWidget , SLOT ( updateView() ) );

		m_parameterWidget->changeHandle( i );
	}


	// adds the buttons for Spectrum analyser on/off
	m_inSpecB = new PixmapButton(this, NULL);
	m_inSpecB->setActiveGraphic(   PLUGIN_NAME::getIconPixmap(
								   "ActiveAnalyse" ) );
	m_inSpecB->setInactiveGraphic(   PLUGIN_NAME::getIconPixmap(
								   "ActiveAnalyseoff" ) );
	m_inSpecB->setCheckable(true);
	m_inSpecB->setModel( &controls->m_analyseInModel );

	m_outSpecB = new PixmapButton(this, NULL);
	m_outSpecB->setActiveGraphic(   PLUGIN_NAME::getIconPixmap(
								   "ActiveAnalyse" ) );
	m_outSpecB->setInactiveGraphic(   PLUGIN_NAME::getIconPixmap(
								   "ActiveAnalyseoff" ) );
	m_outSpecB->setCheckable(true);
	m_outSpecB->setModel( &controls->m_analyseOutModel );
	mainLayout->addWidget( m_inSpecB, 1, 0 );
	mainLayout->addWidget( m_outSpecB, 1, 9 );
	mainLayout->setAlignment( m_inSpecB, Qt::AlignHCenter );
	mainLayout->setAlignment( m_outSpecB, Qt::AlignHCenter );

	//hp filter type
	m_hp12Box = new PixmapButton( this , NULL );
	m_hp12Box->setModel( m_parameterWidget->getBandModels( 0 )->hp12  );
	m_hp12Box->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
									 "12dB" ) );
	m_hp12Box->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap(
									 "12dBoff" ) );

	m_hp24Box = new PixmapButton( this , NULL );
	m_hp24Box->setModel(m_parameterWidget->getBandModels( 0 )->hp24 );


	m_hp24Box->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
									 "24dB" ) );
	m_hp24Box->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap(
									 "24dBoff" ) );

	m_hp48Box = new PixmapButton( this , NULL );
	m_hp48Box->setModel( m_parameterWidget->getBandModels(0)->hp48 );

	m_hp48Box->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
									 "48dB" ) );
	m_hp48Box->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap(
									 "48dBoff" ) );

	//LP filter type
	m_lp12Box = new PixmapButton( this , NULL );
	mainLayout->addWidget( m_lp12Box, 2,1 );
	m_lp12Box->setModel( m_parameterWidget->getBandModels( 7 )->lp12 );
	m_lp12Box->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
									 "12dB" ) );
	m_lp12Box->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap(
									 "12dBoff" ) );

	m_lp24Box = new PixmapButton( this , NULL );
	m_lp24Box->setModel( m_parameterWidget->getBandModels( 7 )->lp24 );
	m_lp24Box->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
									 "24dB" ) );
	m_lp24Box->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap(
									 "24dBoff" ) );

	m_lp48Box = new PixmapButton( this , NULL );
	m_lp48Box->setModel( m_parameterWidget->getBandModels( 7 )->lp48 );
	m_lp48Box->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
									 "48dB" ) );
	m_lp48Box->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap(
									 "48dBoff" ) );
	// the curve has to change its appearance
	QObject::connect( m_parameterWidget->getBandModels( 0 )->hp12 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateView()));
	QObject::connect( m_parameterWidget->getBandModels( 0 )->hp24 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateView()));
	QObject::connect( m_parameterWidget->getBandModels( 0 )->hp48 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateView()));

	QObject::connect( m_parameterWidget->getBandModels( 7 )->lp12 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateView()));
	QObject::connect( m_parameterWidget->getBandModels( 7 )->lp24 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateView()));
	QObject::connect( m_parameterWidget->getBandModels( 7 )->lp48 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateView()));

	QVBoxLayout * hpGrpBtnLayout = new QVBoxLayout;
	hpGrpBtnLayout->addWidget( m_hp12Box );
	hpGrpBtnLayout->addWidget( m_hp24Box );
	hpGrpBtnLayout->addWidget( m_hp48Box );

	QVBoxLayout * lpGrpBtnLayout = new QVBoxLayout;
	lpGrpBtnLayout->addWidget( m_lp12Box );
	lpGrpBtnLayout->addWidget( m_lp24Box );
	lpGrpBtnLayout->addWidget( m_lp48Box );

	mainLayout->addLayout( hpGrpBtnLayout, 2, 1,  Qt::AlignCenter );
	mainLayout->addLayout( lpGrpBtnLayout, 2, 8,  Qt::AlignCenter );

	automatableButtonGroup *lpBtnGrp = new automatableButtonGroup(this,tr ( "lp grp" ) );
	lpBtnGrp->addButton( m_lp12Box );
	lpBtnGrp->addButton( m_lp24Box );
	lpBtnGrp->addButton( m_lp48Box );
	lpBtnGrp->setModel( &m_controls->m_lpTypeModel, false);

	automatableButtonGroup *hpBtnGrp = new automatableButtonGroup( this, tr( "hp grp" ) );
	hpBtnGrp->addButton( m_hp12Box );
	hpBtnGrp->addButton( m_hp24Box );
	hpBtnGrp->addButton( m_hp48Box );
	hpBtnGrp->setModel( &m_controls->m_hpTypeModel,false);

	mainLayout->setAlignment( Qt::AlignTop );
	for (int i = 0 ; i < 10; i++)
	{
		mainLayout->setColumnMinimumWidth(i, 50);
	}

	mainLayout->setAlignment( m_inGainFader, Qt::AlignHCenter );
	mainLayout->setAlignment( m_outGainFader, Qt::AlignHCenter );
	mainLayout->setRowMinimumHeight( 0,200 );
	mainLayout->setRowMinimumHeight( 1, 40 );
	mainLayout->setRowMinimumHeight(6,15);
	mainLayout->setContentsMargins( 0, 11, 0, 0 );
	mainLayout->setAlignment(m_inSpec, Qt::AlignCenter );
	mainLayout->setAlignment(m_outSpec, Qt::AlignCenter );

	m_freqLabel = new QLabel(this);
	m_freqLabel->setText("- " + tr( "Frequency")+ " -" );
	m_freqLabel->move( 217 , 377 );

	m_resLabel1 = new QLabel(this);
	m_resLabel1->setText("- " + tr( "Resonance")+ " -" );
	m_resLabel1->move( 62 , 444 );

	m_resLabel2 = new QLabel(this);
	m_resLabel2->setText("- " + tr( "Resonance")+ " -" );
	m_resLabel2->move( 365 , 444 );

	m_bandWidthLabel = new QLabel(this);
	m_bandWidthLabel->setText("- " + tr( "Bandwidth")+ " -" );
	m_bandWidthLabel->move( 215 , 444 );

	setLayout(mainLayout);
}




void EqControlsDialog::mouseDoubleClickEvent(QMouseEvent *event)
{
	m_originalHeight = parentWidget()->height() == 283 ? m_originalHeight : parentWidget()->height() ;
	parentWidget()->setFixedHeight( parentWidget()->height() == m_originalHeight ? 283 : m_originalHeight  );
	update();
}
