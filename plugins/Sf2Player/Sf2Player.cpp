/*
 * Sf2Player.cpp - a soundfont2 player using fluidSynth
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "Sf2Player.h"

#include <fluidsynth.h>
#include <QDebug>
#include <QDomElement>
#include <QLabel>

#include "ArrayVector.h"
#include "AudioEngine.h"
#include "ConfigManager.h"
#include "FileDialog.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "InstrumentPlayHandle.h"
#include "Knob.h"
#include "NotePlayHandle.h"
#include "PathUtil.h"
#include "PixmapButton.h"
#include "Song.h"
#include "fluidsynthshims.h"

#include "PatchesDialog.h"
#include "LcdSpinBox.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT sf2player_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Sf2 Player",
	QT_TRANSLATE_NOOP( "PluginBrowser", "Player for SoundFont files" ),
	"Paul Giblock <drfaygo/at/gmail/dot/com>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	"sf2,sf3",
	"soundfontfile",
	nullptr,
} ;

}

/**
 * A non-owning reference to a single FluidSynth voice. Captures some initial
 * properties of the referenced voice to help manage changes to it over time.
 */
class FluidVoice
{
public:
	//! Create a reference to the voice currently pointed at by `voice`.
	explicit FluidVoice(fluid_voice_t* voice) :
		m_voice{voice},
		m_id{fluid_voice_get_id(voice)},
		m_coarseTune{fluid_voice_gen_get(voice, GEN_COARSETUNE)}
	{ }

	//! Get a pointer to the referenced voice.
	fluid_voice_t* get() const noexcept { return m_voice; }

	//! Get the original coarse tuning of the referenced voice.
	float coarseTune() const noexcept { return m_coarseTune; }

	//! Test whether this object still refers to the original voice.
	bool isValid() const
	{
		return fluid_voice_get_id(m_voice) == m_id && fluid_voice_is_playing(m_voice);
	}

private:
	fluid_voice_t* m_voice;
	unsigned int m_id;
	float m_coarseTune;
};

struct Sf2PluginData
{
	int midiNote;
	int lastPanning;
	float lastVelocity;
	// The soundfonts I checked used at most two voices per note, so space for
	// four should be safe. This may need to be increased if a soundfont with
	// more voices per note is found.
	ArrayVector<FluidVoice, 4> fluidVoices;
	bool isNew;
	f_cnt_t offset;
	bool noteOffSent;
	panning_t panning;
};



