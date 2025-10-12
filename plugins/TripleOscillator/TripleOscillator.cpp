/*
 * TripleOscillator.cpp - powerful instrument with three oscillators
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QDomElement>
#include <QFileInfo>

#include "TripleOscillator.h"
#include "AudioEngine.h"
#include "AutomatableButton.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "PathUtil.h"
#include "PixmapButton.h"
#include "SampleBuffer.h"
#include "SampleLoader.h"
#include "Song.h"
#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT tripleoscillator_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"TripleOscillator",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"Three powerful oscillators you can modulate "
				"in several ways" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0110,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	nullptr,
	nullptr,
} ;

}



OscillatorObject::OscillatorObject( Model * _parent, int _idx ) :
	Model( _parent ),
	m_volumeModel( DefaultVolume / NUM_OF_OSCILLATORS, MinVolume,
			MaxVolume, 1.0f, this, tr( "Osc %1 volume" ).arg( _idx+1 ) ),
	m_panModel( DefaultPanning, PanningLeft, PanningRight, 1.0f, this,
			tr( "Osc %1 panning" ).arg( _idx+1 ) ),
	m_coarseModel( -_idx*KeysPerOctave,
			-2 * KeysPerOctave, 2 * KeysPerOctave, 1.0f, this,
			tr( "Osc %1 coarse detuning" ).arg( _idx+1 ) ),
	m_fineLeftModel( 0.0f, -100.0f, 100.0f, 1.0f, this,
			tr( "Osc %1 fine detuning left" ).arg( _idx+1 ) ),
	m_fineRightModel( 0.0f, -100.0f, 100.0f, 1.0f, this,
			tr( "Osc %1 fine detuning right" ).arg( _idx + 1 ) ),
	m_phaseOffsetModel( 0.0f, 0.0f, 360.0f, 1.0f, this,
			tr( "Osc %1 phase-offset" ).arg( _idx+1 ) ),
	m_stereoPhaseDetuningModel( 0.0f, 0.0f, 360.0f, 1.0f, this,
			tr( "Osc %1 stereo phase-detuning" ).arg( _idx+1 ) ),
	m_waveShapeModel( static_cast<int>(Oscillator::WaveShape::Sine), 0,
			Oscillator::NumWaveShapes-1, this,
			tr( "Osc %1 wave shape" ).arg( _idx+1 ) ),
	m_modulationAlgoModel( static_cast<int>(Oscillator::ModulationAlgo::SignalMix), 0,
				Oscillator::NumModulationAlgos-1, this,
				tr( "Modulation type %1" ).arg( _idx+1 ) ),
	m_useWaveTableModel(true),

	m_sampleBuffer( new SampleBuffer ),
	m_volumeLeft( 0.0f ),
	m_volumeRight( 0.0f ),
	m_detuningLeft( 0.0f ),
	m_detuningRight( 0.0f ),
	m_phaseOffsetLeft( 0.0f ),
	m_phaseOffsetRight( 0.0f ),
	m_useWaveTable( true )
{
	// Connect knobs with Oscillators' inputs
	connect( &m_volumeModel, SIGNAL( dataChanged() ),
					this, SLOT( updateVolume() ), Qt::DirectConnection );
	connect( &m_panModel, SIGNAL( dataChanged() ),
					this, SLOT( updateVolume() ), Qt::DirectConnection );
	updateVolume();

	connect( &m_coarseModel, SIGNAL( dataChanged() ),
				this, SLOT( updateDetuningLeft() ), Qt::DirectConnection );
	connect( &m_coarseModel, SIGNAL( dataChanged() ),
				this, SLOT( updateDetuningRight() ), Qt::DirectConnection );
	connect( &m_fineLeftModel, SIGNAL( dataChanged() ),
				this, SLOT( updateDetuningLeft() ), Qt::DirectConnection );
	connect( &m_fineRightModel, SIGNAL( dataChanged() ),
				this, SLOT( updateDetuningRight() ), Qt::DirectConnection );
	updateDetuningLeft();
	updateDetuningRight();

	connect( &m_phaseOffsetModel, SIGNAL( dataChanged() ),
			this, SLOT( updatePhaseOffsetLeft() ), Qt::DirectConnection );
	connect( &m_phaseOffsetModel, SIGNAL( dataChanged() ),
			this, SLOT( updatePhaseOffsetRight() ), Qt::DirectConnection );
	connect( &m_stereoPhaseDetuningModel, SIGNAL( dataChanged() ),
			this, SLOT( updatePhaseOffsetLeft() ), Qt::DirectConnection );
	connect ( &m_useWaveTableModel, SIGNAL(dataChanged()),
			this, SLOT( updateUseWaveTable()));

	updatePhaseOffsetLeft();
	updatePhaseOffsetRight();

}

void OscillatorObject::oscUserDefWaveDblClick()
{
	auto af = gui::SampleLoader::openWaveformFile();
	if( af != "" )
	{
		m_sampleBuffer = gui::SampleLoader::createBufferFromFile(af);
		m_userAntiAliasWaveTable = Oscillator::generateAntiAliasUserWaveTable(m_sampleBuffer.get());
		// TODO:
		//m_usrWaveBtn->setToolTip(m_sampleBuffer->audioFile());
	}
}




void OscillatorObject::updateVolume()
{
	if( m_panModel.value() >= 0.0f )
	{
		const float panningFactorLeft = 1.0f - m_panModel.value()
							/ (float)PanningRight;
		m_volumeLeft = panningFactorLeft * m_volumeModel.value() /
									100.0f;
		m_volumeRight = m_volumeModel.value() / 100.0f;
	}
	else
	{
		m_volumeLeft = m_volumeModel.value() / 100.0f;
		const float panningFactorRight = 1.0f + m_panModel.value()
							/ (float)PanningRight;
		m_volumeRight = panningFactorRight * m_volumeModel.value() /
									100.0f;
	}
}




void OscillatorObject::updateDetuningLeft()
{
	m_detuningLeft = std::exp2((m_coarseModel.value() * 100.0f + m_fineLeftModel.value()) / 1200.0f)
		/ Engine::audioEngine()->outputSampleRate();
}




void OscillatorObject::updateDetuningRight()
{
	m_detuningRight = std::exp2((m_coarseModel.value() * 100.0f + m_fineRightModel.value()) / 1200.0f)
		/ Engine::audioEngine()->outputSampleRate();
}




void OscillatorObject::updatePhaseOffsetLeft()
{
	m_phaseOffsetLeft = ( m_phaseOffsetModel.value() +
				m_stereoPhaseDetuningModel.value() ) / 360.0f;
}




void OscillatorObject::updatePhaseOffsetRight()
{
	m_phaseOffsetRight = m_phaseOffsetModel.value() / 360.0f;
}

void OscillatorObject::updateUseWaveTable()
{
	m_useWaveTable = m_useWaveTableModel.value();
}




TripleOscillator::TripleOscillator( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &tripleoscillator_plugin_descriptor )
{
	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		m_osc[i] = new OscillatorObject( this, i );

	}

	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ),
			this, SLOT( updateAllDetuning() ) );
}




void TripleOscillator::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		QString is = QString::number( i );
		m_osc[i]->m_volumeModel.saveSettings( _doc, _this, "vol" + is );
		m_osc[i]->m_panModel.saveSettings( _doc, _this, "pan" + is );
		m_osc[i]->m_coarseModel.saveSettings( _doc, _this, "coarse"
									+ is );
		m_osc[i]->m_fineLeftModel.saveSettings( _doc, _this, "finel" +
									is );
		m_osc[i]->m_fineRightModel.saveSettings( _doc, _this, "finer" +
									is );
		m_osc[i]->m_phaseOffsetModel.saveSettings( _doc, _this,
							"phoffset" + is );
		m_osc[i]->m_stereoPhaseDetuningModel.saveSettings( _doc, _this,
							"stphdetun" + is );
		m_osc[i]->m_waveShapeModel.saveSettings( _doc, _this,
							"wavetype" + is );
		m_osc[i]->m_modulationAlgoModel.saveSettings( _doc, _this,
					"modalgo" + QString::number( i+1 ) );
		m_osc[i]->m_useWaveTableModel.saveSettings( _doc, _this,
					"useWaveTable" + QString::number (i+1 ) );
		_this.setAttribute( "userwavefile" + is,
					m_osc[i]->m_sampleBuffer->audioFile() );
	}
}




void TripleOscillator::loadSettings( const QDomElement & _this )
{
	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		const QString is = QString::number( i );
		m_osc[i]->m_volumeModel.loadSettings( _this, "vol" + is );
		m_osc[i]->m_panModel.loadSettings( _this, "pan" + is );
		m_osc[i]->m_coarseModel.loadSettings( _this, "coarse" + is );
		m_osc[i]->m_fineLeftModel.loadSettings( _this, "finel" + is );
		m_osc[i]->m_fineRightModel.loadSettings( _this, "finer" + is );
		m_osc[i]->m_phaseOffsetModel.loadSettings( _this,
							"phoffset" + is );
		m_osc[i]->m_stereoPhaseDetuningModel.loadSettings( _this,
							"stphdetun" + is );
		m_osc[i]->m_waveShapeModel.loadSettings( _this, "wavetype" +
									is );
		m_osc[i]->m_modulationAlgoModel.loadSettings( _this,
					"modalgo" + QString::number( i+1 ) );
		m_osc[i]->m_useWaveTableModel.loadSettings( _this,
							"useWaveTable" + QString::number (i+1 ) );

		if (auto userWaveFile = _this.attribute("userwavefile" + is); !userWaveFile.isEmpty())
		{
			if (QFileInfo(PathUtil::toAbsolute(userWaveFile)).exists())
			{
				m_osc[i]->m_sampleBuffer = gui::SampleLoader::createBufferFromFile(userWaveFile);
				m_osc[i]->m_userAntiAliasWaveTable = Oscillator::generateAntiAliasUserWaveTable(m_osc[i]->m_sampleBuffer.get());
			}
			else { Engine::getSong()->collectError(QString("%1: %2").arg(tr("Sample not found"), userWaveFile)); }
		}
	}
}




QString TripleOscillator::nodeName() const
{
	return( tripleoscillator_plugin_descriptor.name );
}




void TripleOscillator::playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer )
{
	if (!_n->m_pluginData)
	{
		auto oscs_l = std::array<Oscillator*, NUM_OF_OSCILLATORS>{};
		auto oscs_r = std::array<Oscillator*, NUM_OF_OSCILLATORS>{};

		for( int i = NUM_OF_OSCILLATORS - 1; i >= 0; --i )
		{

			// the last oscs needs no sub-oscs...
			if( i == NUM_OF_OSCILLATORS - 1 )
			{
				oscs_l[i] = new Oscillator(
						&m_osc[i]->m_waveShapeModel,
						&m_osc[i]->m_modulationAlgoModel,
						_n->frequency(),
						m_osc[i]->m_detuningLeft,
						m_osc[i]->m_phaseOffsetLeft,
						m_osc[i]->m_volumeLeft );
				oscs_l[i]->setUseWaveTable(m_osc[i]->m_useWaveTable);
				oscs_r[i] = new Oscillator(
						&m_osc[i]->m_waveShapeModel,
						&m_osc[i]->m_modulationAlgoModel,
						_n->frequency(),
						m_osc[i]->m_detuningRight,
						m_osc[i]->m_phaseOffsetRight,
						m_osc[i]->m_volumeRight );
				oscs_r[i]->setUseWaveTable(m_osc[i]->m_useWaveTable);
			}
			else
			{
				oscs_l[i] = new Oscillator(
						&m_osc[i]->m_waveShapeModel,
						&m_osc[i]->m_modulationAlgoModel,
						_n->frequency(),
						m_osc[i]->m_detuningLeft,
						m_osc[i]->m_phaseOffsetLeft,
						m_osc[i]->m_volumeLeft,
						oscs_l[i + 1] );
				oscs_l[i]->setUseWaveTable(m_osc[i]->m_useWaveTable);
				oscs_r[i] = new Oscillator(
						&m_osc[i]->m_waveShapeModel,
						&m_osc[i]->m_modulationAlgoModel,
						_n->frequency(),
						m_osc[i]->m_detuningRight,
						m_osc[i]->m_phaseOffsetRight,
						m_osc[i]->m_volumeRight,
						oscs_r[i + 1] );
				oscs_r[i]->setUseWaveTable(m_osc[i]->m_useWaveTable);
			}

			oscs_l[i]->setUserWave( m_osc[i]->m_sampleBuffer );
			oscs_r[i]->setUserWave( m_osc[i]->m_sampleBuffer );
			oscs_l[i]->setUserAntiAliasWaveTable(m_osc[i]->m_userAntiAliasWaveTable);
			oscs_r[i]->setUserAntiAliasWaveTable(m_osc[i]->m_userAntiAliasWaveTable);
		}

		_n->m_pluginData = new oscPtr;
		static_cast<oscPtr *>( _n->m_pluginData )->oscLeft = oscs_l[0];
		static_cast< oscPtr *>( _n->m_pluginData )->oscRight =
								oscs_r[0];
	}

	Oscillator * osc_l = static_cast<oscPtr *>( _n->m_pluginData )->oscLeft;
	Oscillator * osc_r = static_cast<oscPtr *>( _n->m_pluginData )->oscRight;

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	osc_l->update( _working_buffer + offset, frames, 0 );
	osc_r->update( _working_buffer + offset, frames, 1 );

	applyFadeIn(_working_buffer, _n);
	applyRelease( _working_buffer, _n );
}




void TripleOscillator::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<Oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscLeft );
	delete static_cast<Oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscRight );
	delete static_cast<oscPtr *>( _n->m_pluginData );
}




gui::PluginView* TripleOscillator::instantiateView( QWidget * _parent )
{
	return new gui::TripleOscillatorView( this, _parent );
}




void TripleOscillator::updateAllDetuning()
{
	for (const auto& osc : m_osc)
	{
		osc->updateDetuningLeft();
		osc->updateDetuningRight();
	}
}



namespace gui
{


class TripleOscKnob : public Knob
{
public:
	TripleOscKnob( QWidget * _parent ) :
			Knob( KnobType::Styled, _parent )
	{
		setFixedSize( 28, 35 );
	}
};

// 82, 109


TripleOscillatorView::TripleOscillatorView( Instrument * _instrument,
							QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	const int mod_x = 66;
	const int mod1_y = 58;
	const int mod2_y = 75;
	const int osc_y = 109;
	const int osc_h = 52;

	// TODO: clean rewrite using layouts and all that...
	auto pm_osc1_btn = new PixmapButton(this, nullptr);
	pm_osc1_btn->move( mod_x, mod1_y );
	pm_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"pm_active" ) );
	pm_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"pm_inactive" ) );
	pm_osc1_btn->setToolTip(tr("Modulate phase of oscillator 1 by oscillator 2"));

	auto am_osc1_btn = new PixmapButton(this, nullptr);
	am_osc1_btn->move( mod_x + 35, mod1_y );
	am_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"am_active" ) );
	am_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"am_inactive" ) );
	am_osc1_btn->setToolTip(tr("Modulate amplitude of oscillator 1 by oscillator 2"));

	auto mix_osc1_btn = new PixmapButton(this, nullptr);
	mix_osc1_btn->move( mod_x + 70, mod1_y );
	mix_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_active" ) );
	mix_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_inactive" ) );
	mix_osc1_btn->setToolTip(tr("Mix output of oscillators 1 & 2"));

	auto sync_osc1_btn = new PixmapButton(this, nullptr);
	sync_osc1_btn->move( mod_x + 105, mod1_y );
	sync_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_active" ) );
	sync_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_inactive" ) );
	sync_osc1_btn->setToolTip(tr("Synchronize oscillator 1 with "
							"oscillator 2" ) );

	auto fm_osc1_btn = new PixmapButton(this, nullptr);
	fm_osc1_btn->move( mod_x + 140, mod1_y );
	fm_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"fm_active" ) );
	fm_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fm_inactive" ) );
	fm_osc1_btn->setToolTip(tr("Modulate frequency of oscillator 1 by oscillator 2"));

	m_mod1BtnGrp = new AutomatableButtonGroup( this );
	m_mod1BtnGrp->addButton( pm_osc1_btn );
	m_mod1BtnGrp->addButton( am_osc1_btn );
	m_mod1BtnGrp->addButton( mix_osc1_btn );
	m_mod1BtnGrp->addButton( sync_osc1_btn );
	m_mod1BtnGrp->addButton( fm_osc1_btn );

	auto pm_osc2_btn = new PixmapButton(this, nullptr);
	pm_osc2_btn->move( mod_x, mod2_y );
	pm_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"pm_active" ) );
	pm_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"pm_inactive" ) );
	pm_osc2_btn->setToolTip(tr("Modulate phase of oscillator 2 by oscillator 3"));

	auto am_osc2_btn = new PixmapButton(this, nullptr);
	am_osc2_btn->move( mod_x + 35, mod2_y );
	am_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"am_active" ) );
	am_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"am_inactive" ) );
	am_osc2_btn->setToolTip(tr("Modulate amplitude of oscillator 2 by oscillator 3"));

	auto mix_osc2_btn = new PixmapButton(this, nullptr);
	mix_osc2_btn->move( mod_x + 70, mod2_y );
	mix_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_active" ) );
	mix_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_inactive" ) );
	mix_osc2_btn->setToolTip(tr("Mix output of oscillators 2 & 3"));

	auto sync_osc2_btn = new PixmapButton(this, nullptr);
	sync_osc2_btn->move( mod_x + 105, mod2_y );
	sync_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_active" ) );
	sync_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_inactive" ) );
	sync_osc2_btn->setToolTip(tr("Synchronize oscillator 2 with oscillator 3"));

	auto fm_osc2_btn = new PixmapButton(this, nullptr);
	fm_osc2_btn->move( mod_x + 140, mod2_y );
	fm_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"fm_active" ) );
	fm_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fm_inactive" ) );
	fm_osc2_btn->setToolTip(tr("Modulate frequency of oscillator 2 by oscillator 3"));

	m_mod2BtnGrp = new AutomatableButtonGroup( this );

	m_mod2BtnGrp->addButton( pm_osc2_btn );
	m_mod2BtnGrp->addButton( am_osc2_btn );
	m_mod2BtnGrp->addButton( mix_osc2_btn );
	m_mod2BtnGrp->addButton( sync_osc2_btn );
	m_mod2BtnGrp->addButton( fm_osc2_btn );


	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		int knob_y = osc_y + i * osc_h;

		// setup volume-knob
		auto vk = new Knob(KnobType::Styled, this);
		vk->setVolumeKnob( true );
		vk->setFixedSize( 28, 35 );
		vk->move( 6, knob_y );
		vk->setHintText( tr( "Osc %1 volume:" ).arg(
							 i+1 ), "%" );

		// setup panning-knob
		Knob * pk = new TripleOscKnob( this );
		pk->move( 35, knob_y );
		pk->setHintText( tr("Osc %1 panning:").arg( i + 1 ), "" );

		// setup coarse-knob
		Knob * ck = new TripleOscKnob( this );
		ck->move( 82, knob_y );
		ck->setHintText( tr( "Osc %1 coarse detuning:" ).arg( i + 1 )
						 , " " + tr( "semitones" ) );

		// setup knob for left fine-detuning
		Knob * flk = new TripleOscKnob( this );
		flk->move( 111, knob_y );
		flk->setHintText( tr( "Osc %1 fine detuning left:" ).
						  arg( i + 1 ),
							" " + tr( "cents" ) );

		// setup knob for right fine-detuning
		Knob * frk = new TripleOscKnob( this );
		frk->move( 140, knob_y );
		frk->setHintText( tr( "Osc %1 fine detuning right:" ).
						  arg( i + 1 ),
							" " + tr( "cents" ) );

		// setup phase-offset-knob
		Knob * pok = new TripleOscKnob( this );
		pok->move( 188, knob_y );
		pok->setHintText( tr( "Osc %1 phase-offset:" ).
						  arg( i + 1 ),
							" " + tr( "degrees" ) );

		// setup stereo-phase-detuning-knob
		Knob * spdk = new TripleOscKnob( this );
		spdk->move( 217, knob_y );
		spdk->setHintText( tr("Osc %1 stereo phase-detuning:" ).
						arg( i + 1 ),
							" " + tr( "degrees" ) );

		int btn_y = 96 + i * osc_h;

		auto sin_wave_btn = new PixmapButton(this, nullptr);
		sin_wave_btn->move( 128, btn_y );
		sin_wave_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sin_shape_active" ) );
		sin_wave_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sin_shape_inactive" ) );
		sin_wave_btn->setToolTip(
				tr( "Sine wave" ) );

		auto triangle_wave_btn = new PixmapButton(this, nullptr);
		triangle_wave_btn->move( 143, btn_y );
		triangle_wave_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "triangle_shape_active" ) );
		triangle_wave_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "triangle_shape_inactive" ) );
		triangle_wave_btn->setToolTip(
				tr( "Triangle wave") );

		auto saw_wave_btn = new PixmapButton(this, nullptr);
		saw_wave_btn->move( 158, btn_y );
		saw_wave_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"saw_shape_active" ) );
		saw_wave_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"saw_shape_inactive" ) );
		saw_wave_btn->setToolTip(
				tr( "Saw wave" ) );

		auto sqr_wave_btn = new PixmapButton(this, nullptr);
		sqr_wave_btn->move( 173, btn_y );
		sqr_wave_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"square_shape_active" ) );
		sqr_wave_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"square_shape_inactive" ) );
		sqr_wave_btn->setToolTip(
				tr( "Square wave" ) );

		auto moog_saw_wave_btn = new PixmapButton(this, nullptr);
		moog_saw_wave_btn->move( 188, btn_y );
		moog_saw_wave_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "moog_saw_shape_active" ) );
		moog_saw_wave_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "moog_saw_shape_inactive" ) );
		moog_saw_wave_btn->setToolTip(
				tr( "Moog-like saw wave" ) );

		auto exp_wave_btn = new PixmapButton(this, nullptr);
		exp_wave_btn->move( 203, btn_y );
		exp_wave_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"exp_shape_active" ) );
		exp_wave_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"exp_shape_inactive" ) );
		exp_wave_btn->setToolTip(
				tr( "Exponential wave" ) );

		auto white_noise_btn = new PixmapButton(this, nullptr);
		white_noise_btn->move( 218, btn_y );
		white_noise_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "white_noise_shape_active" ) );
		white_noise_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "white_noise_shape_inactive" ) );
		white_noise_btn->setToolTip(
				tr( "White noise" ) );

		auto uwb = new PixmapButton(this, nullptr);
		uwb->move( 233, btn_y );
		uwb->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"usr_shape_active" ) );
		uwb->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"usr_shape_inactive" ) );
		uwb->setToolTip(tr("User-defined wave"));

		auto uwt = new PixmapButton(this, nullptr);
		uwt->move( 110, btn_y );
		uwt->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"wavetable_active" ) );
		uwt->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"wavetable_inactive" ) );
		uwt->setCheckable(true);
		uwt->setToolTip(tr("Use alias-free wavetable oscillators."));

		auto wsbg = new AutomatableButtonGroup(this);

		wsbg->addButton( sin_wave_btn );
		wsbg->addButton( triangle_wave_btn );
		wsbg->addButton( saw_wave_btn );
		wsbg->addButton( sqr_wave_btn );
		wsbg->addButton( moog_saw_wave_btn );
		wsbg->addButton( exp_wave_btn );
		wsbg->addButton( white_noise_btn );
		wsbg->addButton( uwb );


		m_oscKnobs[i] = OscillatorKnobs( vk, pk, ck, flk, frk, pok,
							spdk, uwb, wsbg, uwt );
	}
}




void TripleOscillatorView::modelChanged()
{
	auto t = castModel<TripleOscillator>();
	m_mod1BtnGrp->setModel( &t->m_osc[0]->m_modulationAlgoModel );
	m_mod2BtnGrp->setModel( &t->m_osc[1]->m_modulationAlgoModel );

	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		m_oscKnobs[i].m_volKnob->setModel(
					&t->m_osc[i]->m_volumeModel );
		m_oscKnobs[i].m_panKnob->setModel(
					&t->m_osc[i]->m_panModel );
		m_oscKnobs[i].m_coarseKnob->setModel(
					&t->m_osc[i]->m_coarseModel );
		m_oscKnobs[i].m_fineLeftKnob->setModel(
					&t->m_osc[i]->m_fineLeftModel );
		m_oscKnobs[i].m_fineRightKnob->setModel(
					&t->m_osc[i]->m_fineRightModel );
		m_oscKnobs[i].m_phaseOffsetKnob->setModel(
					&t->m_osc[i]->m_phaseOffsetModel );
		m_oscKnobs[i].m_stereoPhaseDetuningKnob->setModel(
				&t->m_osc[i]->m_stereoPhaseDetuningModel );
		m_oscKnobs[i].m_waveShapeBtnGrp->setModel(
					&t->m_osc[i]->m_waveShapeModel );
		m_oscKnobs[i].m_multiBandWaveTableButton->setModel(
					&t->m_osc[i]->m_useWaveTableModel );

		connect( m_oscKnobs[i].m_userWaveButton,
						SIGNAL( doubleClicked() ),
				t->m_osc[i], SLOT( oscUserDefWaveDblClick() ) );
	}
}


} // namespace gui


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model* model, void * )
{
	return new TripleOscillator( static_cast<InstrumentTrack *>( model ) );
}

}



} // namespace lmms
