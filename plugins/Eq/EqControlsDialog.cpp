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

#include <QGraphicsView>
#include <QLayout>
#include <QWidget>

#include "AutomatableButton.h"
#include "embed.h"
#include "Engine.h"
#include "EqControls.h"
#include "EqFader.h"
#include "Fader.h"
#include "LedCheckbox.h"


EqControlsDialog::EqControlsDialog( EqControls *controls ) :
	EffectControlDialog( controls ),
	m_controls( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "EqLayout1BG" ) );
	setPalette( pal );
	setFixedSize( 500, 500 );
	QGridLayout *mainLayout = new QGridLayout( this );

	EqSpectrumView *inSpec = new EqSpectrumView( &controls->m_inFftBands, this );
	mainLayout->addWidget( inSpec, 0, 1, 1, 8 );
	inSpec->setColor( QColor( 238, 154, 120, 80 ) );

	EqSpectrumView *outSpec = new EqSpectrumView( &controls->m_outFftBands, this );
	outSpec->setColor( QColor( 145, 205, 22, 80 ) );
	mainLayout->addWidget( outSpec, 0, 1, 1, 8 );

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

	EqFader *inGainFader = new EqFader( &controls->m_inGainModel, tr( "In Gain" ), this, &controls->m_inPeakL, &controls->m_inPeakR );
	mainLayout->addWidget( inGainFader, 0, 0 );
	inGainFader->setDisplayConversion( false );
	inGainFader->setHintText( tr( "Gain" ), "dBv");

	EqFader *outGainFader = new EqFader( &controls->m_outGainModel, tr( "Out Gain" ), this, &controls->m_outPeakL, &controls->m_outPeakR );
	mainLayout->addWidget( outGainFader, 0, 9 );
	outGainFader->setDisplayConversion( false );
	outGainFader->setHintText( tr( "Gain" ), "dBv" );

	// Gain Fader for each Filter exepts the pass filter
	for( int i = 1; i < m_parameterWidget->bandCount() - 1; i++ )
	{
		EqFader *gainFader = new EqFader( m_parameterWidget->getBandModels( i )->gain, tr( "" ), this,
								   m_parameterWidget->getBandModels( i )->peakL, m_parameterWidget->getBandModels( i )->peakR );
		mainLayout->addWidget( gainFader, 2, i+1 );
		mainLayout->setAlignment( gainFader, Qt::AlignHCenter );
		gainFader->setMinimumHeight(80);
		gainFader->resize(gainFader->width() , 80);
		gainFader->setDisplayConversion( false );
		gainFader->setHintText( tr( "Gain") , "dB");
	}
	
	//Control Button and Knobs for each Band
	for( int i = 0; i < m_parameterWidget->bandCount() ; i++ )
	{
		Knob *resKnob = new Knob( knobBright_26, this );
		mainLayout->setRowMinimumHeight( 4, 33 );
		mainLayout->addWidget( resKnob, 5, i + 1 );
		mainLayout->setAlignment( resKnob, Qt::AlignHCenter );
		resKnob->setVolumeKnob(false);
		resKnob->setModel( m_parameterWidget->getBandModels( i )->res );
		if(i > 1 && i < 6) { resKnob->setHintText( tr( "Bandwidth: " ) , tr( " Octave" ) ); }
		else { resKnob->setHintText( tr( "Resonance : " ) , "" ); }

		Knob *freqKnob = new Knob( knobBright_26, this );
		mainLayout->addWidget( freqKnob, 3, i+1 );
		mainLayout->setAlignment( freqKnob, Qt::AlignHCenter );
		freqKnob->setVolumeKnob( false );
		freqKnob->setModel( m_parameterWidget->getBandModels( i )->freq );
		freqKnob->setHintText( tr( "Frequency:" ), "Hz" );

		// adds the Number Active buttons
		PixmapButton *activeNumButton = new PixmapButton( this, NULL );
		activeNumButton->setCheckable(true);
		activeNumButton->setModel( m_parameterWidget->getBandModels( i )->active );
		QString iconActiveFileName = "bandLabel" + QString::number(i+1) + "on";
		QString iconInactiveFileName = "bandLabel" + QString::number(i+1);
		activeNumButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( iconActiveFileName.toLatin1() ) );
		activeNumButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( iconInactiveFileName.toLatin1() ) );
		mainLayout->addWidget( activeNumButton, 1, i+1 );
		mainLayout->setAlignment( activeNumButton, Qt::AlignHCenter );
		activeNumButton->setModel( m_parameterWidget->getBandModels( i )->active );

		// adds the symbols active buttons
		PixmapButton *activeButton = new PixmapButton( this, NULL );
		activeButton->setCheckable(true);
		activeButton->setModel( m_parameterWidget->getBandModels( i )->active );
		switch (i)
		{
		case 0:
			activeButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveHP" ) );
			activeButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveHPoff" ) );
			break;
		case 1:
			activeButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveLS" ) );
			activeButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveLSoff" ) );
			break;
		case 6:
			activeButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveHS" ) );
			activeButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveHSoff" ) );
			break;
		case 7:
			activeButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveLP" ) );
			activeButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveLPoff" ) );
			break;
		default:
			activeButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "ActivePeak" ) );
			activeButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "ActivePeakoff" ) );
		}

		mainLayout->addWidget( activeButton, 7, i+1 );
		mainLayout->setAlignment( activeButton, Qt::AlignHCenter);
		activeButton->setModel( m_parameterWidget->getBandModels( i )->active );

		// Connects the knobs, Faders and buttons with the curve graphic
		QObject::connect( m_parameterWidget->getBandModels( i )->freq , SIGNAL( dataChanged() ), m_parameterWidget, SLOT ( updateView() ) );
		if ( m_parameterWidget->getBandModels( i )->gain ) QObject::connect( m_parameterWidget->getBandModels( i )->gain, SIGNAL( dataChanged() ), m_parameterWidget, SLOT ( updateView() ));
		QObject::connect( m_parameterWidget->getBandModels( i )->res, SIGNAL( dataChanged() ), m_parameterWidget , SLOT ( updateView() ) );
		QObject::connect( m_parameterWidget->getBandModels( i )->active, SIGNAL( dataChanged() ), m_parameterWidget , SLOT ( updateView() ) );

		m_parameterWidget->changeHandle( i );
	}

	// adds the buttons for Spectrum analyser on/off
	PixmapButton *inSpecB = new PixmapButton(this, NULL);
	inSpecB->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveAnalyse" ) );
	inSpecB->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveAnalyseoff" ) );
	inSpecB->setCheckable( true );
	inSpecB->setModel( &controls->m_analyseInModel );

	PixmapButton *outSpecB = new PixmapButton(this, NULL);
	outSpecB->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveAnalyse" ) );
	outSpecB->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "ActiveAnalyseoff" ) );
	outSpecB->setCheckable( true );
	outSpecB->setModel( &controls->m_analyseOutModel );
	mainLayout->addWidget( inSpecB, 1, 0 );
	mainLayout->addWidget( outSpecB, 1, 9 );
	mainLayout->setAlignment( inSpecB, Qt::AlignHCenter );
	mainLayout->setAlignment( outSpecB, Qt::AlignHCenter );

	//hp filter type
	PixmapButton *hp12Button = new PixmapButton( this , NULL );
	hp12Button->setModel( m_parameterWidget->getBandModels( 0 )->hp12 );
	hp12Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "12dB" ) );
	hp12Button->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap( "12dBoff" ) );

	PixmapButton *hp24Button = new PixmapButton( this , NULL );
	hp24Button->setModel(m_parameterWidget->getBandModels( 0 )->hp24 );
	hp24Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "24dB" ) );
	hp24Button->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap( "24dBoff" ) );

	PixmapButton *hp48Button = new PixmapButton( this , NULL );
	hp48Button->setModel( m_parameterWidget->getBandModels(0)->hp48 );
	hp48Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "48dB" ) );
	hp48Button->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap( "48dBoff" ) );

	//LP filter type
	PixmapButton *lp12Button = new PixmapButton( this , NULL );
	mainLayout->addWidget( lp12Button, 2, 1 );
	lp12Button->setModel( m_parameterWidget->getBandModels( 7 )->lp12 );
	lp12Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "12dB" ) );
	lp12Button->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap( "12dBoff" ) );

	PixmapButton *lp24Button = new PixmapButton( this , NULL );
	lp24Button->setModel( m_parameterWidget->getBandModels( 7 )->lp24 );
	lp24Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "24dB" ) );
	lp24Button->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap( "24dBoff" ) );

	PixmapButton *lp48Button = new PixmapButton( this , NULL );
	lp48Button->setModel( m_parameterWidget->getBandModels( 7 )->lp48 );
	lp48Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "48dB" ) );
	lp48Button->setInactiveGraphic(  PLUGIN_NAME::getIconPixmap( "48dBoff" ) );

	// the curve has to change its appearance
	connect( m_parameterWidget->getBandModels( 0 )->hp12 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateView()));
	connect( m_parameterWidget->getBandModels( 0 )->hp24 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateView()));
	connect( m_parameterWidget->getBandModels( 0 )->hp48 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateView()));

	connect( m_parameterWidget->getBandModels( 7 )->lp12 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateView()));
	connect( m_parameterWidget->getBandModels( 7 )->lp24 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateView()));
	connect( m_parameterWidget->getBandModels( 7 )->lp48 , SIGNAL ( dataChanged() ), m_parameterWidget, SLOT( updateView()));

	QVBoxLayout *hpGrpBtnLayout = new QVBoxLayout;
	hpGrpBtnLayout->addWidget( hp12Button );
	hpGrpBtnLayout->addWidget( hp24Button );
	hpGrpBtnLayout->addWidget( hp48Button );

	QVBoxLayout *lpGrpBtnLayout = new QVBoxLayout;
	lpGrpBtnLayout->addWidget( lp12Button );
	lpGrpBtnLayout->addWidget( lp24Button );
	lpGrpBtnLayout->addWidget( lp48Button );

	mainLayout->addLayout( hpGrpBtnLayout, 2, 1,  Qt::AlignCenter );
	mainLayout->addLayout( lpGrpBtnLayout, 2, 8,  Qt::AlignCenter );

	automatableButtonGroup *lpBtnGrp = new automatableButtonGroup(this,tr ( "lp grp" ) );
	lpBtnGrp->addButton( lp12Button );
	lpBtnGrp->addButton( lp24Button );
	lpBtnGrp->addButton( lp48Button );
	lpBtnGrp->setModel( &m_controls->m_lpTypeModel, false);

	automatableButtonGroup *hpBtnGrp = new automatableButtonGroup( this, tr( "hp grp" ) );
	hpBtnGrp->addButton( hp12Button );
	hpBtnGrp->addButton( hp24Button );
	hpBtnGrp->addButton( hp48Button );
	hpBtnGrp->setModel( &m_controls->m_hpTypeModel,false);

	mainLayout->setAlignment( Qt::AlignTop );

	for (int i = 0 ; i < 10; i++)
	{
		mainLayout->setColumnMinimumWidth(i, 50);
	}

	mainLayout->setAlignment( inGainFader, Qt::AlignHCenter );
	mainLayout->setAlignment( outGainFader, Qt::AlignHCenter );
	mainLayout->setRowMinimumHeight( 0,200 );
	mainLayout->setRowMinimumHeight( 1, 40 );
	mainLayout->setRowMinimumHeight(6,15);
	mainLayout->setContentsMargins( 0, 11, 0, 0 );
	mainLayout->setAlignment(inSpec, Qt::AlignCenter );
	mainLayout->setAlignment(outSpec, Qt::AlignCenter );

	QLabel *freqLabel = new QLabel( this );
	freqLabel->setText("- " + tr( "Frequency")+ " -" );
	freqLabel->move( 217 , 377 );

	QLabel *resLabel1 = new QLabel( this );
	resLabel1->setText("- " + tr( "Resonance")+ " -" );
	resLabel1->move( 62 , 444 );

	QLabel *resLabel2 = new QLabel( this );
	resLabel2->setText("- " + tr( "Resonance")+ " -" );
	resLabel2->move( 365 , 444 );

	QLabel *bandWidthLabel = new QLabel( this );
	bandWidthLabel->setText("- " + tr( "Bandwidth")+ " -" );
	bandWidthLabel->move( 215 , 444 );

	setLayout(mainLayout);
}




void EqControlsDialog::mouseDoubleClickEvent(QMouseEvent *event)
{
	m_originalHeight = parentWidget()->height() == 283 ? m_originalHeight : parentWidget()->height() ;
	parentWidget()->setFixedHeight( parentWidget()->height() == m_originalHeight ? 283 : m_originalHeight  );
	update();
}
