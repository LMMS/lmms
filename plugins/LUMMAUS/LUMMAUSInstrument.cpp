//
// Created by seledreams on 06/03/2021.
//

#include <cassert>
#include "LUMMAUSInstrument.h"
#include "VocalInstrument.h"
#include "Engine.h"
#include "VocalInstrumentTrack.h"
#include "plugin_export.h"
#include "VocalNote.h"
#include <sstream>
#include <array>
#include <world4utau.h>
#include <QDir>
#include <QDebug>
#include <wavtool-yawu.h>
#include <thread>
#include "TrackContainer.h"
#include <chrono>
#include "Song.h"
using namespace std;


extern "C"{

Plugin::Descriptor PLUGIN_EXPORT LUMMAUS_plugin_descriptor =
	{
	STRINGIFY( PLUGIN_NAME ),
	"LUMMAUS",
	QT_TRANSLATE_NOOP( "PluginBrowser",
					   "Vocal Synthesizer" ),
	"SeleDreams <seledreams.contact/at/gmail.com>",
	0x0100,
	Plugin::VocalInstrument,
	new PluginPixmapLoader( "logo" ),
	NULL
};
// neccessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return( new LUMMAUSInstrument( static_cast<VocalInstrumentTrack *>( _data ) ) );
}

}


LUMMAUSInstrument::LUMMAUSInstrument(VocalInstrumentTrack *_vocal_instrument_track) :
VocalInstrument(_vocal_instrument_track,&LUMMAUS_plugin_descriptor)
{

	vb.Load(QDir::currentPath() + "/../data/voicebanks/Kumi Hitsuboku - English (Complete)");
}

LUMMAUSInstrumentView::LUMMAUSInstrumentView(VocalInstrument *_vocal_instrument, QWidget *_parent):
	InstrumentViewFixedSize(_vocal_instrument,_parent)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(),PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
}

LUMMAUSInstrumentView::~LUMMAUSInstrumentView()
{
}

PluginView *LUMMAUSInstrument::instantiateView(QWidget *_parent)
{
	return new LUMMAUSInstrumentView(this,_parent);
}

void LUMMAUSInstrument::bounceVocalPattern(VocalPattern *_pattern_to_bounce)
{
	_pattern_to_bounce->setName("Bouncing");
}

// Straight conversion to C++ from the UTSU function, credits to UTSU's devs
std::string LUMMAUSInstrument::convertNumberTo12Bit(int number){
	if (number < 0){
		number += 4096;
	}
	number = max(0,min(4095,number));
	std::array<int,2> thearray{};
	thearray[0] = number / 64;
	thearray[1] = number % 64;
	stringstream ss;
	for (int sixBitNumber : thearray){
		if (sixBitNumber >=0 && sixBitNumber < 26){
			ss << (char)(sixBitNumber + 'A');
		}
		else if(sixBitNumber >= 26 && sixBitNumber < 52){
			ss << (char)(sixBitNumber - 26 + 'a');
		}
		else if (sixBitNumber >= 52 && sixBitNumber < 62){
			ss << (char)(sixBitNumber - 52 + '0');
		}
		else if (sixBitNumber == 62){
			ss << '+';
		}
		else if (sixBitNumber == 63){
			ss << '/';
		}
		else {
			return "AA";
		}
	}
	std::string endString = ss.str();
	if (endString.length() != 2){
		return "AA";
	}
	return endString;
}

std::vector<float> *LUMMAUSInstrument::getUTAUPitchBend(VocalNote *_vocal_note)
{
	std::vector<float> *pitchBendPoints = new std::vector<float>();
	AutomationPattern *pitchBendAutomation = _vocal_note->detuning()->automationPattern();
	if (pitchBendAutomation){
		TimePos note_length = _vocal_note->length();
		TimePos time_per_point = 96 / note_length.ticksPerBeat(TimeSig(4,4));// 96;
		float addedValue;
		for (TimePos current_pos = 0;current_pos < note_length;current_pos += 1){
			float val = pitchBendAutomation->valueAt(current_pos);
			addedValue = (pitchBendAutomation->valueAt(current_pos + 1) - val) / time_per_point;
			for (int i = 0; i < time_per_point;i++){
				pitchBendPoints->push_back(val);
				val += addedValue;
			}
		}
	}
	return pitchBendPoints;
}
static constexpr std::array<const char*,12> m_nameList =
	{
		"C",
		"C#",
		"D",
		"D#",
		"E",
		"F",
		"F#",
		"G",
		"G#",
		"A",
		"A#",
		"B"
	};
std::string getNoteName(int key)
{
	return m_nameList[key % 12] +
		std::to_string(static_cast<int>(key / KeysPerOctave));
}