Sf2Instrument::Sf2Instrument( InstrumentTrack * _instrument_track ) :
	Instrument(_instrument_track, &sf2player_plugin_descriptor, nullptr, Flag::IsSingleStreamed),
	m_srcState( nullptr ),
	m_synth(nullptr),
	m_font( nullptr ),
	m_fontId( 0 ),
	m_filename( "" ),
	m_lastMidiPitch( -1 ),
	m_lastMidiPitchRange( -1 ),
	m_channel( 1 ),
	m_bankNum( 0, 0, 999, this, tr("Bank") ),
	m_patchNum( 0, 0, 127, this, tr("Patch") ),
	m_gain( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Gain" ) ),
	m_reverbOn( false, this, tr( "Reverb" ) ),
	m_reverbRoomSize( FLUID_REVERB_DEFAULT_ROOMSIZE, 0, 1.0, 0.01f, this, tr( "Reverb room size" ) ),
	m_reverbDamping(FLUID_REVERB_DEFAULT_DAMP, 0, 1.f, 0.01f, this, tr("Reverb damping")),
	m_reverbWidth( FLUID_REVERB_DEFAULT_WIDTH, 0, 1.0, 0.01f, this, tr( "Reverb width" ) ),
	m_reverbLevel( FLUID_REVERB_DEFAULT_LEVEL, 0, 1.0, 0.01f, this, tr( "Reverb level" ) ),
	m_chorusOn( false, this, tr( "Chorus" ) ),
	m_chorusNum( FLUID_CHORUS_DEFAULT_N, 0, 10.0, 1.0, this, tr( "Chorus voices" ) ),
	m_chorusLevel(FLUID_CHORUS_DEFAULT_LEVEL, 0, 10.f, 0.01f, this, tr("Chorus level")),
	m_chorusSpeed(FLUID_CHORUS_DEFAULT_SPEED, 0.29f, 5.f, 0.01f, this, tr("Chorus speed")),
	m_chorusDepth(FLUID_CHORUS_DEFAULT_DEPTH, 0, 46.f, 0.05f, this, tr("Chorus depth"))
{


#if QT_VERSION_CHECK(FLUIDSYNTH_VERSION_MAJOR, FLUIDSYNTH_VERSION_MINOR, FLUIDSYNTH_VERSION_MICRO) >= QT_VERSION_CHECK(1,1,9)
	// Deactivate all audio drivers in fluidsynth
	const char *none[] = { nullptr };
	fluid_audio_driver_register( none );
#endif
	m_settings = new_fluid_settings();

	//fluid_settings_setint( m_settings, (char *) "audio.period-size", engine::audioEngine()->framesPerPeriod() );

	// This sets up m_synth and updates reverb/chorus/gain
	reloadSynth();

#if FLUIDSYNTH_VERSION_MAJOR >= 2
	// Get the default values from the setting
	double settingVal;

	fluid_settings_getnum_default(m_settings, "synth.reverb.room-size", &settingVal);
	m_reverbRoomSize.setInitValue(settingVal);
	fluid_settings_getnum_default(m_settings, "synth.reverb.damping", &settingVal);
	m_reverbDamping.setInitValue(settingVal);
	fluid_settings_getnum_default(m_settings, "synth.reverb.width", &settingVal);
	m_reverbWidth.setInitValue(settingVal);
	fluid_settings_getnum_default(m_settings, "synth.reverb.level", &settingVal);
	m_reverbLevel.setInitValue(settingVal);

	fluid_settings_getnum_default(m_settings, "synth.chorus.nr", &settingVal);
	m_chorusNum.setInitValue(settingVal);
	fluid_settings_getnum_default(m_settings, "synth.chorus.level", &settingVal);
	m_chorusLevel.setInitValue(settingVal);
	fluid_settings_getnum_default(m_settings, "synth.chorus.speed", &settingVal);
	m_chorusSpeed.setInitValue(settingVal);
	fluid_settings_getnum_default(m_settings, "synth.chorus.depth", &settingVal);
	m_chorusDepth.setInitValue(settingVal);
#endif

	// FIXME: there's no good way to tell if we're loading a preset or an empty instrument
	// We rely on instantiate() to load the default soundfont for new instruments,
	// but we don't need that when loading a project/preset/preview
	if (!Engine::getSong()->isLoadingProject() && !instrumentTrack()->isPreviewMode())
	{
		loadFile(ConfigManager::inst()->sf2File());
	}

	connect( &m_bankNum, SIGNAL( dataChanged() ), this, SLOT( updatePatch() ) );
	connect( &m_patchNum, SIGNAL( dataChanged() ), this, SLOT( updatePatch() ) );

	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(reloadSynth()));

	// Gain
	connect( &m_gain, SIGNAL( dataChanged() ), this, SLOT( updateGain() ) );

	// Reverb
	connect( &m_reverbOn, SIGNAL( dataChanged() ), this, SLOT( updateReverbOn() ) );
	connect( &m_reverbRoomSize, SIGNAL( dataChanged() ), this, SLOT( updateReverb() ) );
	connect( &m_reverbDamping, SIGNAL( dataChanged() ), this, SLOT( updateReverb() ) );
	connect( &m_reverbWidth, SIGNAL( dataChanged() ), this, SLOT( updateReverb() ) );
	connect( &m_reverbLevel, SIGNAL( dataChanged() ), this, SLOT( updateReverb() ) );

	// Chorus
	connect( &m_chorusOn, SIGNAL( dataChanged() ), this, SLOT( updateChorusOn() ) );
	connect( &m_chorusNum, SIGNAL( dataChanged() ), this, SLOT( updateChorus() ) );
	connect( &m_chorusLevel, SIGNAL( dataChanged() ), this, SLOT( updateChorus() ) );
	connect( &m_chorusSpeed, SIGNAL( dataChanged() ), this, SLOT( updateChorus() ) );
	connect( &m_chorusDepth, SIGNAL( dataChanged() ), this, SLOT( updateChorus() ) );
	
	// Microtuning
	connect(Engine::getSong(), &Song::scaleListChanged, this, &Sf2Instrument::updateTuning);
	connect(Engine::getSong(), &Song::keymapListChanged, this, &Sf2Instrument::updateTuning);
	connect(instrumentTrack()->microtuner()->enabledModel(), &Model::dataChanged, this, &Sf2Instrument::updateTuning, Qt::DirectConnection);
	connect(instrumentTrack()->microtuner()->scaleModel(), &Model::dataChanged, this, &Sf2Instrument::updateTuning, Qt::DirectConnection);
	connect(instrumentTrack()->microtuner()->keymapModel(), &Model::dataChanged, this, &Sf2Instrument::updateTuning, Qt::DirectConnection);
	connect(instrumentTrack()->microtuner()->keyRangeImportModel(), &Model::dataChanged, this, &Sf2Instrument::updateTuning, Qt::DirectConnection);
	connect(instrumentTrack()->baseNoteModel(), &Model::dataChanged, this, &Sf2Instrument::updateTuning, Qt::DirectConnection);

	auto iph = new InstrumentPlayHandle(this, _instrument_track);
	Engine::audioEngine()->addPlayHandle( iph );
}



Sf2Instrument::~Sf2Instrument()
{
	Engine::audioEngine()->removePlayHandlesOfTypes( instrumentTrack(),
				PlayHandle::Type::NotePlayHandle
				| PlayHandle::Type::InstrumentPlayHandle );
	freeFont();
	delete_fluid_synth( m_synth );
	delete_fluid_settings( m_settings );
	if( m_srcState != nullptr )
	{
		src_delete( m_srcState );
	}

}



void Sf2Instrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "src", m_filename );
	m_patchNum.saveSettings( _doc, _this, "patch" );
	m_bankNum.saveSettings( _doc, _this, "bank" );

	m_gain.saveSettings( _doc, _this, "gain" );

	m_reverbOn.saveSettings( _doc, _this, "reverbOn" );
	m_reverbRoomSize.saveSettings( _doc, _this, "reverbRoomSize" );
	m_reverbDamping.saveSettings( _doc, _this, "reverbDamping" );
	m_reverbWidth.saveSettings( _doc, _this, "reverbWidth" );
	m_reverbLevel.saveSettings( _doc, _this, "reverbLevel" );

	m_chorusOn.saveSettings( _doc, _this, "chorusOn" );
	m_chorusNum.saveSettings( _doc, _this, "chorusNum" );
	m_chorusLevel.saveSettings( _doc, _this, "chorusLevel" );
	m_chorusSpeed.saveSettings( _doc, _this, "chorusSpeed" );
	m_chorusDepth.saveSettings( _doc, _this, "chorusDepth" );
}




