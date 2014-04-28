/*
 * FlpImport.cpp - support for importing FLP-files
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#include <QtXml/QDomDocument>
#include <QtGui/QApplication>
#include <QtGui/QProgressDialog>
#include <QtCore/QDir>
#include <QtCore/QBuffer>

#include "FlpImport.h"
#include "NotePlayHandle.h"
#include "AutomationPattern.h"
#include "basic_filters.h"
#include "bb_track.h"
#include "bb_track_container.h"
#include "combobox.h"
#include "config_mgr.h"
#include "debug.h"
#include "Effect.h"
#include "engine.h"
#include "FxMixer.h"
#include "group_box.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "EnvelopeAndLfoParameters.h"
#include "knob.h"
#include "Oscillator.h"
#include "pattern.h"
#include "Piano.h"
#include "ProjectJournal.h"
#include "project_notes.h"
#include "song.h"
#include "TrackContainer.h"
#include "embed.h"
#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CTYPE_H
#include <ctype.h>
#endif

#define makeID(_c0, _c1, _c2, _c3) \
	( ( _c0 ) | ( ( _c1 ) << 8 ) | ( ( _c2 ) << 16 ) | ( ( _c3 ) << 24 ) )


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT flpimport_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"FLP Import",
	QT_TRANSLATE_NOOP( "pluginBrowser",
			"Filter for importing FL Studio projects into LMMS" ),
	"Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>",
	0x0100,
	Plugin::ImportFilter,
	NULL,
	NULL,
	NULL
} ;


// unrtf-stuff
#include "defs.h"
#include "main.h"
#include "html.h"
#include "word.h"
#include "hash.h"
#include "convert.h"
#include "attr.h"

extern OutputPersonality * op;
extern int lineno;
extern QString outstring;

}



static void dump_mem( const void * buffer, uint n_bytes )
{
	uchar * cp = (uchar *) buffer;
	for( uint k = 0; k < n_bytes; ++k )
	{
		qDebug( "%02x ", (unsigned int)cp[k] );//( cp[k] > 31 || cp[k] < 7 ) ? cp[k] : '.' );
	}
	qDebug( "\n" );
}


enum FLP_Events
{
	// BYTE EVENTS
	FLP_Byte		= 0,
	FLP_Enabled		= 0,
	FLP_NoteOn		= 1,	//+pos (byte)
	FLP_Vol			= 2,
	FLP_Pan			= 3,
	FLP_MIDIChan		= 4,
	FLP_MIDINote		= 5,
	FLP_MIDIPatch		= 6,
	FLP_MIDIBank		= 7,
	FLP_LoopActive		= 9,
	FLP_ShowInfo		= 10,
	FLP_Shuffle		= 11,
	FLP_MainVol		= 12,
	FLP_Stretch		= 13,	// old byte version
	FLP_Pitchable		= 14,
	FLP_Zipped		= 15,
	FLP_Delay_Flags		= 16,
	FLP_PatLength		= 17,
	FLP_BlockLength		= 18,
	FLP_UseLoopPoints	= 19,
	FLP_LoopType		= 20,
	FLP_ChanType		= 21,
	FLP_MixSliceNum		= 22,
	FLP_EffectChannelMuted	= 27,

	// WORD EVENTS
	FLP_Word		= 64,
	FLP_NewChan		= FLP_Word,
	FLP_NewPat		= FLP_Word + 1,		//+PatNum (word)
	FLP_Tempo		= FLP_Word + 2,
	FLP_CurrentPatNum	= FLP_Word + 3,
	FLP_PatData		= FLP_Word + 4,
	FLP_FX			= FLP_Word + 5,
	FLP_Fade_Stereo		= FLP_Word + 6,
	FLP_CutOff		= FLP_Word + 7,
	FLP_DotVol		= FLP_Word + 8,
	FLP_DotPan		= FLP_Word + 9,
	FLP_PreAmp		= FLP_Word + 10,
	FLP_Decay		= FLP_Word + 11,
	FLP_Attack		= FLP_Word + 12,
	FLP_DotNote		= FLP_Word + 13,
	FLP_DotPitch		= FLP_Word + 14,
	FLP_DotMix		= FLP_Word + 15,
	FLP_MainPitch		= FLP_Word + 16,
	FLP_RandChan		= FLP_Word + 17,
	FLP_MixChan		= FLP_Word + 18,
	FLP_Resonance		= FLP_Word + 19,
	FLP_LoopBar		= FLP_Word + 20,
	FLP_StDel		= FLP_Word + 21,
	FLP_FX3			= FLP_Word + 22,
	FLP_DotReso		= FLP_Word + 23,
	FLP_DotCutOff		= FLP_Word + 24,
	FLP_ShiftDelay		= FLP_Word + 25,
	FLP_LoopEndBar		= FLP_Word + 26,
	FLP_Dot			= FLP_Word + 27,
	FLP_DotShift		= FLP_Word + 28,
	FLP_LayerChans		= FLP_Word + 30,

	// DWORD EVENTS
	FLP_Int			= 128,
	FLP_Color		= FLP_Int,
	FLP_PlayListItem	= FLP_Int + 1,	//+Pos (word) +PatNum (word)
	FLP_Echo		= FLP_Int + 2,
	FLP_FXSine		= FLP_Int + 3,
	FLP_CutCutBy		= FLP_Int + 4,
	FLP_WindowH		= FLP_Int + 5,
	FLP_MiddleNote		= FLP_Int + 7,
	FLP_Reserved		= FLP_Int + 8,	// may contain an invalid
						// version info
	FLP_MainResoCutOff	= FLP_Int + 9,
	FLP_DelayReso		= FLP_Int + 10,
	FLP_Reverb		= FLP_Int + 11,
	FLP_IntStretch		= FLP_Int + 12,
	FLP_SSNote		= FLP_Int + 13,
	FLP_FineTune		= FLP_Int + 14,

	// TEXT EVENTS
	FLP_Undef		= 192,		//+Size (var length)
	FLP_Text		= FLP_Undef,	//+Size (var length)+Text
						//	(Null Term. String)
	FLP_Text_ChanName	= FLP_Text,	// name for the current channel
	FLP_Text_PatName	= FLP_Text + 1,	// name for the current pattern
	FLP_Text_Title		= FLP_Text + 2,	// title of the loop
	FLP_Text_Comment	= FLP_Text + 3,	// old comments in text format.
						// Not used anymore
	FLP_Text_SampleFileName	= FLP_Text + 4,	// filename for the sample in
						// the current channel, stored
						// as relative path
	FLP_Text_URL		= FLP_Text + 5,
	FLP_Text_CommentRTF	= FLP_Text + 6,	// new comments in Rich Text
						// format
	FLP_Text_Version	= FLP_Text + 7,
	FLP_Text_PluginName	= FLP_Text + 9,	// plugin file name
						// (without path)

	FLP_Text_EffectChanName	= FLP_Text + 12,
	FLP_Text_MIDICtrls	= FLP_Text + 16,
	FLP_Text_Delay		= FLP_Text + 17,
	FLP_Text_TS404Params	= FLP_Text + 18,
	FLP_Text_DelayLine	= FLP_Text + 19,
	FLP_Text_NewPlugin	= FLP_Text + 20,
	FLP_Text_PluginParams	= FLP_Text + 21,
	FLP_Text_ChanParams	= FLP_Text + 23,// block of various channel
						// params (can grow)
	FLP_Text_EnvLfoParams	= FLP_Text + 26,
	FLP_Text_BasicChanParams= FLP_Text + 27,
	FLP_Text_OldFilterParams= FLP_Text + 28,
	FLP_Text_AutomationData	= FLP_Text + 31,
	FLP_Text_PatternNotes	= FLP_Text + 32,
	FLP_Text_ChanGroupName	= FLP_Text + 39,
	FLP_Text_PlayListItems	= FLP_Text + 41,

	FLP_Event_EffectParams = 225,
	FLP_Event_PlaylistItems = 233,

	FLP_CmdCount

} ;


struct FL_Automation
{
	FL_Automation() :
		pos( 0 ),
		value( 0 ),
		channel( 0 ),
		control( 0 )
	{
	}

	enum Controls
	{
		ControlVolume			= 0,
		ControlPanning			= 1,
		ControlFilterCut		= 2,
		ControlFilterRes		= 3,
		ControlPitch			= 4,
		ControlFilterType		= 5,
		ControlFXChannel		= 8,

		ControlVolPredelay		= 4354,
		ControlVolAttack,
		ControlVolHold,
		ControlVolDecay,
		ControlVolSustain,
		ControlVolRelease,
		ControlVolLfoPredelay		= ControlVolPredelay+7,
		ControlVolLfoAttack,
		ControlVolLfoAmount,
		ControlVolLfoSpeed,
		ControlVolAttackTension		= ControlVolPredelay+12,
		ControlVolDecayTension,
		ControlVolReleaseTension,
		ControlCutPredelay		= 4610,
		ControlCutAttack,
		ControlCutHold,
		ControlCutDecay,
		ControlCutSustain,
		ControlCutRelease,
		ControlCutAmount,
		ControlCutLfoPredelay		= ControlCutPredelay+7,
		ControlCutLfoAttack,
		ControlCutLfoAmount,
		ControlCutLfoSpeed,
		ControlCutAttackTension		= ControlCutPredelay+12,
		ControlCutDecayTension,
		ControlCutReleaseTension,

		ControlResPredelay		= 4866,
		ControlResAttack,
		ControlResHold,
		ControlResDecay,
		ControlResSustain,
		ControlResRelease,
		ControlResAmount,
		ControlResLfoPredelay		= ControlResPredelay+7,
		ControlResLfoAttack,
		ControlResLfoAmount,
		ControlResLfoSpeed,
		ControlResAttackTension		= ControlResPredelay+12,
		ControlResDecayTension,
		ControlResReleaseTension
	} ;

	int pos;
	int value;
	int channel;
	int control;

} ;



struct FL_Channel_Envelope
{
	InstrumentSoundShaping::Targets target;
	float predelay;
	float attack;
	float hold;
	float decay;
	float sustain;
	float release;
	float amount;
} ;


struct FL_Plugin
{
	enum PluginTypes
	{
		UnknownPlugin,

		InstrumentPlugin,
		Sampler,
		TS404,
		Fruity_3x_Osc,
		Layer,
		BeepMap,
		BuzzGeneratorAdapter,
		FruitKick,
		FruityDrumSynthLive,
		FruityDX10,
		FruityGranulizer,
		FruitySlicer,
		FruitySoundfontPlayer,
		FruityVibrator,
		MidiOut,
		Plucked,
		SimSynth,
		Sytrus,
		WASP,

		EffectPlugin,
		Fruity7BandEq,
		FruityBalance,
		FruityBassBoost,
		FruityBigClock,
		FruityBloodOverdrive,
		FruityCenter,
		FruityChorus,
		FruityCompressor,
		FruityDbMeter,
		FruityDelay,
		FruityDelay2,
		FruityFastDist,
		FruityFastLP,
		FruityFilter,
		FruityFlanger,
		FruityFormulaController,
		FruityFreeFilter,
		FruityHTMLNotebook,
		FruityLSD,
		FruityMute2,
		FruityNotebook,
		FruityPanOMatic,
		FruityParametricEQ,
		FruityPeakController,
		FruityPhaseInverter,
		FruityPhaser,
		FruityReeverb,
		FruityScratcher,
		FruitySend,
		FruitySoftClipper,
		FruitySpectroman,
		FruityStereoEnhancer,
		FruityXYController
	} ;

	FL_Plugin( PluginTypes _pt = UnknownPlugin ) :
		pluginType( _pt ),
		name(),
		pluginSettings( NULL ),
		pluginSettingsLength( 0 )
	{
	}

	~FL_Plugin()
	{
		delete[] pluginSettings;
	}

	PluginTypes pluginType;
	QString name;

	char * pluginSettings;
	int pluginSettingsLength;

} ;


struct FL_Channel : public FL_Plugin
{
	QList<FL_Automation> automationData;

	int volume;
	int panning;
	int baseNote;
	int fxChannel;
	int layerParent;

	typedef QList<QPair<int, note> > noteVector;
	noteVector notes;

	QList<int> dots;


	QString sampleFileName;
	int sampleAmp;
	bool sampleReversed;
	bool sampleReverseStereo;
	bool sampleUseLoopPoints;

	Instrument * instrumentPlugin;

	QList<FL_Channel_Envelope> envelopes;

	int filterType;
	float filterCut;
	float filterRes;
	bool filterEnabled;

	int arpDir;
	int arpRange;
	int selectedArp;
	float arpTime;
	float arpGate;
	bool arpEnabled;

	QRgb color;


	FL_Channel( PluginTypes _pt = UnknownPlugin ) :
		FL_Plugin( _pt ),
		automationData(),
		volume( DefaultVolume ),
		panning( DefaultPanning ),
		baseNote( DefaultKey ),
		fxChannel( 0 ),
		layerParent( -1 ),
		notes(),
		dots(),
		sampleFileName(),
		sampleAmp( 100 ),
		sampleReversed( false ),
		sampleReverseStereo( false ),
		sampleUseLoopPoints( false ),
		instrumentPlugin( NULL ),
		envelopes(),
		filterType( basicFilters<>::LowPass ),
		filterCut( 10000 ),
		filterRes( 0.1 ),
		filterEnabled( false ),
		arpDir( InstrumentFunctionArpeggio::ArpDirUp ),
		arpRange( 0 ),
		selectedArp( 0 ),
		arpTime( 100 ),
		arpGate( 100 ),
		arpEnabled( false ),
		color( qRgb( 64, 128, 255 ) )
	{
	}

} ;


struct FL_Effect : public FL_Plugin
{
	FL_Effect( PluginTypes _pt = UnknownPlugin ) :
		FL_Plugin( _pt ),
		fxChannel( 0 ),
		fxPos( 0 )
	{
	}

	int fxChannel;
	int fxPos;

} ;


struct FL_PlayListItem
{
	FL_PlayListItem() :
		position( 0 ),
		length( 1 ),
		pattern( 0 )
	{
	}
	int position;
	int length;
	int pattern;
} ;


struct FL_EffectChannel
{
	FL_EffectChannel() :
		name(),
		volume( DefaultVolume ),
		isMuted( false )
	{
	}

	QString name;
	int volume;
	bool isMuted;
} ;


struct FL_Project
{
	int mainVolume;
	int mainPitch;
	bpm_t tempo;
	int numChannels;

	QList<FL_Channel> channels;
	QList<FL_Effect> effects;
	QList<FL_PlayListItem> playListItems;

	QMap<int, QString> patternNames;
	int maxPatterns;
	int currentPattern;
	int activeEditPattern;

	FL_EffectChannel effectChannels[NumFxChannels+1];
	int currentEffectChannel;

	QString projectNotes;
	QString projectTitle;

	QString versionString;
	int version;
	int versionSpecificFactor;

	FL_Project() :
		mainVolume( DefaultVolume ),
		mainPitch( 0 ),
		tempo( DefaultTempo ),
		numChannels( 0 ),
		channels(),
		effects(),
		playListItems(),
		patternNames(),
		maxPatterns( 0 ),
		currentPattern( 0 ),
		activeEditPattern( 0 ),
		effectChannels(),
		currentEffectChannel( -1 ),
		projectNotes(),
		projectTitle(),
		versionString(),
		version( 0x100 ),
		versionSpecificFactor( 1 )
	{
	}

} ;




FlpImport::FlpImport( const QString & _file ) :
	ImportFilter( _file, &flpimport_plugin_descriptor )
{
}




FlpImport::~FlpImport()
{
}




bool FlpImport::tryImport( TrackContainer* tc )
{
	const int mappedFilter[] =
	{
		basicFilters<>::LowPass,// fast LP
		basicFilters<>::LowPass,
		basicFilters<>::BandPass_CSG,
		basicFilters<>::HiPass,
		basicFilters<>::Notch,
		basicFilters<>::NumFilters+basicFilters<>::LowPass,
		basicFilters<>::LowPass,
		basicFilters<>::NumFilters+basicFilters<>::LowPass
	} ;

	const InstrumentFunctionArpeggio::ArpDirections mappedArpDir[] =
	{
		InstrumentFunctionArpeggio::ArpDirUp,
		InstrumentFunctionArpeggio::ArpDirUp,
		InstrumentFunctionArpeggio::ArpDirDown,
		InstrumentFunctionArpeggio::ArpDirUpAndDown,
		InstrumentFunctionArpeggio::ArpDirUpAndDown,
		InstrumentFunctionArpeggio::ArpDirRandom
	} ;

	QMap<QString, int> mappedPluginTypes;

	// instruments
	mappedPluginTypes["sampler"] = FL_Plugin::Sampler;
	mappedPluginTypes["ts404"] = FL_Plugin::TS404;
	mappedPluginTypes["3x osc"] = FL_Plugin::Fruity_3x_Osc;
	mappedPluginTypes["beepmap"] = FL_Plugin::BeepMap;
	mappedPluginTypes["buzz generator adapter"] = FL_Plugin::BuzzGeneratorAdapter;
	mappedPluginTypes["fruit kick"] = FL_Plugin::FruitKick;
	mappedPluginTypes["fruity drumsynth live"] = FL_Plugin::FruityDrumSynthLive;
	mappedPluginTypes["fruity dx10"] = FL_Plugin::FruityDX10;
	mappedPluginTypes["fruity granulizer"] = FL_Plugin::FruityGranulizer;
	mappedPluginTypes["fruity slicer"] = FL_Plugin::FruitySlicer;
	mappedPluginTypes["fruity soundfont player"] = FL_Plugin::FruitySoundfontPlayer;
	mappedPluginTypes["fruity vibrator"] = FL_Plugin::FruityVibrator;
	mappedPluginTypes["midi out"] = FL_Plugin::MidiOut;
	mappedPluginTypes["plucked!"] = FL_Plugin::Plucked;
	mappedPluginTypes["simsynth"] = FL_Plugin::SimSynth;
	mappedPluginTypes["sytrus"] = FL_Plugin::Sytrus;
	mappedPluginTypes["wasp"] = FL_Plugin::WASP;

	// effects
	mappedPluginTypes["fruity 7 band EQ"] = FL_Plugin::Fruity7BandEq;
	mappedPluginTypes["fruity balance"] = FL_Plugin::FruityBalance;
	mappedPluginTypes["fruity bass boost"] = FL_Plugin::FruityBassBoost;
	mappedPluginTypes["fruity big clock"] = FL_Plugin::FruityBigClock;
	mappedPluginTypes["fruity blood overdrive"] = FL_Plugin::FruityBloodOverdrive;
	mappedPluginTypes["fruity center"] = FL_Plugin::FruityCenter;
	mappedPluginTypes["fruity chorus"] = FL_Plugin::FruityChorus;
	mappedPluginTypes["fruity compressor"] = FL_Plugin::FruityCompressor;
	mappedPluginTypes["fruity db meter"] = FL_Plugin::FruityDbMeter;
	mappedPluginTypes["fruity delay"] = FL_Plugin::FruityDelay;
	mappedPluginTypes["fruity delay 2"] = FL_Plugin::FruityDelay2;
	mappedPluginTypes["fruity fast dist"] = FL_Plugin::FruityFastDist;
	mappedPluginTypes["fruity fast lp"] = FL_Plugin::FruityFastLP;
	mappedPluginTypes["fruity filter"] = FL_Plugin::FruityFilter;
	mappedPluginTypes["fruity flanger"] = FL_Plugin::FruityFlanger;
	mappedPluginTypes["fruity formula controller"] = FL_Plugin::FruityFormulaController;
	mappedPluginTypes["fruity free filter"] = FL_Plugin::FruityFreeFilter;
	mappedPluginTypes["fruity html notebook"] = FL_Plugin::FruityHTMLNotebook;
	mappedPluginTypes["fruity lsd"] = FL_Plugin::FruityLSD;
	mappedPluginTypes["fruity mute 2"] = FL_Plugin::FruityMute2;
	mappedPluginTypes["fruity notebook"] = FL_Plugin::FruityNotebook;
	mappedPluginTypes["fruity panomatic"] = FL_Plugin::FruityPanOMatic;
	mappedPluginTypes["fruity parametric eq"] = FL_Plugin::FruityParametricEQ;
	mappedPluginTypes["fruity peak controller"] = FL_Plugin::FruityPeakController;
	mappedPluginTypes["fruity phase inverter"] = FL_Plugin::FruityPhaseInverter;
	mappedPluginTypes["fruity phaser"] = FL_Plugin::FruityPhaser;
	mappedPluginTypes["fruity reeverb"] = FL_Plugin::FruityReeverb;
	mappedPluginTypes["fruity scratcher"] = FL_Plugin::FruityScratcher;
	mappedPluginTypes["fruity send"] = FL_Plugin::FruitySend;
	mappedPluginTypes["fruity soft clipper"] = FL_Plugin::FruitySoftClipper;
	mappedPluginTypes["fruity spectroman"] = FL_Plugin::FruitySpectroman;
	mappedPluginTypes["fruity stereo enhancer"] = FL_Plugin::FruityStereoEnhancer;
	mappedPluginTypes["fruity x-y controller"] = FL_Plugin::FruityXYController;


	FL_Project p;

	if( openFile() == false )
	{
		return false;
	}

	if( readID() != makeID( 'F', 'L', 'h', 'd' ) )
	{
		qWarning( "FlpImport::tryImport(): not a valid FL project\n" );
		return false;
	}

	const int header_len = read32LE();
	if( header_len != 6 )
	{
		qWarning( "FlpImport::tryImport(): invalid file format\n" );
		return false;
	}

	const int type = read16LE();
	if( type != 0 )
	{
		qWarning( "FlpImport::tryImport(): type %d format is not "
							"supported\n", type );
		return false;
	}

	p.numChannels = read16LE();
	if( p.numChannels < 1 || p.numChannels > 1000 )
	{
		qWarning( "FlpImport::tryImport(): invalid number of channels "
						"(%d)\n", p.numChannels );
		return false;
	}

	const int ppq = read16LE();
	if( ppq < 0 )
	{
		qWarning( "FlpImport::tryImport(): invalid ppq\n" );
		return false;
	}

	QProgressDialog progressDialog(
			TrackContainer::tr( "Importing FLP-file..." ),
			TrackContainer::tr( "Cancel" ), 0, p.numChannels );
	progressDialog.setWindowTitle( TrackContainer::tr( "Please wait..." ) );
	progressDialog.show();

	bool valid = false;

	// search for FLdt chunk
	while( 1 )
	{
		int32_t id = readID();
		const int len = read32LE();
		if( file().atEnd() )
		{
			qWarning( "FlpImport::tryImport(): unexpected "
						"end of file\n" );
			return false;
		}
		if( len < 0 || len >= 0x10000000 )
		{
			qWarning( "FlpImport::tryImport(): invalid "
						"chunk length %d\n", len );
			return false;
		}
		if( id == makeID( 'F', 'L', 'd', 't' ) )
		{
			valid = true;
			break;
		}
		skip( len );
	}

	if( valid == false )
	{
		return false;
	}

	for( int i = 0; i < p.numChannels; ++i )
	{
		p.channels += FL_Channel();
	}

	qDebug( "channels: %d\n", p.numChannels );


	char * text = NULL;
	int text_len = 0;
	FL_Plugin::PluginTypes last_plugin_type = FL_Plugin::UnknownPlugin;
	
	int cur_channel = -1;

	const bool is_journ = engine::projectJournal()->isJournalling();
	engine::projectJournal()->setJournalling( false );


	while( file().atEnd() == false )
	{
		FLP_Events ev = static_cast<FLP_Events>( readByte() );
		uint32_t data = readByte();

		if( ev >= FLP_Word && ev < FLP_Text )
		{
			data = data | ( readByte() << 8 );
		}

		if( ev >= FLP_Int && ev < FLP_Text )
		{
			data = data | ( readByte() << 16 );
			data = data | ( readByte() << 24 );
		}


		if( ev >= FLP_Text )
		{
			text_len = data & 0x7F;
			uint8_t shift = 0;
			while( data & 0x80 )
			{
				data = readByte();
				text_len = text_len | ( ( data & 0x7F ) <<
							( shift += 7 ) );
			}

			delete[] text;
			text = new char[text_len+1];
			if( readBlock( text, text_len ) <= 0 )
			{
				qWarning( "could not read string (len: %d)\n",
							text_len );
			}

			text[text_len] = 0;
		}
		const unsigned char * puc = (const unsigned char*) text;
		const int * pi = (const int *) text;


		FL_Channel * cc = cur_channel >= 0 ?
					&p.channels[cur_channel] : NULL;

		switch( ev )
		{
			// BYTE EVENTS
			case FLP_Byte:
				qDebug( "undefined byte %d\n", data );
				break;

			case FLP_NoteOn:
				qDebug( "note on: %d\n", data );
				// data = pos   how to handle?
				break;

			case FLP_Vol:
				qDebug( "vol %d\n", data );
				break;

			case FLP_Pan:
				qDebug( "pan %d\n", data );
				break;

			case FLP_LoopActive:
				qDebug( "active loop: %d\n", data );
				break;

			case FLP_ShowInfo:
				qDebug( "show info: %d\n", data );
				break;

			case FLP_Shuffle:
				qDebug( "shuffle: %d\n", data );
				break;

			case FLP_MainVol:
				p.mainVolume = data * 100 / 128;
				break;

			case FLP_PatLength:
				qDebug( "pattern length: %d\n", data );
				break;

			case FLP_BlockLength:
				qDebug( "block length: %d\n", data );
				break;

			case FLP_UseLoopPoints:
				cc->sampleUseLoopPoints = true;
				break;

			case FLP_LoopType:
				qDebug( "loop type: %d\n", data );
				break;

			case FLP_ChanType:
				qDebug( "channel type: %d\n", data );
				if( cc )
				{
		switch( data )
		{
			case 0: cc->pluginType = FL_Plugin::Sampler; break;
			case 1: cc->pluginType = FL_Plugin::TS404; break;
//			case 2: cc->pluginType = FL_Plugin::Fruity_3x_Osc; break;
			case 3: cc->pluginType = FL_Plugin::Layer; break;
			default:
				break;
		}
				}
				break;

			case FLP_MixSliceNum:
				cc->fxChannel = data+1;
				break;

			case FLP_EffectChannelMuted:
if( p.currentEffectChannel <= NumFxChannels )
{
	p.effectChannels[p.currentEffectChannel].isMuted =
					( data & 0x08 ) > 0 ? false : true;
}
				break;


			// WORD EVENTS
			case FLP_NewChan:
				cur_channel = data;
				qDebug( "new channel: %d\n", data );
				break;

			case FLP_NewPat:
				p.currentPattern = data - 1;
				if( p.currentPattern > p.maxPatterns )
				{
					p.maxPatterns = p.currentPattern;
				}
				break;

			case FLP_Tempo:
				p.tempo = data;
				break;

			case FLP_CurrentPatNum:
				p.activeEditPattern = data;
				break;

			case FLP_FX:
				qDebug( "FX: %d\n", data );
				break;

			case FLP_Fade_Stereo:
				if( data & 0x02 )
				{
					cc->sampleReversed = true;
				}
				else if( data & 0x100 )
				{
					cc->sampleReverseStereo = true;
				}
				qDebug( "fade stereo: %d\n", data );
				break;

			case FLP_CutOff:
				qDebug( "cutoff (sample): %d\n", data );
				break;

			case FLP_PreAmp:
				cc->sampleAmp = 100 + data * 100 / 256;
				break;

			case FLP_Decay:
				qDebug( "decay (sample): %d\n", data );
				break;

			case FLP_Attack:
				qDebug( "attack (sample): %d\n", data );
				break;

			case FLP_MainPitch:
				p.mainPitch = data;
				break;

			case FLP_Resonance:
				qDebug( "reso (sample): %d\n", data );
				break;

			case FLP_LoopBar:
				qDebug( "loop bar: %d\n", data );
				break;

			case FLP_StDel:
				qDebug( "stdel (delay?): %d\n", data );
				break;

			case FLP_FX3:
				qDebug( "FX 3: %d\n", data );
				break;

			case FLP_ShiftDelay:
				qDebug( "shift delay: %d\n", data );
				break;

			case FLP_Dot:
				cc->dots.push_back( ( data & 0xff ) +
						( p.currentPattern << 8 ) );
				break;

			case FLP_LayerChans:
				p.channels[data].layerParent = cur_channel;
				break;

			// DWORD EVENTS
			case FLP_Color:
				cc->color = data;
				break;

			case FLP_PlayListItem:
			{
				FL_PlayListItem i;
				i.position = ( data & 0xffff ) *
							DefaultTicksPerTact;
				i.length = DefaultTicksPerTact;
				i.pattern = ( data >> 16 ) - 1;
				p.playListItems.push_back( i );
				if( i.pattern > p.maxPatterns )
				{
					p.maxPatterns = i.pattern;
				}
				break;
			}

			case FLP_FXSine:
				qDebug( "fx sine: %d\n", data );
				break;

			case FLP_CutCutBy:
				qDebug( "cut cut by: %d\n", data );
				break;

			case FLP_MiddleNote:
				cc->baseNote = data+9;
				break;

			case FLP_DelayReso:
				qDebug( "delay resonance: %d\n", data );
				break;

			case FLP_Reverb:
				qDebug( "reverb (sample): %d\n", data );
				break;

			case FLP_IntStretch:
				qDebug( "int stretch (sample): %d\n", data );
				break;

			// TEXT EVENTS
			case FLP_Text_ChanName:
				cc->name = text;
				break;

			case FLP_Text_PatName:
				p.patternNames[p.currentPattern] = text;
				break;

			case FLP_Text_CommentRTF:
			{
				QByteArray ba( text, text_len );
				QBuffer buf( &ba );
				buf.open( QBuffer::ReadOnly );
				lineno = 0;
				attr_clear_all();
				op = html_init();
				hash_init();
				Word * word = word_read( &buf );
				QString out;
				word_print( word, out );
				word_free( word );
				op_free( op );

				p.projectNotes = out;
				outstring = "";
				break;
			}

			case FLP_Text_Title:
				p.projectTitle = text;
				break;

			case FLP_Text_SampleFileName:
			{
				QString f = text;
/*				if( f.mid( 1, 11 ) == "Instruments" )
				{
					f = "\\Patches\\Packs" +
								f.mid( 12 );
				}*/
				f.replace( '\\', QDir::separator() );
				if( QFileInfo( configManager::inst()->flDir() +
						"/Data/" ).exists() )
				{
					f = configManager::inst()->flDir() +
								"/Data/" + f;
				}
				else
				{
					// FL 3 compat
					f = configManager::inst()->flDir() +
							"/Samples/" + f;
				}
				cc->sampleFileName = f;
				break;
			}

			case FLP_Text_Version:
			{
				qDebug( "FLP version: %s\n", text );
				p.versionString = text;
				QStringList l = p.versionString.split( '.' );
				p.version = ( l[0].toInt() << 8 ) +
						( l[1].toInt() << 4 ) +
						( l[2].toInt() << 0 );
				if( p.version >= 0x600 )
				{
					p.versionSpecificFactor = 100;
				}
				break;
			}

			case FLP_Text_PluginName:
				if( mappedPluginTypes.
					contains( QString( text ).toLower() ) )
				{
	const FL_Plugin::PluginTypes t = static_cast<FL_Plugin::PluginTypes>(
				mappedPluginTypes[QString( text ).toLower()] );
					if( t > FL_Plugin::EffectPlugin )
					{
						qDebug( "recognized new effect %s\n", text );
						p.effects.push_back( FL_Effect( t ) );
					}
					else if( cc )
					{
						qDebug( "recognized new plugin %s\n", text );
						cc->pluginType = t;
					}
					last_plugin_type = t;
				}
				else
				{
					qDebug( "unsupported plugin: %s!\n", text );
				}
				break;

			case FLP_Text_EffectChanName:
				++p.currentEffectChannel;
				if( p.currentEffectChannel <= NumFxChannels )
				{
					p.effectChannels[p.currentEffectChannel].name = text;
				}
				break;

			case FLP_Text_Delay:
				qDebug( "delay data: " );
				// pi[1] seems to be volume or similiar and
				// needs to be divided
				// by p.versionSpecificFactor
				dump_mem( text, text_len );
				break;

			case FLP_Text_TS404Params:
				if( cc && cc->pluginType == FL_Plugin::UnknownPlugin &&
						cc->pluginSettings == NULL )
				{
					cc->pluginSettings = new char[text_len];
					memcpy( cc->pluginSettings, text, text_len );
					cc->pluginSettingsLength = text_len;
					cc->pluginType = FL_Plugin::TS404;
				}
				break;

			case FLP_Text_NewPlugin:
				if( last_plugin_type > FL_Plugin::EffectPlugin )
				{
					FL_Effect * e = &p.effects.last();
					e->fxChannel = puc[0];
					e->fxPos = puc[4];
					qDebug( "new effect: " );
				}
				else
				{
					qDebug( "new plugin: " );
				}
				dump_mem( text, text_len );
				break;

			case FLP_Text_PluginParams:
				if( cc && cc->pluginSettings == NULL )
				{
					cc->pluginSettings = new char[text_len];
					memcpy( cc->pluginSettings, text,
								text_len );
					cc->pluginSettingsLength = text_len;
				}
				qDebug( "plugin params: " );
				dump_mem( text, text_len );
				break;

			case FLP_Text_ChanParams:
				cc->arpDir = mappedArpDir[pi[10]];
				cc->arpRange = pi[11];
				cc->selectedArp = pi[12];
	if( cc->selectedArp < 8 )
	{
		const int mappedArps[] = { 0, 1, 5, 6, 2, 3, 4 } ;
		cc->selectedArp = mappedArps[cc->selectedArp];
	}
				cc->arpTime = ( ( pi[13]+1 ) * p.tempo ) /
								( 4*16 ) + 1;
				cc->arpGate = ( pi[14] * 100.0f ) / 48.0f;
				cc->arpEnabled = pi[10] > 0;

				qDebug( "channel params: " );
				dump_mem( text, text_len );
				break;

			case FLP_Text_EnvLfoParams:
			{
				const float scaling = 1.0 / 65536.0f;
				FL_Channel_Envelope e;

		switch( cc->envelopes.size() )
		{
			case 1:
				e.target = InstrumentSoundShaping::Volume;
				break;
			case 2:
				e.target = InstrumentSoundShaping::Cut;
				break;
			case 3:
				e.target = InstrumentSoundShaping::Resonance;
				break;
			default:
				e.target = InstrumentSoundShaping::NumTargets;
				break;
		}
				e.predelay = pi[2] * scaling;
				e.attack = pi[3] * scaling;
				e.hold = pi[4] * scaling;
				e.decay = pi[5] * scaling;
				e.sustain = 1-pi[6] / 128.0f;
				e.release = pi[7] * scaling;
				if( e.target == InstrumentSoundShaping::Volume )
				{
					e.amount = pi[1] ? 1 : 0;
				}
				else
				{
					e.amount = pi[8] / 128.0f;
				}
//				e.lfoAmount = pi[11] / 128.0f;
				cc->envelopes.push_back( e );

				qDebug( "envelope and lfo params:\n" );
				dump_mem( text, text_len );

				break;
			}

			case FLP_Text_BasicChanParams:
		cc->volume = ( pi[1] / p.versionSpecificFactor ) * 100 / 128;
		cc->panning = ( pi[0] / p.versionSpecificFactor ) * 200 / 128 -
								PanningRight;
				if( text_len > 12 )
				{
			cc->filterType = mappedFilter[puc[20]];
			cc->filterCut = puc[12] / ( 255.0f * 2.5f );
			cc->filterRes = 0.01f + puc[16] / ( 256.0f * 2 );
			cc->filterEnabled = ( puc[13] == 0 );
			if( puc[20] >= 6 )
			{
				cc->filterCut *= 0.5f;
			}
				}
				qDebug( "basic chan params: " );
				dump_mem( text, text_len );
				break;

			case FLP_Text_OldFilterParams:
				cc->filterType = mappedFilter[puc[8]];
				cc->filterCut = puc[0] / ( 255.0f * 2.5 );
				cc->filterRes = 0.1f + puc[4] / ( 256.0f * 2 );
				cc->filterEnabled = ( puc[1] == 0 );
				if( puc[8] >= 6 )
				{
					cc->filterCut *= 0.5;
				}
				qDebug( "old filter params: " );
				dump_mem( text, text_len );
				break;

			case FLP_Text_AutomationData:
			{
				const int bpae = 12;
				const int imax = text_len / bpae;
				qDebug( "automation data (%d items)\n", imax );
				for( int i = 0; i < imax; ++i )
				{
					FL_Automation a;
					a.pos = pi[3*i+0] /
						( 4*ppq / DefaultTicksPerTact );
					a.value = pi[3*i+2];
					a.channel = pi[3*i+1] >> 16;
					a.control = pi[3*i+1] & 0xffff;
					if( a.channel >= 0 &&
						a.channel < p.numChannels )
					{
						qDebug( "add channel %d at %d  val %d  control:%d\n",
									a.channel, a.pos, a.value, a.control );
						p.channels[a.channel].automationData += a;
					}
//					dump_mem( text+i*bpae, bpae );
				}
				break;
			}

			case FLP_Text_PatternNotes:
			{
				//dump_mem( text, text_len );
				const int bpn = 20;
				const int imax = ( text_len + bpn - 1 ) / bpn;
				for( int i = 0; i < imax; ++i )
				{
					int ch = *( puc + i*bpn + 6 );
					int pan = *( puc + i*bpn + 16 );
					int vol = *( puc + i*bpn + 17 );
					int pos = *( (int *)( puc + i*bpn ) );
					int key = *( puc + i*bpn + 12 );
					int len = *( (int*)( puc + i*bpn +
									8 ) );
					pos /= (4*ppq) / DefaultTicksPerTact;
					len /= (4*ppq) / DefaultTicksPerTact;
					note n( len, pos, key, vol * 100 / 128,
							pan*200 / 128 - 100 );
					if( ch < p.numChannels )
					{
	p.channels[ch].notes.push_back( qMakePair( p.currentPattern, n ) );
					}
					else
					{
						qDebug( "invalid " );
					}
					qDebug( "note: " );
					dump_mem( text+i*bpn, bpn );
				}
				break;
			}

			case FLP_Text_ChanGroupName:
				qDebug( "channel group name: %s\n", text );
				break;

			// case 216: pi[2] /= p.versionSpecificFactor
			// case 229: pi[1] /= p.versionSpecificFactor

			case FLP_Event_EffectParams:
			{
				enum FLP_EffectParams
				{
					EffectParamVolume = 0x1fc0
				} ;

				const int bpi = 12;
				const int imax = text_len / bpi;
				for( int i = 0; i < imax; ++i )
				{
					const int param = pi[i*3+1] & 0xffff;
					const int ch = ( pi[i*3+1] >> 22 )
									& 0x7f;
					if( ch < 0 || ch > NumFxChannels )
					{
						continue;
					}
					const int val = pi[i*3+2];
					if( param == EffectParamVolume )
					{
p.effectChannels[ch].volume = ( val / p.versionSpecificFactor ) * 100 / 128;
					}
					else
					{
qDebug( "FX-ch: %d  param: %x  value:%x\n", ch, param, val );
					}
				}
				break;
			}

			case FLP_Event_PlaylistItems:	// playlist items
			{
				const int bpi = 28;
				const int imax = text_len / bpi;
				for( int i = 0; i < imax; ++i )
				{
const int pos = pi[i*bpi/sizeof(int)+0] / ( (4*ppq) / DefaultTicksPerTact );
const int len = pi[i*bpi/sizeof(int)+2] / ( (4*ppq) / DefaultTicksPerTact );
const int pat = pi[i*bpi/sizeof(int)+3] & 0xfff;
if( pat > 2146 && pat <= 2278 )	// whatever these magic numbers are for...
{
	FL_PlayListItem i;
	i.position = pos;
	i.length = len;
	i.pattern = 2278 - pat;
	p.playListItems += i;
}
else
{
	qDebug( "unknown playlist item: " );
	dump_mem( text+i*bpi, bpi );
}
				}
				break;
			}

			default:
				if( ev >= FLP_Text )
				{
					qDebug( "!! unhandled text (ev: %d, len: %d): ",
								ev, text_len );
					dump_mem( text, text_len );
				}
				else
				{
					qDebug( "!! handling of FLP-event %d not implemented yet "
							"(data=%d).\n", ev, data );
				}
				break;
		}
	}


	// now create a project from FL_Project data structure

	engine::getSong()->clearProject();

	// set global parameters
	engine::getSong()->setMasterVolume( p.mainVolume );
	engine::getSong()->setMasterPitch( p.mainPitch );
	engine::getSong()->setTempo( p.tempo );

	// set project notes
	engine::getProjectNotes()->setText( p.projectNotes );


	progressDialog.setMaximum( p.maxPatterns + p.channels.size() +
								p.effects.size() );
	int cur_progress = 0;

	// create BB tracks
	QList<bbTrack *> bb_tracks;
	QList<InstrumentTrack *> i_tracks;

	while( engine::getBBTrackContainer()->numOfBBs() <= p.maxPatterns )
	{
		const int cur_pat = bb_tracks.size();
		bbTrack * bbt = dynamic_cast<bbTrack *>(
			track::create( track::BBTrack, engine::getSong() ) );
		if( p.patternNames.contains( cur_pat ) )
		{
			bbt->setName( p.patternNames[cur_pat] );
		}
		bb_tracks += bbt;
		progressDialog.setValue( ++cur_progress );
		qApp->processEvents();
	}

	// create instrument-track for each channel
	for( QList<FL_Channel>::Iterator it = p.channels.begin();
						it != p.channels.end(); ++it )
	{
		InstrumentTrack * t = dynamic_cast<InstrumentTrack *>(
			track::create( track::InstrumentTrack,
					engine::getBBTrackContainer() ) );
		engine::getBBTrackContainer()->updateAfterTrackAdd();
		i_tracks.push_back( t );
		switch( it->pluginType )
		{
			case FL_Plugin::Fruity_3x_Osc:
				it->instrumentPlugin =
					t->loadInstrument( "tripleoscillator" );
				break;
			case FL_Plugin::Plucked:
				it->instrumentPlugin =
					t->loadInstrument( "vibedstrings" );
				break;
			case FL_Plugin::FruitKick:
				it->instrumentPlugin =
					t->loadInstrument( "kicker" );
				break;
			case FL_Plugin::TS404:
				it->instrumentPlugin =
					t->loadInstrument( "lb302" );
				break;
			case FL_Plugin::FruitySoundfontPlayer:
				it->instrumentPlugin =
					t->loadInstrument( "sf2player" );
				break;
			case FL_Plugin::Sampler:
			case FL_Plugin::UnknownPlugin:
			default:
				it->instrumentPlugin =
					t->loadInstrument( "audiofileprocessor" );
				break;
		}
		processPluginParams( &( *it ) );

		t->setName( it->name );
		t->volumeModel()->setValue( it->volume );
		t->panningModel()->setValue( it->panning );
		t->baseNoteModel()->setValue( it->baseNote );
		t->effectChannelModel()->setValue( it->fxChannel );

		InstrumentSoundShaping * iss = &t->m_soundShaping;
		iss->m_filterModel.setValue( it->filterType );
		iss->m_filterCutModel.setValue( it->filterCut *
			( iss->m_filterCutModel.maxValue() -
				iss->m_filterCutModel.minValue() ) +
					iss->m_filterCutModel.minValue() );
		iss->m_filterResModel.setValue( it->filterRes *
			( iss->m_filterResModel.maxValue() -
				iss->m_filterResModel.minValue() ) +
					iss->m_filterResModel.minValue() );
		iss->m_filterEnabledModel.setValue( it->filterEnabled );

		for( QList<FL_Channel_Envelope>::iterator jt = it->envelopes.begin();
					jt != it->envelopes.end(); ++jt )
		{
			if( jt->target != InstrumentSoundShaping::NumTargets )
			{
				EnvelopeAndLfoParameters * elp =
					iss->m_envLfoParameters[jt->target];

				elp->m_predelayModel.setValue( jt->predelay );
				elp->m_attackModel.setValue( jt->attack );
				elp->m_holdModel.setValue( jt->hold );
				elp->m_decayModel.setValue( jt->decay );
				elp->m_sustainModel.setValue( jt->sustain );
				elp->m_releaseModel.setValue( jt->release );
				elp->m_amountModel.setValue( jt->amount );
				elp->updateSampleVars();
			}
		}

		InstrumentFunctionArpeggio * arp = &t->m_arpeggio;
		arp->m_arpDirectionModel.setValue( it->arpDir );
		arp->m_arpRangeModel.setValue( it->arpRange );
		arp->m_arpModel.setValue( it->selectedArp );
		arp->m_arpTimeModel.setValue( it->arpTime );
		arp->m_arpGateModel.setValue( it->arpGate );
		arp->m_arpEnabledModel.setValue( it->arpEnabled );

		// process all dots
		for( QList<int>::ConstIterator jt = it->dots.begin();
						jt != it->dots.end(); ++jt )
		{
			const int pat = *jt / 256;
			const int pos = *jt % 256;
			pattern * p =
				dynamic_cast<pattern *>( t->getTCO( pat ) );
			if( p == NULL )
			{
				continue;
			}
			p->setStep( pos, true );
		}

		// TODO: use future layering feature
		if( it->layerParent >= 0 )
		{
			it->notes += p.channels[it->layerParent].notes;
		}

		// process all notes
		for( FL_Channel::noteVector::ConstIterator jt = it->notes.begin();
						jt != it->notes.end(); ++jt )
		{
			const int pat = jt->first;

			if( pat > 100 )
			{
				continue;
			}
			pattern * p = dynamic_cast<pattern *>( t->getTCO( pat ) );
			if( p != NULL )
			{
				p->addNote( jt->second, false );
			}
		}

		// process automation data
		for( QList<FL_Automation>::ConstIterator jt =
						it->automationData.begin();
					jt != it->automationData.end(); ++jt )
		{
			AutomatableModel * m = NULL;
			float value = jt->value;
			bool scale = false;
			switch( jt->control )
			{
				case FL_Automation::ControlVolume:
					m = t->volumeModel();
					value *= ( 100.0f / 128.0f ) / p.versionSpecificFactor;
					break;
				case FL_Automation::ControlPanning:
					m = t->panningModel();
	value = ( value / p.versionSpecificFactor ) *200/128 - PanningRight;
					break;
				case FL_Automation::ControlPitch:
					m = t->pitchModel();
					break;
				case FL_Automation::ControlFXChannel:
					m = t->effectChannelModel();
					value = value*200/128 - PanningRight;
					break;
				case FL_Automation::ControlFilterCut:
					scale = true;
					m = &t->m_soundShaping.m_filterCutModel;
					value /= ( 255 * 2.5f );
					break;
				case FL_Automation::ControlFilterRes:
					scale = true;
					m = &t->m_soundShaping.m_filterResModel;
					value = 0.1f + value / ( 256.0f * 2 );
					break;
				case FL_Automation::ControlFilterType:
					m = &t->m_soundShaping.m_filterModel;
					value = mappedFilter[jt->value];
					break;
				default:
					qDebug( "handling automation data of "
							"control %d not implemented "
							"yet\n", jt->control );
					break;
			}
			if( m )
			{
if( scale )
{
	value = m->minValue<float>() + value *
				( m->maxValue<float>() - m->minValue<float>() );
}
AutomationPattern * p = AutomationPattern::globalAutomationPattern( m );
p->putValue( jt->pos, value, false );
			}
		}

		progressDialog.setValue( ++cur_progress );
		qApp->processEvents();
	}

	// process all effects
	EffectKeyList effKeys;
	Plugin::DescriptorList pluginDescs;
	Plugin::getDescriptorsOfAvailPlugins( pluginDescs );
	for( Plugin::DescriptorList::ConstIterator it = pluginDescs.begin();
											it != pluginDescs.end(); ++it )
	{
		if( it->type != Plugin::Effect )
		{
			continue;
		}
		if( it->subPluginFeatures )
		{
			it->subPluginFeatures->listSubPluginKeys( &( *it ), effKeys );
		}
		else
		{
			effKeys << EffectKey( &( *it ), it->name );
		}
	}

	for( int fx_ch = 0; fx_ch <= NumFxChannels ; ++fx_ch )
	{
		FxChannel * ch = engine::fxMixer()->effectChannel( fx_ch );
		if( !ch )
		{
			continue;
		}
		FL_EffectChannel * flch = &p.effectChannels[fx_ch];
		if( !flch->name.isEmpty() )
		{
			ch->m_name = flch->name;
		}
		ch->m_volumeModel.setValue( flch->volume / 100.0f );
		ch->m_muteModel.setValue( flch->isMuted );
	}

	for( QList<FL_Effect>::ConstIterator it = p.effects.begin();
										it != p.effects.end(); ++it )
	{
		QString effName;
		switch( it->pluginType )
		{
			case FL_Plugin::Fruity7BandEq:
				effName = "C* Eq2x2";
				break;
			case FL_Plugin::FruityBassBoost:
				effName = "BassBooster";
				break;
			case FL_Plugin::FruityChorus:
				effName = "TAP Chorus";
				break;
			case FL_Plugin::FruityCompressor:
				//effName = "C* Compress";
				effName = "Fast Lookahead limiter";
				break;
			case FL_Plugin::FruityDelay:
			case FL_Plugin::FruityDelay2:
//				effName = "Feedback Delay Line (Maximum Delay 5s)";
				break;
			case FL_Plugin::FruityBloodOverdrive:
			case FL_Plugin::FruityFastDist:
			case FL_Plugin::FruitySoftClipper:
				effName = "C* Clip";
				break;
			case FL_Plugin::FruityFastLP:
				effName = "Low Pass Filter";
				break;
			case FL_Plugin::FruityPhaser:
				effName = "C* PhaserI";
				break;
			case FL_Plugin::FruityReeverb:
				effName = "C* Plate2x2";
				break;
			case FL_Plugin::FruitySpectroman:
				effName = "Spectrum Analyzer";
				break;
			default:
				break;
		}
		if( effName.isEmpty() || it->fxChannel < 0 ||
						it->fxChannel > NumFxChannels )
		{
			continue;
		}
		EffectChain * ec = &engine::fxMixer()->
					effectChannel( it->fxChannel )->m_fxChain;
		qDebug( "adding %s to %d\n", effName.toUtf8().constData(),
								it->fxChannel );
		for( EffectKeyList::Iterator jt = effKeys.begin();
						jt != effKeys.end(); ++jt )
		{
			if( QString( jt->desc->displayName ).contains( effName ) ||
				( jt->desc->subPluginFeatures != NULL &&
					jt->name.contains( effName ) ) )
			{
				qDebug( "instantiate %s\n", jt->desc->name );
				::Effect * e = Effect::instantiate( jt->desc->name, ec, &( *jt ) );
				ec->appendEffect( e );
				ec->setEnabled( true );
				break;
			}
		}

		progressDialog.setValue( ++cur_progress );
		qApp->processEvents();
	}



	// process all playlist-items
	for( QList<FL_PlayListItem>::ConstIterator it = p.playListItems.begin();
					it != p.playListItems.end(); ++it )
	{
		if( it->pattern > p.maxPatterns )
		{
			continue;
		}
		trackContentObject * tco = bb_tracks[it->pattern]->createTCO( MidiTime() );
		tco->movePosition( it->position );
		if( it->length != DefaultTicksPerTact )
		{
			tco->changeLength( it->length );
		}
	}



	// set current pattern
	if( p.activeEditPattern < engine::getBBTrackContainer()->numOfBBs() )
	{
		engine::getBBTrackContainer()->setCurrentBB(
							p.activeEditPattern );
	}

	// restore journalling settings
	engine::projectJournal()->setJournalling( is_journ );

	return true;
}