void LUMMAUSInstrument::renderUTAU(VocalPattern *vocalPattern)
{
	if (vb.empty()){
		return;
	}
	factor = 125.0 / Engine::getSong()->getTempo();
	QDir temporary_directory = QDir(QDir::currentPath() + "/temp_" + QString::number(vocalPattern->id()));
	temporary_directory.removeRecursively();
	temporary_directory.mkdir(QDir::currentPath() + "/temp_" + QString::number(vocalPattern->id()));
	std::string wavtool_output_name = QDir::currentPath().toStdString() + "/temp_" + to_string(vocalPattern->id()) + "/pattern-" + to_string(vocalPattern->id()) + "-wavtool.wav";
	std::string resampler_output_prefixes = QDir::currentPath().toStdString() + "/temp_" + to_string(vocalPattern->id()) + "/resampler-pattern-" + to_string(vocalPattern->id()) + "-";

	QVector<Note*> notes = vocalPattern->notes();

	for (int i = 0;i < notes.length();i++){
		if (!notes[i]){
			continue;
		}
		std::string note_resampler_path =resampler_output_prefixes + to_string(i) + ".wav";
		auto *vocalNote = dynamic_cast<VocalNote*>(notes[i]);
		assert(vocalNote != nullptr);
		std::string lyric = vocalNote->getLyric();
		const OtoItem *otoItem = vb.getOtoData(lyric);
		if (otoItem == nullptr){
			otoItem = vb.getOtoData("la");
		}
		CallResampler(otoItem,vocalNote, note_resampler_path);
		bool last = i == notes.length() - 1;
		if (i > 0)
		{
			TimePos distancebetween = vocalNote->pos() - (notes[i - 1]->pos() + notes[i-1]->length());
			if (distancebetween > 0){
				std::string silence_path = "";
				VocalNote tempNote;
				OtoItem item;
				tempNote.setLength(distancebetween);
				CallWavtool(&item,&tempNote,wavtool_output_name,silence_path,-1,false);
			}
		}
		else if (i == 0 && vocalNote->pos() > 0){
			TimePos distancebetween = vocalNote->pos();
			if (distancebetween > 0){
				std::string silence_path = "";
				VocalNote tempNote;
				OtoItem item;
				tempNote.setLength(distancebetween);
				CallWavtool(&item,&tempNote,wavtool_output_name,silence_path,-1,false);
			}
		}
		CallWavtool(otoItem,vocalNote,wavtool_output_name,note_resampler_path,i,last);

	}
	vocalPattern->getBouncedBuffer().setAudioFile(wavtool_output_name.c_str());
}

LUMMAUSInstrument::~LUMMAUSInstrument()
{
//	renderingThread->join();
}

void LUMMAUSInstrument::saveSettings(QDomDocument &_doc, QDomElement &_parent)
{

}
void LUMMAUSInstrument::loadSettings(const QDomElement &_this)
{

}

QString LUMMAUSInstrument::nodeName() const
{
	return( LUMMAUS_plugin_descriptor.name );
}

f_cnt_t LUMMAUSInstrument::desiredReleaseFrames() const
{
	return Instrument::desiredReleaseFrames();
}

void LUMMAUSInstrument::bounceVocalPatterns(VocalPattern **_patterns_to_bounce, int _count)
{
	for (int i = 0;i < _count;i++){
		renderUTAU(_patterns_to_bounce[i]);
	}
}

void LUMMAUSInstrument::CallResampler(const OtoItem *otoItem,VocalNote* note, const std::string &output_path,int id)
{
	stringstream pitch_curve;
	std::vector<float> *pitches = getUTAUPitchBend(note);
	for (int j = 0; j < pitches->size();j++){
		float pitch = pitches->at(j) * 100;
		pitch_curve << convertNumberTo12Bit((int)(pitch));
	}
	std::string endPitchCurve = pitch_curve.str();
	World4UTAUMain(
		(otoItem->oto->getDirectory() + "/" + otoItem->sample).toLocal8Bit().constData(),
		output_path.c_str(),
		getNoteName(note->key()).c_str(),
		note->getVelocity(),
		"?",
		otoItem->offset,
		note->length().getTimeInMilliseconds(Engine::getSong()->getTempo()),
		otoItem->consonant,
		otoItem->cutoff,
		1,
		1,
		Engine::getSong()->getTempo(),
		endPitchCurve.c_str());
}

void LUMMAUSInstrument::CallWavtool(const OtoItem *otoItem, VocalNote *note,const std::string &output_path, const std::string &input_path, int id, bool last)
{
	setlocale(LC_ALL,nullptr);
	double overlap = id > 0 ? otoItem->overlap * factor : 0;
	WAVTOOL_YAWU::process(
		output_path,
		input_path,
		0,
		note->length().getTimeInMilliseconds(Engine::getSong()->getTempo()) + (otoItem->preutterance * factor),
		0,
		0,
		0,
		100,
		100,
		100,
		100,
		overlap,
		0,
		0,
		0,
		last
	);
}