void Sf2Instrument::loadSettings( const QDomElement & _this )
{
	openFile( _this.attribute( "src" ), false );
	m_patchNum.loadSettings( _this, "patch" );
	m_bankNum.loadSettings( _this, "bank" );

	m_gain.loadSettings( _this, "gain" );

	m_reverbOn.loadSettings( _this, "reverbOn" );
	m_reverbRoomSize.loadSettings( _this, "reverbRoomSize" );
	m_reverbDamping.loadSettings( _this, "reverbDamping" );
	m_reverbWidth.loadSettings( _this, "reverbWidth" );
	m_reverbLevel.loadSettings( _this, "reverbLevel" );

	m_chorusOn.loadSettings( _this, "chorusOn" );
	m_chorusNum.loadSettings( _this, "chorusNum" );
	m_chorusLevel.loadSettings( _this, "chorusLevel" );
	m_chorusSpeed.loadSettings( _this, "chorusSpeed" );
	m_chorusDepth.loadSettings( _this, "chorusDepth" );

}




void Sf2Instrument::loadFile( const QString & _file )
{
	if( !_file.isEmpty() && QFileInfo( _file ).exists() )
	{
		openFile( _file, false );
	}

	// setting the first bank and patch number that is found
	auto sSoundCount = ::fluid_synth_sfcount( m_synth );
	for ( int i = 0; i < sSoundCount; ++i ) {
		int iBank = 0;
		int iProg = 0;
		fluid_sfont_t *pSoundFont = ::fluid_synth_get_sfont( m_synth, i );

		if ( pSoundFont ) {
#ifdef CONFIG_FLUID_BANK_OFFSET
			int iBankOff = ::fluid_synth_get_bank_offset( m_synth, fluid_sfont_get_id( pSoundFont ) );
#endif

			fluid_sfont_iteration_start( pSoundFont );
#if FLUIDSYNTH_VERSION_MAJOR < 2
			fluid_preset_t preset;
			fluid_preset_t *pCurPreset = &preset;
#else
			fluid_preset_t *pCurPreset = nullptr;
#endif

			if ( ( pCurPreset = fluid_sfont_iteration_next_wrapper( pSoundFont, pCurPreset ) ) ) {
				iBank = fluid_preset_get_banknum( pCurPreset );
				iProg = fluid_preset_get_num( pCurPreset );

#ifdef CONFIG_FLUID_BANK_OFFSET
				iBank += iBankOff;
#endif

				::fluid_synth_bank_select( m_synth, 1, iBank );
				::fluid_synth_program_change( m_synth, 1, iProg );
				m_bankNum.setValue( iBank );
				m_patchNum.setValue ( iProg );
				break;
			}
		}
	}
}




AutomatableModel * Sf2Instrument::childModel( const QString & _modelName )
{
	if( _modelName == "bank" )
	{
		return &m_bankNum;
	}
	else if( _modelName == "patch" )
	{
		return &m_patchNum;
	}
	qCritical() << "requested unknown model " << _modelName;
	return nullptr;
}



QString Sf2Instrument::nodeName() const
{
	return sf2player_plugin_descriptor.name;
}




void Sf2Instrument::freeFont()
{
	m_synthMutex.lock();

	if (m_font != nullptr)
	{
		fluid_synth_sfunload(m_synth, m_fontId, true);
		m_font = nullptr;
	}

	m_synthMutex.unlock();
}



void Sf2Instrument::openFile( const QString & _sf2File, bool updateTrackName )
{
	emit fileLoading();

	// Used for loading file
	char * sf2Ascii = qstrdup( qPrintable( PathUtil::toAbsolute( _sf2File ) ) );
	QString relativePath = PathUtil::toShortestRelative( _sf2File );

	// free the soundfont if one is selected
	freeFont();

	m_synthMutex.lock();

	bool loaded = false;
	if (fluid_is_soundfont(sf2Ascii))
	{
		m_fontId = fluid_synth_sfload(m_synth, sf2Ascii, true);

		if (fluid_synth_sfcount(m_synth) > 0)
		{
			// Grab this sf from the top of the stack and add to list
			m_font = fluid_synth_get_sfont(m_synth, 0);
			loaded = true;
		}
	}

	if (!loaded)
	{
		collectErrorForUI(Sf2Instrument::tr("A soundfont %1 could not be loaded.").arg(QFileInfo(_sf2File).baseName()));
	}

	m_synthMutex.unlock();

	if( m_fontId >= 0 )
	{
		// Don't reset patch/bank, so that it isn't cleared when
		// someone resolves a missing file
		//m_patchNum.setValue( 0 );
		//m_bankNum.setValue( 0 );
		m_filename = relativePath;

		emit fileChanged();
	}

	delete[] sf2Ascii;

	if( updateTrackName || instrumentTrack()->displayName() == displayName() )
	{
		instrumentTrack()->setName( PathUtil::cleanName( _sf2File ) );
	}

	updatePatch();
}




void Sf2Instrument::updatePatch()
{
	if( m_bankNum.value() >= 0 && m_patchNum.value() >= 0 )
	{
		fluid_synth_program_select( m_synth, m_channel, m_fontId,
				m_bankNum.value(), m_patchNum.value() );
	}
}




