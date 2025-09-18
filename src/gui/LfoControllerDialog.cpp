/*
 * LfoControllerDialog.cpp - per-controller-specific view for changing a
 *                           controller's settings
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
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


#include "embed.h"


#include "FontHelper.h"
#include "LfoController.h"
#include "Knob.h"
#include "TempoSyncKnob.h"
#include "PixmapButton.h"
#include "SampleLoader.h"

namespace lmms::gui
{


const int CD_ENV_KNOBS_LBL_Y = 11;
const int CD_KNOB_X_SPACING = 32;

const int CD_LFO_SHAPES_X = 6;
const int CD_LFO_SHAPES_Y = CD_ENV_KNOBS_LBL_Y + 4;

const int CD_LFO_GRAPH_Y = CD_ENV_KNOBS_LBL_Y+3;
const int CD_LFO_CD_KNOB_Y = CD_LFO_GRAPH_Y-2;
const int CD_LFO_BASE_CD_KNOB_X = CD_LFO_SHAPES_X + 68;
const int CD_LFO_SPEED_CD_KNOB_X = CD_LFO_BASE_CD_KNOB_X+CD_KNOB_X_SPACING;
const int CD_LFO_AMOUNT_CD_KNOB_X = CD_LFO_SPEED_CD_KNOB_X+CD_KNOB_X_SPACING;
const int CD_LFO_PHASE_CD_KNOB_X = CD_LFO_AMOUNT_CD_KNOB_X+CD_KNOB_X_SPACING;
const int CD_LFO_MULTIPLIER_X = CD_LFO_PHASE_CD_KNOB_X+CD_KNOB_X_SPACING+3;

LfoControllerDialog::LfoControllerDialog( Controller * _model, QWidget * _parent ) :
	ControllerDialog( _model, _parent )
{
	QString title = tr( "LFO" );
	title.append( " (" );
	title.append( _model->name() );
	title.append( ")" );
	setWindowTitle( title );
	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setFixedSize( 240, 58 );
	
	m_baseKnob = new Knob(KnobType::Bright26, tr("BASE"), SMALL_FONT_SIZE, this);
	m_baseKnob->move( CD_LFO_BASE_CD_KNOB_X, CD_LFO_CD_KNOB_Y );
	m_baseKnob->setHintText( tr( "Base:" ), "" );

	m_speedKnob = new TempoSyncKnob(KnobType::Bright26, tr("FREQ"), SMALL_FONT_SIZE, this);
	m_speedKnob->move( CD_LFO_SPEED_CD_KNOB_X, CD_LFO_CD_KNOB_Y );
	m_speedKnob->setHintText( tr( "LFO frequency:" ), "" );

	m_amountKnob = new Knob(KnobType::Bright26, tr("AMNT"), SMALL_FONT_SIZE, this);
	m_amountKnob->move( CD_LFO_AMOUNT_CD_KNOB_X, CD_LFO_CD_KNOB_Y );
	m_amountKnob->setHintText( tr( "Modulation amount:" ), "" );

	m_phaseKnob = new Knob(KnobType::Bright26, tr("PHS"), SMALL_FONT_SIZE, this);
	m_phaseKnob->move( CD_LFO_PHASE_CD_KNOB_X, CD_LFO_CD_KNOB_Y );
	m_phaseKnob->setHintText( tr( "Phase offset:" ) , "" + tr( " degrees" ) );

	auto sin_wave_btn = new PixmapButton(this, nullptr);
	sin_wave_btn->move( CD_LFO_SHAPES_X, CD_LFO_SHAPES_Y );
	sin_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"sin_wave_active" ) );
	sin_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"sin_wave_inactive" ) );
	sin_wave_btn->setToolTip(
			tr( "Sine wave" ) );

	auto triangle_wave_btn = new PixmapButton(this, nullptr);
	triangle_wave_btn->move( CD_LFO_SHAPES_X + 15, CD_LFO_SHAPES_Y );
	triangle_wave_btn->setActiveGraphic(
		embed::getIconPixmap( "triangle_wave_active" ) );
	triangle_wave_btn->setInactiveGraphic(
		embed::getIconPixmap( "triangle_wave_inactive" ) );
	triangle_wave_btn->setToolTip(
			tr( "Triangle wave" ) );

	auto saw_wave_btn = new PixmapButton(this, nullptr);
	saw_wave_btn->move( CD_LFO_SHAPES_X + 30, CD_LFO_SHAPES_Y );
	saw_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"saw_wave_active" ) );
	saw_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"saw_wave_inactive" ) );
	saw_wave_btn->setToolTip(
			tr( "Saw wave" ) );

	auto sqr_wave_btn = new PixmapButton(this, nullptr);
	sqr_wave_btn->move( CD_LFO_SHAPES_X + 45, CD_LFO_SHAPES_Y );
	sqr_wave_btn->setActiveGraphic( embed::getIconPixmap(
					"square_wave_active" ) );
	sqr_wave_btn->setInactiveGraphic( embed::getIconPixmap(
					"square_wave_inactive" ) );
	sqr_wave_btn->setToolTip(
			tr( "Square wave" ) );

	auto moog_saw_wave_btn = new PixmapButton(this, nullptr);
	moog_saw_wave_btn->move( CD_LFO_SHAPES_X, CD_LFO_SHAPES_Y + 15 );
	moog_saw_wave_btn->setActiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_active" ) );
	moog_saw_wave_btn->setInactiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_inactive" ) );
	moog_saw_wave_btn->setToolTip(
			tr( "Moog saw wave" ) );

	auto exp_wave_btn = new PixmapButton(this, nullptr);
	exp_wave_btn->move( CD_LFO_SHAPES_X + 15, CD_LFO_SHAPES_Y + 15 );
	exp_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"exp_wave_active" ) );
	exp_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"exp_wave_inactive" ) );
	exp_wave_btn->setToolTip(
			tr( "Exponential wave" ) );

	auto white_noise_btn = new PixmapButton(this, nullptr);
	white_noise_btn->move( CD_LFO_SHAPES_X + 30, CD_LFO_SHAPES_Y + 15 );
	white_noise_btn->setActiveGraphic(
		embed::getIconPixmap( "white_noise_wave_active" ) );
	white_noise_btn->setInactiveGraphic(
		embed::getIconPixmap( "white_noise_wave_inactive" ) );
	white_noise_btn->setToolTip(
				tr( "White noise" ) );

	m_userWaveBtn = new PixmapButton( this, nullptr );
	m_userWaveBtn->move( CD_LFO_SHAPES_X + 45, CD_LFO_SHAPES_Y + 15 );
	m_userWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"usr_wave_active" ) );
	m_userWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"usr_wave_inactive" ) );
	connect( m_userWaveBtn,
					SIGNAL(doubleClicked()),
			this, SLOT(askUserDefWave()));
	m_userWaveBtn->setToolTip(
				tr( "User-defined shape.\nDouble click to pick a file." ) );
	
	m_waveBtnGrp = new AutomatableButtonGroup( this );
	m_waveBtnGrp->addButton( sin_wave_btn );
	m_waveBtnGrp->addButton( triangle_wave_btn );
	m_waveBtnGrp->addButton( saw_wave_btn );
	m_waveBtnGrp->addButton( sqr_wave_btn );
	m_waveBtnGrp->addButton( moog_saw_wave_btn );
	m_waveBtnGrp->addButton( exp_wave_btn );
	m_waveBtnGrp->addButton( white_noise_btn );
	m_waveBtnGrp->addButton( m_userWaveBtn );

	auto x1 = new PixmapButton(this, nullptr);
	x1->move(CD_LFO_MULTIPLIER_X, CD_LFO_SHAPES_Y + 7);
	x1->setActiveGraphic(embed::getIconPixmap("lfo_x1_active"));
	x1->setInactiveGraphic(embed::getIconPixmap("lfo_x1_inactive"));
	x1->setToolTip(tr("Multiply modulation frequency by 1"));

	auto x100 = new PixmapButton(this, nullptr);
	x100->move(CD_LFO_MULTIPLIER_X, CD_LFO_SHAPES_Y - 8);
	x100->setActiveGraphic(embed::getIconPixmap("lfo_x100_active"));
	x100->setInactiveGraphic(embed::getIconPixmap("lfo_x100_inactive"));
	x100->setToolTip(tr("Multiply modulation frequency by 100"));

	auto d100 = new PixmapButton(this, nullptr);
	d100->move(CD_LFO_MULTIPLIER_X, CD_LFO_SHAPES_Y + 22);
	d100->setActiveGraphic(embed::getIconPixmap("lfo_d100_active"));
	d100->setInactiveGraphic(embed::getIconPixmap("lfo_d100_inactive"));
	d100->setToolTip(tr("Divide modulation frequency by 100"));

	m_multiplierBtnGrp = new AutomatableButtonGroup( this );
	m_multiplierBtnGrp->addButton( x1 );
	m_multiplierBtnGrp->addButton( x100 );
	m_multiplierBtnGrp->addButton( d100 );


	setModel( _model );

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), embed::getIconPixmap( "lfo_controller_artwork" ) );
	setPalette( pal );

}



LfoControllerDialog::~LfoControllerDialog()
{
	m_userWaveBtn->disconnect( this );
}



void LfoControllerDialog::askUserDefWave()
{
	const auto fileName = SampleLoader::openWaveformFile();
	if (fileName.isEmpty()) { return; }

	auto lfoModel = dynamic_cast<LfoController*>(model());
	auto& buffer = lfoModel->m_userDefSampleBuffer;
	buffer = SampleLoader::createBufferFromFile(fileName);

	m_userWaveBtn->setToolTip(buffer->audioFile());
}



void LfoControllerDialog::contextMenuEvent( QContextMenuEvent * )
{
	/*
	QPointer<captionMenu> contextMenu = new captionMenu(
						getEffect()->publicName() );
		// ^ if this ever gets activated again:
		//   a47a36c86e4473af862df58d2b59f6cb8620b9ee says that publicName
		//   should be replaced by Model::displayName
	contextMenu->addAction( embed::getIconPixmap( "arp_up_on" ),
						tr( "Move &up" ),
						this, SLOT(moveUp()));
	contextMenu->addAction( embed::getIconPixmap( "arp_down_on" ),
						tr( "Move &down" ),
						this, SLOT(moveDown()));
	contextMenu->addSeparator();
	contextMenu->addAction( embed::getIconPixmap( "cancel" ),
						tr( "&Remove this plugin" ),
						this, SLOT(deletePlugin()));
	contextMenu->exec( QCursor::pos() );
	delete contextMenu;
	*/
}



void LfoControllerDialog::modelChanged()
{
	m_lfo = castModel<LfoController>();

	m_baseKnob->setModel( &m_lfo->m_baseModel );
	m_speedKnob->setModel( &m_lfo->m_speedModel );
	m_amountKnob->setModel( &m_lfo->m_amountModel );
	m_phaseKnob->setModel( &m_lfo->m_phaseModel );
	m_waveBtnGrp->setModel( &m_lfo->m_waveModel );
	m_multiplierBtnGrp->setModel( &m_lfo->m_multiplierModel );
}


} // namespace lmms::gui
