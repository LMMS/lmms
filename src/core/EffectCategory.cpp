/*
 * EffectCategory - Associates an effect to a category (for grouping/filtering afterwards)
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

#include "EffectCategory.h"

#include <qlist.h>
#include <qobject.h>

namespace lmms {

const std::map<QString, QString> lmmsEffects = 
{
	{"Amplifier", "Amplifier"},
	{"BassBooster", "Equalization"},
	{"Bitcrush", "Bitcrush"},
	{"Compressor", "Compressor"},
	{"Crossover Equalizer", "Equalization"},
	{"Delay", "Delay"},
	{"Dispersion", "Filter"},
	{"Dual Filter", "Filter"},
	{"Dynamics Processor", "Distortion"},
	{"Equalizer", "Equalization"},
	{"Flanger", "Flanger"},
	{"Frequency Shifter", "Pitch"},
	{"Granular Pitch Shifter", "Pitch"},
	{"LOMM", "Distortion"},
	{"Multitap Echo", "Delay"},
	{"Oscilloscope", "Tool"},
	{"Peak Controller", "Automation"},
	{"ReverbSC", "Reverb"},
	{"Slew Distortion", "Distortion"},
	{"Spectrum Analyzer", "Tool"},
	{"Stereo Matrix", "Stereo"},
	{"StereoEnhancer Effect", "Stereo"},
	{"Vectorscope", "Tool"},
	{"Waveshaper Effect", "Distortion"}
};

const std::map<QString, QString> ladspaEffects = 
{
	{"4 x 4 pole allpass", "Filter"},
	{"AM pitchshifter", "Pitch"},
	{"Aliasing", "Distortion"},
	{"Allpass delay line, cubic spline interpolation", "Delay"},
	{"Allpass delay line, linear interpolation", "Delay"},
	{"Allpass delay line, noninterpolating", "Delay"},
	{"Amplifier (Mono)", "Amplifier"},
	{"Amplifier (Stereo)", "Amplifier"},
	{"Artificial latency", "Other"},
	{"Audio Divider (Suboctave Generator)", "Distortion"},
	{"Auto phaser", "Phaser"},
	{"Barry's Satan Maximiser", "Distortion"},
	{"C* AmpIII - Tube amp", "Amplifier"},
	{"C* AmpIV - Tube amp + tone controls", "Amplifier"},
	{"C* AmpV - Tube amp", "Amplifier"},
	{"C* AmpVTS - Tube amp + Tone stack", "Amplifier"},
	{"C* AutoFilter - Self-modulating resonant filter", "Filter"},
	{"C* AutoWah - Resonant envelope-following filter", "Wah"},
	{"C* CabinetI - Loudspeaker cabinet emulation", "Amplifier"},
	{"C* CabinetII - Refined loudspeaker cabinet emulation", "Amplifier"},
	{"C* CabinetIII - Simplistic loudspeaker cabinet emulation", "Amplifier"},
	{"C* CabinetIV - Idealised loudspeaker cabinet", "Amplifier"},
	{"C* ChorusI - Mono chorus/flanger", "Chorus"},
	{"C* ChorusII - Mono chorus/flanger modulated by a fractal", "Chorus"},
	{"C* Clip - Hard clipper, 8x oversampled", "Distortion"},
	{"C* Compress - Mono compressor", "Compressor"},
	{"C* CompressX2 - Stereo compressor and saturating limiter", "Compressor"},
	{"C* Eq - 10-band equalizer", "Equalization"},
	{"C* Eq10 - 10-band equaliser", "Equalization"},
	{"C* Eq10X2 - Stereo 10-band equaliser", "Equalization"},
	{"C* Eq2x2 - stereo 10-band equalizer", "Equalization"},
	{"C* Eq4p - 4-band parametric shelving equaliser", "Equalization"},
	{"C* EqFA4p - 4-band parametric eq", "Equalization"},
	{"C* Narrower - Stereo image width reduction", "Stereo"},
	{"C* Noisegate - Attenuating hum and noise", "Gate"},
	{"C* PhaserI - Mono phaser", "Phaser"},
	{"C* PhaserII - Mono phaser modulated by a Lorenz fractal", "Phaser"},
	{"C* Plate2x2 - Versatile plate reverb, stereo inputs", "Reverb"},
	{"C* PlateX2 - Versatile plate reverb, stereo inputs", "Reverb"},
	{"C* PreampIII - Tube preamp emulation", "Amplifier"},
	{"C* PreampIV - Tube preamp emulation + tone controls", "Amplifier"},
	{"C* Saturate - Various static nonlinearities, 8x oversampled", "Distortion"},
	{"C* Spice - Not an exciter", "Equalization"},
	{"C* SpiceX2 - Not an exciter either", "Equalization"},
	{"C* SweepVFI - Resonant filter swept by a Lorenz fractal", "Filter"},
	{"C* SweepVFII - Resonant filter, f and Q swept by a Lorenz fractal", "Filter"},
	{"C* ToneStack - Tone stack emulation", "Filter"},
	{"C* ToneStackLT - Tone stack emulation, lattice filter 44.1", "Filter"},
	{"Calf Analyzer LADSPA", "Tool"},
	{"Calf Bass Enhancer LADSPA", "Equalization"},
	{"Calf Compensation Delay Line LADSPA", "Delay"},
	{"Calf Compressor LADSPA", "Compressor"},
	{"Calf Crusher LADSPA", "Distortion"},
	{"Calf Deesser LADSPA", "Deesser"},
	{"Calf Emphasis LADSPA", "Filter"},
	{"Calf Envelope Filter LADSPA", "Wah"},
	{"Calf Equalizer 12 Band LADSPA", "Equalization"},
	{"Calf Equalizer 30 Band LADSPA", "Equalization"},
	{"Calf Equalizer 5 Band LADSPA", "Equalization"},
	{"Calf Equalizer 8 Band LADSPA", "Equalization"},
	{"Calf Exciter LADSPA", "Distortion"},
	{"Calf Filter LADSPA", "Filter"},
	{"Calf Filterclavier LADSPA", "Filter"},
	{"Calf Flanger LADSPA", "Flanger"},
	{"Calf Gate LADSPA", "Gate"},
	{"Calf Haas Stereo Enhancer LADSPA", "Stereo"},
	{"Calf Limiter LADSPA", "Limiter"},
	{"Calf Mono Compressor LADSPA", "Compressor"},
	{"Calf Multi Chorus LADSPA", "Chorus"},
	{"Calf Multi Spread LADSPA", "Stereo"},
	{"Calf Multiband Compressor LADSPA", "Compressor"},
	{"Calf Multiband Enhancer LADSPA", "Distortion"},
	{"Calf Multiband Gate LADSPA", "Gate"},
	{"Calf Multiband Limiter LADSPA", "Limiter"},
	{"Calf Phaser LADSPA", "Phaser"},
	{"Calf Psychoachoustic Clipper LADSPA", "Distortion"},
	{"Calf Pulsator LADSPA", "Stereo"},
	{"Calf Reverb LADSPA", "Reverb"},
	{"Calf Reverse Delay LADSPA", "Delay"},
	{"Calf Ring Modulator LADSPA", "LFO"},
	{"Calf Rotary Speaker LADSPA", "Stereo"},
	{"Calf Saturator LADSPA", "Distortion"},
	{"Calf Sidechain Compressor LADSPA", "Compressor"},
	{"Calf Sidechain Gate LADSPA", "Gate"},
	{"Calf Sidechain Limiter LADSPA", "Limiter"},
	{"Calf Stereo Tools LADSPA", "Stereo"},
	{"Calf Tape Simulator LADSPA", "Distortion"},
	{"Calf Transient Designer LADSPA", "Equalization"},
	{"Calf Vintage Delay LADSPA", "Delay"},
	{"Canyon Delay", "Delay"},
	{"Chebyshev distortion", "Distortion"},
	{"Comb Filter", "Filter"},
	{"Comb delay line, cubic spline interpolation", "Delay"},
	{"Comb delay line, linear interpolation", "Delay"},
	{"Comb delay line, noninterpolating", "Delay"},
	{"Constant Signal Generator", "Tool"},
	{"Crossover distortion", "Distortion"},
	{"DC Offset Remover", "Tool"},
	{"DJ EQ", "Equalization"},
	{"DJ EQ (mono)", "Equalization"},
	{"DJ flanger", "Flanger"},
	{"Debug Plugin", "Tool"},
	{"Decimator", "Distortion"},
	{"Declipper", "Distortion"},
	{"Delayorama", "Delay"},
	{"Diode Processor", "Distortion"},
	{"Disintegrator", "Distortion"},
	{"Dyson compressor", "Compressor"},
	{"Echo Delay Line (Maximum Delay 0.01s)", "Delay"},
	{"Echo Delay Line (Maximum Delay 0.1s)", "Delay"},
	{"Echo Delay Line (Maximum Delay 1s)", "Delay"},
	{"Echo Delay Line (Maximum Delay 5s)", "Delay"},
	{"Echo Delay Line (Maximum Delay 60s)", "Delay"},
	{"Exponential signal decay", "Delay"},
	{"Fast Lookahead limiter", "Limiter"},
	{"Fast overdrive", "Distortion"},
	{"Feedback Delay Line (Maximum Delay 0.01s)", "Delay"},
	{"Feedback Delay Line (Maximum Delay 0.1s)", "Delay"},
	{"Feedback Delay Line (Maximum Delay 1s)", "Delay"},
	{"Feedback Delay Line (Maximum Delay 5s)", "Delay"},
	{"Feedback Delay Line (Maximum Delay 60s)", "Delay"},
	{"Flanger", "Flanger"},
	{"Foldover distortion", "Distortion"},
	{"Fractionally Addressed Delay Line", "Delay"},
	{"Freeverb (Version 3)", "Reverb"},
	{"GLAME Butterworth Highpass", "Filter"},
	{"GLAME Butterworth Lowpass", "Filter"},
	{"GSM simulator", "Distortion"},
	{"Gate", "Gate"},
	{"Giant Flange", "Flanger"},
	{"Glame Bandpass Analog Filter", "Filter"},
	{"Glame Bandpass Filter", "Filter"},
	{"Glame Highpass Filter", "Filter"},
	{"Glame Lowpass Filter", "Filter"},
	{"Gong beater", "Other"},
	{"Gong model", "Other"},
	{"Granular Scatter Processor", "Stereo"},
	{"Hard Gate", "Gate"},
	{"Hard Limiter", "Limiter"},
	{"Harmonic generator", "Other"},
	{"Hermes Filter", "Filter"},
	{"High Pass Filter (One Pole)", "Filter"},
	{"Higher Quality Pitch Scaler", "Pitch"},
	{"Identity (Audio)", "Tool"},
	{"Impulse convolver", "Reverb"},
	{"Inverter", "Tool"},
	{"Karaoke", "Tool"},
	{"L/C/R Delay", "Delay"},
	{"LFO Phaser", "Phaser"},
	{"LS Filter", "Filter"},
	{"Lo Fi", "Distortion"},
	{"Low Pass Filter (One Pole)", "Filter"},
	{"Mag's Notch Filter", "Filter"},
	{"Matrix Spatialiser", "Stereo"},
	{"Modulatable delay", "Delay"},
	{"Multiband EQ", "Equalization"},
	{"Multivoice Chorus", "Chorus"},
	{"Pitch Scaler", "Pitch"},
	{"Pointer cast distortion", "Distortion"},
	{"Rate shifter", "Pitch"},
	{"Retro Flanger", "Flanger"},
	{"Reverse Delay (5s max)", "Delay"},
	{"Ringmod with LFO", "LFO"},
	{"Ringmod with two inputs", "LFO"},
	{"SC1", "Compressor"},
	{"SC4", "Compressor"},
	{"SC4 mono", "Compressor"},
	{"SE4", "Stereo"},
	{"Signal sifter", "Distortion"},
	{"Simple Compressor (Peak Envelope Tracking)", "Compressor"},
	{"Simple Compressor (RMS Envelope Tracking)", "Compressor"},
	{"Simple Expander (Peak Envelope Tracking)", "Stereo"},
	{"Simple Expander (RMS Envelope Tracking)", "Stereo"},
	{"Simple Limiter (Peak Envelope Tracking)", "Limiter"},
	{"Simple Limiter (RMS Envelope Tracking)", "Limiter"},
	{"Simple amplifier", "Amplifier"},
	{"Simple delay line, cubic spline interpolation", "Delay"},
	{"Simple delay line, linear interpolation", "Delay"},
	{"Simple delay line, noninterpolating", "Delay"},
	{"Single band parametric", "Filter"},
	{"Sinus wavewrapper", "Distortion"},
	{"Smooth Decimator", "Distortion"},
	{"State Variable Filter", "Filter"},
	{"Stereo Gate", "Gate"},
	{"TAP AutoPanner", "Stereo"},
	{"TAP Chorus/Flanger", "Flanger"},
	{"TAP DeEsser", "Deesser"},
	{"TAP Dynamics (M)", "Distortion"},
	{"TAP Dynamics (St)", "Distortion"},
	{"TAP Equalizer", "Equalization"},
	{"TAP Equalizer/BW", "Equalization"},
	{"TAP Fractal Doubler", "Delay"},
	{"TAP Pink/Fractal Noise", "Noise"},
	{"TAP Pitch Shifter", "Pitch"},
	{"TAP Reflector", "Delay"},
	{"TAP Reverberator", "Delay"},
	{"TAP Rotary Speaker", "Stereo"},
	{"TAP Scaling Limiter", "Limiter"},
	{"TAP Sigmoid Booster", "Equalization"},
	{"TAP Stereo Echo", "Stereo"},
	{"TAP Tremolo", "Pitch"},
	{"TAP TubeWarmth", "Amplifier"},
	{"TAP Vibrato", "Pitch"},
	{"Tape Delay Simulation", "Delay"},
	{"Transient mangler", "Other"},
	{"Triple band parametric with shelves", "Equalization"},
	{"VCF 303", "Filter"},
	{"Valve rectifier", "Distortion"},
	{"Valve saturation", "Distortion"},
	{"Vocoder", "Vocoder"},
	{"VyNil (Vinyl Effect)", "Distortion"},
	{"Wave Shaper (Sine-Based)", "Distortion"},
	{"Wave shaper", "Distortion"},
	{"z-1", "Delay"},
};
const QString defaultCategory = "Other";

std::unique_ptr<EffectCategory> EffectCategory::s_instance;
QList<QString>* m_categories;

EffectCategory* EffectCategory::instance()
{
	if (s_instance == nullptr) { s_instance = std::make_unique<EffectCategory>(); }

	return s_instance.get();
}

EffectCategory* getEffectCategory()
{ return EffectCategory::instance(); }

QString EffectCategory::getCategoryName(QString effectName)
{
	if (lmmsEffects.find(effectName) != lmmsEffects.end()) { return lmmsEffects.at(effectName); }
	if (ladspaEffects.find(effectName) != ladspaEffects.end()) { return ladspaEffects.at(effectName); }
	return defaultCategory;
}

QStringList* EffectCategory::getCategories() 
{
	if(m_categories == nullptr || m_categories->isEmpty()) 
	{
		m_categories = getCategoriesFromMap(lmmsEffects);
		QStringList* ladspaCategories = getCategoriesFromMap(ladspaEffects);
		foreach(QString category, *ladspaCategories) 
		{
			if(! m_categories->contains(category)) 
			{
				m_categories->append(category);
			}
		}
	}
	m_categories->sort();
	return m_categories;
}

QStringList* EffectCategory::getCategoriesFromMap(std::map<QString,QString> map)
{
	auto* categories = new QStringList();
	for(auto & it : map) 
	{
		if(! categories->contains(it.second)) 
		{
			categories->append(it.second);
		}
	}
	return categories;
}

} // namespace lmms