QString Sf2Instrument::getCurrentPatchName()
{
	int iBankSelected = m_bankNum.value();
	int iProgSelected = m_patchNum.value();

	// For all soundfonts (in reversed stack order) fill the available programs...
	int cSoundFonts = ::fluid_synth_sfcount( m_synth );
	for( int i = 0; i < cSoundFonts; i++ )
	{
		fluid_sfont_t *pSoundFont = fluid_synth_get_sfont( m_synth, i );
		if ( pSoundFont )
		{
#ifdef CONFIG_FLUID_BANK_OFFSET
			int iBankOffset =
				fluid_synth_get_bank_offset(
						m_synth, fluid_sfont_get_id(pSoundFont) );
#endif
			fluid_sfont_iteration_start( pSoundFont );
#if FLUIDSYNTH_VERSION_MAJOR < 2
			fluid_preset_t preset;
			fluid_preset_t *pCurPreset = &preset;
#else
			fluid_preset_t *pCurPreset = nullptr;
#endif
			while ((pCurPreset = fluid_sfont_iteration_next_wrapper(pSoundFont, pCurPreset)))
			{
				int iBank = fluid_preset_get_banknum( pCurPreset );
#ifdef CONFIG_FLUID_BANK_OFFSET
				iBank += iBankOffset;
#endif
				int iProg = fluid_preset_get_num( pCurPreset );
				if( iBank == iBankSelected && iProg ==
								iProgSelected )
				{
					return fluid_preset_get_name( pCurPreset );
				}
			}
		}
	}
	return "";
}




void Sf2Instrument::updateGain()
{
	fluid_synth_set_gain( m_synth, m_gain.value() );
}

#define FLUIDSYNTH_VERSION_HEX ((FLUIDSYNTH_VERSION_MAJOR << 16) \
	| (FLUIDSYNTH_VERSION_MINOR << 8) \
	| FLUIDSYNTH_VERSION_MICRO)
#define USE_NEW_EFFECT_API (FLUIDSYNTH_VERSION_HEX >= 0x020200)

void Sf2Instrument::updateReverbOn()
{
#if USE_NEW_EFFECT_API
	fluid_synth_reverb_on(m_synth, -1, m_reverbOn.value() ? 1 : 0);
#else
	fluid_synth_set_reverb_on(m_synth, m_reverbOn.value() ? 1 : 0);
#endif
}

void Sf2Instrument::updateReverb()
{
#if USE_NEW_EFFECT_API
	fluid_synth_set_reverb_group_roomsize(m_synth, -1, m_reverbRoomSize.value());
	fluid_synth_set_reverb_group_damp(m_synth, -1, m_reverbDamping.value());
	fluid_synth_set_reverb_group_width(m_synth, -1, m_reverbWidth.value());
	fluid_synth_set_reverb_group_level(m_synth, -1, m_reverbLevel.value());
#else
	fluid_synth_set_reverb(m_synth, m_reverbRoomSize.value(),
			m_reverbDamping.value(), m_reverbWidth.value(),
			m_reverbLevel.value());
#endif
}

void Sf2Instrument::updateChorusOn()
{
#if USE_NEW_EFFECT_API
	fluid_synth_chorus_on(m_synth, -1, m_chorusOn.value() ? 1 : 0);
#else
	fluid_synth_set_chorus_on(m_synth, m_chorusOn.value() ? 1 : 0);
#endif
}

void Sf2Instrument::updateChorus()
{
#if USE_NEW_EFFECT_API
	fluid_synth_set_chorus_group_nr(m_synth, -1, static_cast<int>(m_chorusNum.value()));
	fluid_synth_set_chorus_group_level(m_synth, -1, m_chorusLevel.value());
	fluid_synth_set_chorus_group_speed(m_synth, -1, m_chorusSpeed.value());
	fluid_synth_set_chorus_group_depth(m_synth, -1, m_chorusDepth.value());
	fluid_synth_set_chorus_group_type(m_synth, -1, FLUID_CHORUS_MOD_SINE);
#else
	fluid_synth_set_chorus(m_synth, static_cast<int>(m_chorusNum.value()),
			m_chorusLevel.value(), m_chorusSpeed.value(),
			m_chorusDepth.value(), FLUID_CHORUS_MOD_SINE);
#endif
}

void Sf2Instrument::updateTuning()
{
	if (instrumentTrack()->microtuner()->enabledModel()->value())
	{
		auto centArray = std::array<double, 128>{};
		double lowestHz = std::exp2(-69. / 12.) * 440.; // Frequency of MIDI note 0, which is approximately 8.175798916 Hz
		for (int i = 0; i < 128; ++i)
		{
			// Get desired Hz of note
			double noteHz = instrumentTrack()->microtuner()->keyToFreq(i, DefaultBaseKey);
			// Convert Hz to cents
			centArray[i] = noteHz == 0. ? 0. : 1200. * log2(noteHz / lowestHz);
		}

		fluid_synth_activate_key_tuning(m_synth, 0, 0, "", centArray.data(), true);
		for (int chan = 0; chan < 16; chan++)
		{
		    fluid_synth_activate_tuning(m_synth, chan, 0, 0, true);
		}
	}
	else
	{
		fluid_synth_activate_key_tuning(m_synth, 0, 0, "", nullptr, true);
		for (int chan = 0; chan < 16; chan++)
		{
		    fluid_synth_activate_tuning(m_synth, chan, 0, 0, true);
		}
	}
}