void FlpImport::processPluginParams( FL_Channel * _ch )
{
	qDebug( "plugin params for plugin %d (%d bytes): ", _ch->pluginType,
						_ch->pluginSettingsLength );
	dump_mem( _ch->pluginSettings, _ch->pluginSettingsLength );
	switch( _ch->pluginType )
	{
		case FL_Plugin::Sampler:	// AudioFileProcessor loaded
		{
			QDomDocument dd;
			QDomElement de = dd.createElement(
					_ch->instrumentPlugin->nodeName() );
			de.setAttribute( "reversed", _ch->sampleReversed );
			de.setAttribute( "amp", _ch->sampleAmp );
			de.setAttribute( "looped", _ch->sampleUseLoopPoints );
			de.setAttribute( "sframe", 0 );
			de.setAttribute( "eframe", 1 );
			de.setAttribute( "src", _ch->sampleFileName );
			_ch->instrumentPlugin->restoreState( de );
			break;
		}

		case FL_Plugin::TS404:		// LB302 loaded
			break;

		case FL_Plugin::Fruity_3x_Osc:	// TripleOscillator loaded
		{
			const Oscillator::WaveShapes mapped_3xOsc_Shapes[] =
			{
				Oscillator::SineWave,
				Oscillator::TriangleWave,
				Oscillator::SquareWave,
				Oscillator::SawWave,
				Oscillator::SquareWave,	// square-sin
				Oscillator::WhiteNoise,
				Oscillator::UserDefinedWave
			} ;

			QDomDocument dd;
			QDomElement de = dd.createElement(
					_ch->instrumentPlugin->nodeName() );
			de.setAttribute( "modalgo1", Oscillator::SignalMix );
			de.setAttribute( "modalgo2", Oscillator::SignalMix );
			int ws = Oscillator::UserDefinedWave;
			for( int i = 0; i < 3; ++i )
			{
				const int32_t * d = (const int32_t *)
					( _ch->pluginSettings + i * 28 );
				QString is = QString::number( i );
				de.setAttribute( "vol" + is,
					QString::number( d[0] * 100 /
								( 3 * 128 ) ) );
				de.setAttribute( "pan" + is,
						QString::number( d[1] ) );
				de.setAttribute( "coarse" + is,
						QString::number( d[3] ) );
				de.setAttribute( "finel" + is,
					QString::number( d[4] - d[6] / 2 ) );
				de.setAttribute( "finer" + is,
					QString::number( d[4] + d[6] / 2 ) );
				de.setAttribute( "stphdetun" + is,
						QString::number( d[5] ) );
				const int s = mapped_3xOsc_Shapes[d[2]];
				de.setAttribute( "wavetype" + is,
					QString::number( s ) );
				if( s != Oscillator::UserDefinedWave )
				{
					ws = s;
				}
			}
			if( ws == Oscillator::UserDefinedWave )
			{
				de.setAttribute( "wavetype0",
							Oscillator::SawWave );
			}
			de.setAttribute( "vol0", QString::number( 100/2 ) );
			// now apply the prepared plugin-state
			_ch->instrumentPlugin->restoreState( de );
			break;
		}

		case FL_Plugin::Layer:
			// nothing to do
			break;

		case FL_Plugin::Plucked:	// Vibed loaded
			// TODO: setup vibed-instrument
			break;

		case FL_Plugin::UnknownPlugin:
		default:
			qDebug( "handling of plugin params not implemented "
					"for current plugin\n" );
			break;
	}
}




extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new FlpImport( QString::fromUtf8(
									static_cast<const char *>( _data ) ) );
}

}