void Sf2Instrument::reloadSynth()
{
	double tempRate;

	// Set & get, returns the true sample rate
	fluid_settings_setnum( m_settings, (char *) "synth.sample-rate", Engine::audioEngine()->outputSampleRate() );
	fluid_settings_getnum( m_settings, (char *) "synth.sample-rate", &tempRate );
	m_internalSampleRate = static_cast<int>( tempRate );

	if( m_font )
	{
		// Now, delete the old one and replace
		m_synthMutex.lock();
		fluid_synth_remove_sfont( m_synth, m_font );
		delete_fluid_synth( m_synth );

		// New synth
		m_synth = new_fluid_synth( m_settings );
		m_fontId = fluid_synth_add_sfont( m_synth, m_font );
		m_synthMutex.unlock();

		// synth program change (set bank and patch)
		updatePatch();
	}
	else
	{
		// Recreate synth with no soundfonts
		m_synthMutex.lock();
		if (m_synth != nullptr)
		{
			delete_fluid_synth(m_synth);
		}
		m_synth = new_fluid_synth( m_settings );
		m_synthMutex.unlock();
	}

	m_synthMutex.lock();
	if( Engine::audioEngine()->currentQualitySettings().interpolation >=
			AudioEngine::qualitySettings::Interpolation::SincFastest )
	{
		fluid_synth_set_interp_method( m_synth, -1, FLUID_INTERP_7THORDER );
	}
	else
	{
		fluid_synth_set_interp_method( m_synth, -1, FLUID_INTERP_DEFAULT );
	}
	m_synthMutex.unlock();
	if( m_internalSampleRate < Engine::audioEngine()->outputSampleRate() )
	{
		m_synthMutex.lock();
		if( m_srcState != nullptr )
		{
			src_delete( m_srcState );
		}
		int error;
		m_srcState = src_new( Engine::audioEngine()->currentQualitySettings().libsrcInterpolation(), DEFAULT_CHANNELS, &error );
		if( m_srcState == nullptr || error )
		{
			qCritical("error while creating libsamplerate data structure in Sf2Instrument::reloadSynth()");
		}
		m_synthMutex.unlock();
	}
	updateReverb();
	updateChorus();
	updateReverbOn();
	updateChorusOn();
	updateGain();
	updateTuning();

	// Reset last MIDI pitch properties, which will be set to the correct values
	// upon playing the next note
	m_lastMidiPitch = -1;
	m_lastMidiPitchRange = -1;
}




void Sf2Instrument::playNote( NotePlayHandle * _n, SampleFrame* )
{
	if( _n->isMasterNote() || ( _n->hasParent() && _n->isReleased() ) )
	{
		return;
	}

	int masterPitch = instrumentTrack()->useMasterPitchModel()->value() ? Engine::getSong()->masterPitch() : 0;
	int baseNote = instrumentTrack()->baseNoteModel()->value();
	int midiNote = _n->midiKey() - baseNote + DefaultBaseKey + masterPitch;

	// out of range?
	if (midiNote < 0 || midiNote >= 128)
	{
		return;
	}

	if (!_n->m_pluginData)
	{
		const int baseVelocity = instrumentTrack()->midiPort()->baseVelocity();

		auto pluginData = new Sf2PluginData;
		pluginData->midiNote = midiNote;
		pluginData->lastPanning = 0;
		pluginData->lastVelocity = _n->midiVelocity( baseVelocity );
		pluginData->isNew = true;
		pluginData->offset = _n->offset();
		pluginData->noteOffSent = false;
		pluginData->panning = _n->getPanning();

		_n->m_pluginData = pluginData;

		// insert the nph to the playing notes vector
		m_playingNotesMutex.lock();
		m_playingNotes.append( _n );
		m_playingNotesMutex.unlock();
	}
	else if( _n->isReleased() && ! _n->instrumentTrack()->isSustainPedalPressed() ) // note is released during this period
	{
		auto pluginData = static_cast<Sf2PluginData*>(_n->m_pluginData);
		pluginData->offset = _n->framesBeforeRelease();
		pluginData->isNew = false;

		m_playingNotesMutex.lock();
		m_playingNotes.append( _n );
		m_playingNotesMutex.unlock();
	}

	// Update the pitch of all the voices
	if (const auto data = static_cast<Sf2PluginData*>(_n->m_pluginData)) {
		const auto detuning = _n->currentDetuning();
		for (const auto& voice : data->fluidVoices) {
			if (voice.isValid()) {
				fluid_voice_gen_set(voice.get(), GEN_COARSETUNE, voice.coarseTune() + detuning);
				fluid_voice_update_param(voice.get(), GEN_COARSETUNE);
			}
		}
	}
}


void Sf2Instrument::noteOn( Sf2PluginData * n )
{
	m_synthMutex.lock();

	// get list of current voice IDs so we can easily spot the new
	// voice after the fluid_synth_noteon() call
	const int poly = fluid_synth_get_polyphony( m_synth );
#ifndef _MSC_VER
	fluid_voice_t* voices[poly];
#else
	const auto voices = static_cast<fluid_voice_t**>(_alloca(poly * sizeof(fluid_voice_t*)));
#endif

	fluid_synth_noteon( m_synth, m_channel, n->midiNote, n->lastVelocity );

	// Get any new voices and store them in the plugin data
	fluid_synth_get_voicelist(m_synth, voices, poly, -1);
	for (int i = 0; i < poly && voices[i] && !n->fluidVoices.full(); ++i)
	{
		const auto voice = voices[i];
		// FluidSynth stops voices with the same channel and pitch upon note-on,
		// so voices with the current channel and pitch are playing this note.
		if (fluid_voice_get_channel(voice) == m_channel
			&& fluid_voice_get_key(voice) == n->midiNote
			&& fluid_voice_is_on(voice)
		) {
			n->fluidVoices.emplace_back(voices[i]);
		}
	}

#if FLUIDSYNTH_VERSION_MAJOR >= 2
	// Smallest balance value that results in full attenuation of one channel.
	// Corresponds to internal FluidSynth macro `FLUID_CB_AMP_SIZE`.
	constexpr static auto maxBalance = 1441.f;
	// Convert panning from linear to exponential for FluidSynth
	const auto panning = n->panning;
	const auto factor = 1.f - std::abs(panning) / static_cast<float>(PanningRight);
	const auto balance = std::copysign(
		factor <= 0 ? maxBalance : std::min(-200.f * std::log10(factor), maxBalance),
		panning
	);
	// Set note panning on all the voices
	for (const auto& voice : n->fluidVoices) {
		if (voice.isValid()) {
			fluid_voice_gen_set(voice.get(), GEN_CUSTOM_BALANCE, balance);
			fluid_voice_update_param(voice.get(), GEN_CUSTOM_BALANCE);
		}
	}
#endif

	m_synthMutex.unlock();

	m_notesRunningMutex.lock();
	++m_notesRunning[ n->midiNote ];
	m_notesRunningMutex.unlock();
}


void Sf2Instrument::noteOff( Sf2PluginData * n )
{
	n->noteOffSent = true;
	m_notesRunningMutex.lock();
	const int notes = --m_notesRunning[n->midiNote];
	m_notesRunningMutex.unlock();

	if( notes <= 0 )
	{
		m_synthMutex.lock();
		fluid_synth_noteoff( m_synth, m_channel, n->midiNote );
		m_synthMutex.unlock();
	}
}


void Sf2Instrument::play( SampleFrame* _working_buffer )
{
	const fpp_t frames = Engine::audioEngine()->framesPerPeriod();

	// set midi pitch for this period
	const int currentMidiPitch = instrumentTrack()->midiPitch();
	if( m_lastMidiPitch != currentMidiPitch )
	{
		m_lastMidiPitch = currentMidiPitch;
		m_synthMutex.lock();
		fluid_synth_pitch_bend( m_synth, m_channel, m_lastMidiPitch );
		m_synthMutex.unlock();
	}

	const int currentMidiPitchRange = instrumentTrack()->midiPitchRange();
	if( m_lastMidiPitchRange != currentMidiPitchRange )
	{
		m_lastMidiPitchRange = currentMidiPitchRange;
		m_synthMutex.lock();
		fluid_synth_pitch_wheel_sens( m_synth, m_channel, m_lastMidiPitchRange );
		m_synthMutex.unlock();
	}
	// if we have no new noteons/noteoffs, just render a period and call it a day
	if( m_playingNotes.isEmpty() )
	{
		renderFrames( frames, _working_buffer );
		return;
	}

	// processing loop
	// go through noteplayhandles in processing order
	f_cnt_t currentFrame = 0;

	while( ! m_playingNotes.isEmpty() )
	{
		// find the note with lowest offset
		NotePlayHandle * currentNote = m_playingNotes[0];
		for( int i = 1; i < m_playingNotes.size(); ++i )
		{
			auto currentData = static_cast<Sf2PluginData*>(currentNote->m_pluginData);
			auto iData = static_cast<Sf2PluginData*>(m_playingNotes[i]->m_pluginData);
			if( currentData->offset > iData->offset )
			{
				currentNote = m_playingNotes[i];
			}
		}

		// process the current note:
		// first see if we're synced in frame count
		auto currentData = static_cast<Sf2PluginData*>(currentNote->m_pluginData);
		if( currentData->offset > currentFrame )
		{
			renderFrames( currentData->offset - currentFrame, _working_buffer + currentFrame );
			currentFrame = currentData->offset;
		}
		if( currentData->isNew )
		{
			noteOn( currentData );
			if( currentNote->isReleased() ) // if the note is released during the same period, we have to process it again for noteoff
			{
				currentData->isNew = false;
				currentData->offset = currentNote->framesBeforeRelease();
			}
			else // otherwise remove the handle
			{
				m_playingNotesMutex.lock();
				m_playingNotes.remove( m_playingNotes.indexOf( currentNote ) );
				m_playingNotesMutex.unlock();
			}
		}
		else
		{
			noteOff( currentData );
			m_playingNotesMutex.lock();
			m_playingNotes.remove( m_playingNotes.indexOf( currentNote ) );
			m_playingNotesMutex.unlock();
		}
	}

	if( currentFrame < frames )
	{
		renderFrames( frames - currentFrame, _working_buffer + currentFrame );
	}
}


void Sf2Instrument::renderFrames( f_cnt_t frames, SampleFrame* buf )
{
	m_synthMutex.lock();
	fluid_synth_get_gain(m_synth); // This flushes voice updates as a side effect
	if( m_internalSampleRate < Engine::audioEngine()->outputSampleRate() &&
							m_srcState != nullptr )
	{
		const fpp_t f = frames * m_internalSampleRate / Engine::audioEngine()->outputSampleRate();
#ifdef __GNUC__
		SampleFrame tmp[f];
#else
		SampleFrame* tmp = new SampleFrame[f];
#endif
		fluid_synth_write_float( m_synth, f, tmp, 0, 2, tmp, 1, 2 );

		SRC_DATA src_data;
		src_data.data_in = (float *)tmp;
		src_data.data_out = (float *)buf;
		src_data.input_frames = f;
		src_data.output_frames = frames;
		src_data.src_ratio = (double) frames / f;
		src_data.end_of_input = 0;
		int error = src_process( m_srcState, &src_data );
#ifndef __GNUC__
		delete[] tmp;
#endif
		if( error )
		{
			qCritical( "Sf2Instrument: error while resampling: %s", src_strerror( error ) );
		}
		if (static_cast<f_cnt_t>(src_data.output_frames_gen) < frames)
		{
			qCritical("Sf2Instrument: not enough frames: %ld / %zu", src_data.output_frames_gen, frames);
		}
	}
	else
	{
		fluid_synth_write_float( m_synth, frames, buf, 0, 2, buf, 1, 2 );
	}
	m_synthMutex.unlock();
}




void Sf2Instrument::deleteNotePluginData( NotePlayHandle * _n )
{
	auto pluginData = static_cast<Sf2PluginData*>(_n->m_pluginData);
	if( ! pluginData->noteOffSent ) // if we for some reason haven't noteoffed the note before it gets deleted,
									// do it here
	{
		noteOff( pluginData );
		m_playingNotesMutex.lock();
		if( m_playingNotes.indexOf( _n ) >= 0 )
		{
			m_playingNotes.remove( m_playingNotes.indexOf( _n ) );
		}
		m_playingNotesMutex.unlock();
	}
	delete pluginData;
}




gui::PluginView * Sf2Instrument::instantiateView( QWidget * _parent )
{
	return new gui::Sf2InstrumentView( this, _parent );
}




namespace gui
{


class Sf2Knob : public Knob
{
public:
	Sf2Knob( QWidget * _parent ) :
			Knob( KnobType::Styled, _parent )
	{
		setFixedSize( 31, 38 );
	}
};



Sf2InstrumentView::Sf2InstrumentView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent )
{
//	QVBoxLayout * vl = new QVBoxLayout( this );
//	QHBoxLayout * hl = new QHBoxLayout();

	auto k = castModel<Sf2Instrument>();

	connect(&k->m_bankNum, SIGNAL(dataChanged()), this, SLOT(updatePatchName()));
	connect(&k->m_patchNum, SIGNAL(dataChanged()), this, SLOT(updatePatchName()));

	// File Button
	m_fileDialogButton = new PixmapButton(this);
	m_fileDialogButton->setCursor(QCursor(Qt::PointingHandCursor));
	m_fileDialogButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("fileselect_on"));
	m_fileDialogButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("fileselect_off"));
	m_fileDialogButton->move(217, 107);

	connect(m_fileDialogButton, SIGNAL(clicked()), this, SLOT(showFileDialog()));

	m_fileDialogButton->setToolTip(tr("Open SoundFont file"));

	// Patch Button
	m_patchDialogButton = new PixmapButton(this);
	m_patchDialogButton->setCursor(QCursor(Qt::PointingHandCursor));
	m_patchDialogButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("patches_on"));
	m_patchDialogButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("patches_off"));
	m_patchDialogButton->setEnabled(false);
	m_patchDialogButton->move(217, 125);

	connect(m_patchDialogButton, SIGNAL(clicked()), this, SLOT(showPatchDialog()));

	m_patchDialogButton->setToolTip(tr("Choose patch"));

	// LCDs
	m_bankNumLcd = new LcdSpinBox(3, "21pink", this);
	m_bankNumLcd->move(131, 62);
//	m_bankNumLcd->addTextForValue( -1, "---" );
//	m_bankNumLcd->setEnabled( false );

	m_patchNumLcd = new LcdSpinBox( 3, "21pink", this );
	m_patchNumLcd->move(190, 62);
//	m_patchNumLcd->addTextForValue( -1, "---" );
//	m_patchNumLcd->setEnabled( false );

	/*hl->addWidget( m_fileDialogButton );
	hl->addWidget( m_bankNumLcd );
	hl->addWidget( m_patchNumLcd );
	hl->addWidget( m_patchDialogButton );

	vl->addLayout( hl );*/

	// Next row

	//hl = new QHBoxLayout();

	m_filenameLabel = new QLabel( this );
	m_filenameLabel->setGeometry( 58, 109, 156, 11 );
	m_patchLabel = new QLabel( this );
	m_patchLabel->setGeometry( 58, 127, 156, 11 );

	//hl->addWidget( m_filenameLabel );
//	vl->addLayout( hl );

	// Gain
	m_gainKnob = new Sf2Knob( this );
	m_gainKnob->setHintText( tr("Gain:"), "" );
	m_gainKnob->move( 86, 55 );
//	vl->addWidget( m_gainKnob );

	// Reverb
//	hl = new QHBoxLayout();


	m_reverbButton = new PixmapButton( this );
	m_reverbButton->setCheckable( true );
	m_reverbButton->move( 14, 180 );
	m_reverbButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "reverb_on" ) );
	m_reverbButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "reverb_off" ) );
	m_reverbButton->setToolTip(tr("Apply reverb (if supported)"));


	m_reverbRoomSizeKnob = new Sf2Knob( this );
	m_reverbRoomSizeKnob->setHintText( tr("Room size:"), "" );
	m_reverbRoomSizeKnob->move( 93, 160 );

	m_reverbDampingKnob = new Sf2Knob( this );
	m_reverbDampingKnob->setHintText( tr("Damping:"), "" );
	m_reverbDampingKnob->move( 130, 160 );

	m_reverbWidthKnob = new Sf2Knob( this );
	m_reverbWidthKnob->setHintText( tr("Width:"), "" );
	m_reverbWidthKnob->move( 167, 160 );

	m_reverbLevelKnob = new Sf2Knob( this );
	m_reverbLevelKnob->setHintText( tr("Level:"), "" );
	m_reverbLevelKnob->move( 204, 160 );

/*	hl->addWidget( m_reverbOnLed );
	hl->addWidget( m_reverbRoomSizeKnob );
	hl->addWidget( m_reverbDampingKnob );
	hl->addWidget( m_reverbWidthKnob );
	hl->addWidget( m_reverbLevelKnob );

	vl->addLayout( hl );
*/

	// Chorus
//	hl = new QHBoxLayout();

	m_chorusButton = new PixmapButton( this );
	m_chorusButton->setCheckable( true );
	m_chorusButton->move( 14, 226 );
	m_chorusButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "chorus_on" ) );
	m_chorusButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "chorus_off" ) );
	m_chorusButton->setToolTip(tr("Apply chorus (if supported)"));

	m_chorusNumKnob = new Sf2Knob( this );
	m_chorusNumKnob->setHintText( tr("Voices:"), "" );
	m_chorusNumKnob->move( 93, 206 );

	m_chorusLevelKnob = new Sf2Knob( this );
	m_chorusLevelKnob->setHintText( tr("Level:"), "" );
	m_chorusLevelKnob->move( 130 , 206 );

	m_chorusSpeedKnob = new Sf2Knob( this );
	m_chorusSpeedKnob->setHintText( tr("Speed:"), "" );
	m_chorusSpeedKnob->move( 167 , 206 );

	m_chorusDepthKnob = new Sf2Knob( this );
	m_chorusDepthKnob->setHintText( tr("Depth:"), "" );
	m_chorusDepthKnob->move( 204 , 206 );
/*
	hl->addWidget( m_chorusOnLed );
	hl->addWidget( m_chorusNumKnob);
	hl->addWidget( m_chorusLevelKnob);
	hl->addWidget( m_chorusSpeedKnob);
	hl->addWidget( m_chorusDepthKnob);

	vl->addLayout( hl );
*/
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	updateFilename();
}




void Sf2InstrumentView::modelChanged()
{
	auto k = castModel<Sf2Instrument>();
	m_bankNumLcd->setModel( &k->m_bankNum );
	m_patchNumLcd->setModel( &k->m_patchNum );

	m_gainKnob->setModel( &k->m_gain );

	m_reverbButton->setModel( &k->m_reverbOn );
	m_reverbRoomSizeKnob->setModel( &k->m_reverbRoomSize );
	m_reverbDampingKnob->setModel( &k->m_reverbDamping );
	m_reverbWidthKnob->setModel( &k->m_reverbWidth );
	m_reverbLevelKnob->setModel( &k->m_reverbLevel );

	m_chorusButton->setModel( &k->m_chorusOn );
	m_chorusNumKnob->setModel( &k->m_chorusNum );
	m_chorusLevelKnob->setModel( &k->m_chorusLevel );
	m_chorusSpeedKnob->setModel( &k->m_chorusSpeed );
	m_chorusDepthKnob->setModel( &k->m_chorusDepth );


	connect( k, SIGNAL( fileChanged() ), this, SLOT( updateFilename() ) );

	connect( k, SIGNAL( fileLoading() ), this, SLOT( invalidateFile() ) );

	updateFilename();
}




void Sf2InstrumentView::updateFilename()
{
	auto i = castModel<Sf2Instrument>();
	QFontMetrics fm( m_filenameLabel->font() );
	QString file = i->m_filename.endsWith( ".sf2", Qt::CaseInsensitive ) ?
			i->m_filename.left( i->m_filename.length() - 4 ) :
			i->m_filename;
	m_filenameLabel->setText( fm.elidedText( file, Qt::ElideLeft, m_filenameLabel->width() ) );
			//		i->m_filename + "\nPatch: TODO" );

	m_patchDialogButton->setEnabled( !i->m_filename.isEmpty() );

	updatePatchName();

	update();
}




void Sf2InstrumentView::updatePatchName()
{
	auto i = castModel<Sf2Instrument>();
	QFontMetrics fm( font() );
	QString patch = i->getCurrentPatchName();
	m_patchLabel->setText( fm.elidedText( patch, Qt::ElideLeft, m_patchLabel->width() ) );


	update();
}




void Sf2InstrumentView::invalidateFile()
{
	m_patchDialogButton->setEnabled( false );
}




void Sf2InstrumentView::showFileDialog()
{
	auto k = castModel<Sf2Instrument>();

	FileDialog ofd( nullptr, tr( "Open SoundFont file" ) );
	ofd.setFileMode( FileDialog::ExistingFiles );

	QStringList types;
	types << tr( "SoundFont Files (*.sf2 *.sf3)" );
	ofd.setNameFilters( types );

	if( k->m_filename != "" )
	{
		QString f = PathUtil::toAbsolute( k->m_filename );
		ofd.setDirectory( QFileInfo( f ).absolutePath() );
		ofd.selectFile( QFileInfo( f ).fileName() );
	}
	else
	{
		ofd.setDirectory( ConfigManager::inst()->sf2Dir() );
	}

	m_fileDialogButton->setEnabled( false );

	if( ofd.exec() == QDialog::Accepted && !ofd.selectedFiles().isEmpty() )
	{
		QString f = ofd.selectedFiles()[0];
		if( f != "" )
		{
			k->openFile( f );
			Engine::getSong()->setModified();
		}
	}

	m_fileDialogButton->setEnabled( true );
}




void Sf2InstrumentView::showPatchDialog()
{
	auto k = castModel<Sf2Instrument>();

	PatchesDialog pd( this );

	pd.setup( k->m_synth, 1, k->instrumentTrack()->name(), &k->m_bankNum, &k->m_patchNum, m_patchLabel );

	pd.exec();
}


} // namespace gui

extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return new Sf2Instrument( static_cast<InstrumentTrack *>( m ) );
}
}


} // namespace lmms
