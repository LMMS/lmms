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

#include <QList>
#include <QObject>

namespace lmms {

const std::map<QString, QString> lmmsEffects = 
{
	{"Amplifier", QObject::tr("Amplifier")},
	{"BassBooster", QObject::tr("Equalization")},
	{"Bitcrush", QObject::tr("Bitcrush")},
	{"Compressor", QObject::tr("Compressor")},
	{"Crossover Equalizer", QObject::tr("Equalization")},
	{"Delay", QObject::tr("Delay")},
	{"Dispersion", QObject::tr("Filter")},
	{"Dual Filter", QObject::tr("Filter")},
	{"Dynamics Processor", QObject::tr("Distortion")},
	{"Equalizer", QObject::tr("Equalization")},
	{"Flanger", QObject::tr("Flanger")},
	{"Frequency Shifter", QObject::tr("Pitch")},
	{"Granular Pitch Shifter", QObject::tr("Pitch")},
	{"LOMM", QObject::tr("Distortion")},
	{"Multitap Echo", QObject::tr("Delay")},
	{"Oscilloscope", QObject::tr("Tool")},
	{"Peak Controller", QObject::tr("Automation")},
	{"ReverbSC", QObject::tr("Reverb")},
	{"Slew Distortion", QObject::tr("Distortion")},
	{"Spectrum Analyzer", QObject::tr("Tool")},
	{"Stereo Matrix", QObject::tr("Stereo")},
	{"StereoEnhancer Effect", QObject::tr("Stereo")},
	{"Vectorscope", QObject::tr("Tool")},
	{"Waveshaper Effect", QObject::tr("Distortion")}
};

const std::map<QString, QString> ladspaEffects = 
{
	{"4 x 4 pole allpass", QObject::tr("Filter")},
	{"AM pitchshifter", QObject::tr("Pitch")},
	{"Aliasing", QObject::tr("Distortion")},
	{"Allpass delay line, cubic spline interpolation", QObject::tr("Delay")},
	{"Allpass delay line, linear interpolation", QObject::tr("Delay")},
	{"Allpass delay line, noninterpolating", QObject::tr("Delay")},
	{"Amplifier (Mono)", QObject::tr("Amplifier")},
	{"Amplifier (Stereo)", QObject::tr("Amplifier")},
	{"Artificial latency", QObject::tr("Other")},
	{"Audio Divider (Suboctave Generator)", QObject::tr("Distortion")},
	{"Auto phaser", QObject::tr("Phaser")},
	{"Barry's Satan Maximiser", QObject::tr("Distortion")},
	{"C* AmpIII - Tube amp", QObject::tr("Amplifier")},
	{"C* AmpIV - Tube amp + tone controls", QObject::tr("Amplifier")},
	{"C* AmpV - Tube amp", QObject::tr("Amplifier")},
	{"C* AmpVTS - Tube amp + Tone stack", QObject::tr("Amplifier")},
	{"C* AutoFilter - Self-modulating resonant filter", QObject::tr("Filter")},
	{"C* AutoWah - Resonant envelope-following filter", QObject::tr("Wah")},
	{"C* CabinetI - Loudspeaker cabinet emulation", QObject::tr("Amplifier")},
	{"C* CabinetII - Refined loudspeaker cabinet emulation", QObject::tr("Amplifier")},
	{"C* CabinetIII - Simplistic loudspeaker cabinet emulation", QObject::tr("Amplifier")},
	{"C* CabinetIV - Idealised loudspeaker cabinet", QObject::tr("Amplifier")},
	{"C* ChorusI - Mono chorus/flanger", QObject::tr("Chorus")},
	{"C* ChorusII - Mono chorus/flanger modulated by a fractal", QObject::tr("Chorus")},
	{"C* Clip - Hard clipper, 8x oversampled", QObject::tr("Distortion")},
	{"C* Compress - Mono compressor", QObject::tr("Compressor")},
	{"C* CompressX2 - Stereo compressor and saturating limiter", QObject::tr("Compressor")},
	{"C* Eq - 10-band equalizer", QObject::tr("Equalization")},
	{"C* Eq10 - 10-band equaliser", QObject::tr("Equalization")},
	{"C* Eq10X2 - Stereo 10-band equaliser", QObject::tr("Equalization")},
	{"C* Eq2x2 - stereo 10-band equalizer", QObject::tr("Equalization")},
	{"C* Eq4p - 4-band parametric shelving equaliser", QObject::tr("Equalization")},
	{"C* EqFA4p - 4-band parametric eq", QObject::tr("Equalization")},
	{"C* Narrower - Stereo image width reduction", QObject::tr("Stereo")},
	{"C* Noisegate - Attenuating hum and noise", QObject::tr("Gate")},
	{"C* PhaserI - Mono phaser", QObject::tr("Phaser")},
	{"C* PhaserII - Mono phaser modulated by a Lorenz fractal", QObject::tr("Phaser")},
	{"C* Plate2x2 - Versatile plate reverb, stereo inputs", QObject::tr("Reverb")},
	{"C* PlateX2 - Versatile plate reverb, stereo inputs", QObject::tr("Reverb")},
	{"C* PreampIII - Tube preamp emulation", QObject::tr("Amplifier")},
	{"C* PreampIV - Tube preamp emulation + tone controls", QObject::tr("Amplifier")},
	{"C* Saturate - Various static nonlinearities, 8x oversampled", QObject::tr("Distortion")},
	{"C* Spice - Not an exciter", QObject::tr("Equalization")},
	{"C* SpiceX2 - Not an exciter either", QObject::tr("Equalization")},
	{"C* SweepVFI - Resonant filter swept by a Lorenz fractal", QObject::tr("Filter")},
	{"C* SweepVFII - Resonant filter, f and Q swept by a Lorenz fractal", QObject::tr("Filter")},
	{"C* ToneStack - Tone stack emulation", QObject::tr("Filter")},
	{"C* ToneStackLT - Tone stack emulation, lattice filter 44.1", QObject::tr("Filter")},
	{"Calf Analyzer LADSPA", QObject::tr("Tool")},
	{"Calf Bass Enhancer LADSPA", QObject::tr("Equalization")},
	{"Calf Compensation Delay Line LADSPA", QObject::tr("Delay")},
	{"Calf Compressor LADSPA", QObject::tr("Compressor")},
	{"Calf Crusher LADSPA", QObject::tr("Distortion")},
	{"Calf Deesser LADSPA", QObject::tr("Deesser")},
	{"Calf Emphasis LADSPA", QObject::tr("Filter")},
	{"Calf Envelope Filter LADSPA", QObject::tr("Wah")},
	{"Calf Equalizer 12 Band LADSPA", QObject::tr("Equalization")},
	{"Calf Equalizer 30 Band LADSPA", QObject::tr("Equalization")},
	{"Calf Equalizer 5 Band LADSPA", QObject::tr("Equalization")},
	{"Calf Equalizer 8 Band LADSPA", QObject::tr("Equalization")},
	{"Calf Exciter LADSPA", QObject::tr("Distortion")},
	{"Calf Filter LADSPA", QObject::tr("Filter")},
	{"Calf Filterclavier LADSPA", QObject::tr("Filter")},
	{"Calf Flanger LADSPA", QObject::tr("Flanger")},
	{"Calf Gate LADSPA", QObject::tr("Gate")},
	{"Calf Haas Stereo Enhancer LADSPA", QObject::tr("Stereo")},
	{"Calf Limiter LADSPA", QObject::tr("Limiter")},
	{"Calf Mono Compressor LADSPA", QObject::tr("Compressor")},
	{"Calf Multi Chorus LADSPA", QObject::tr("Chorus")},
	{"Calf Multi Spread LADSPA", QObject::tr("Stereo")},
	{"Calf Multiband Compressor LADSPA", QObject::tr("Compressor")},
	{"Calf Multiband Enhancer LADSPA", QObject::tr("Distortion")},
	{"Calf Multiband Gate LADSPA", QObject::tr("Gate")},
	{"Calf Multiband Limiter LADSPA", QObject::tr("Limiter")},
	{"Calf Phaser LADSPA", QObject::tr("Phaser")},
	{"Calf Psychoachoustic Clipper LADSPA", QObject::tr("Distortion")},
	{"Calf Pulsator LADSPA", QObject::tr("Stereo")},
	{"Calf Reverb LADSPA", QObject::tr("Reverb")},
	{"Calf Reverse Delay LADSPA", QObject::tr("Delay")},
	{"Calf Ring Modulator LADSPA", QObject::tr("LFO")},
	{"Calf Rotary Speaker LADSPA", QObject::tr("Stereo")},
	{"Calf Saturator LADSPA", QObject::tr("Distortion")},
	{"Calf Sidechain Compressor LADSPA", QObject::tr("Compressor")},
	{"Calf Sidechain Gate LADSPA", QObject::tr("Gate")},
	{"Calf Sidechain Limiter LADSPA", QObject::tr("Limiter")},
	{"Calf Stereo Tools LADSPA", QObject::tr("Stereo")},
	{"Calf Tape Simulator LADSPA", QObject::tr("Distortion")},
	{"Calf Transient Designer LADSPA", QObject::tr("Equalization")},
	{"Calf Vintage Delay LADSPA", QObject::tr("Delay")},
	{"Canyon Delay", QObject::tr("Delay")},
	{"Chebyshev distortion", QObject::tr("Distortion")},
	{"Comb Filter", QObject::tr("Filter")},
	{"Comb delay line, cubic spline interpolation", QObject::tr("Delay")},
	{"Comb delay line, linear interpolation", QObject::tr("Delay")},
	{"Comb delay line, noninterpolating", QObject::tr("Delay")},
	{"Constant Signal Generator", QObject::tr("Tool")},
	{"Crossover distortion", QObject::tr("Distortion")},
	{"DC Offset Remover", QObject::tr("Tool")},
	{"DJ EQ", QObject::tr("Equalization")},
	{"DJ EQ (mono)", QObject::tr("Equalization")},
	{"DJ flanger", QObject::tr("Flanger")},
	{"Debug Plugin", QObject::tr("Tool")},
	{"Decimator", QObject::tr("Distortion")},
	{"Declipper", QObject::tr("Distortion")},
	{"Delayorama", QObject::tr("Delay")},
	{"Diode Processor", QObject::tr("Distortion")},
	{"Disintegrator", QObject::tr("Distortion")},
	{"Dyson compressor", QObject::tr("Compressor")},
	{"Echo Delay Line (Maximum Delay 0.01s)", QObject::tr("Delay")},
	{"Echo Delay Line (Maximum Delay 0.1s)", QObject::tr("Delay")},
	{"Echo Delay Line (Maximum Delay 1s)", QObject::tr("Delay")},
	{"Echo Delay Line (Maximum Delay 5s)", QObject::tr("Delay")},
	{"Echo Delay Line (Maximum Delay 60s)", QObject::tr("Delay")},
	{"Exponential signal decay", QObject::tr("Delay")},
	{"Fast Lookahead limiter", QObject::tr("Limiter")},
	{"Fast overdrive", QObject::tr("Distortion")},
	{"Feedback Delay Line (Maximum Delay 0.01s)", QObject::tr("Delay")},
	{"Feedback Delay Line (Maximum Delay 0.1s)", QObject::tr("Delay")},
	{"Feedback Delay Line (Maximum Delay 1s)", QObject::tr("Delay")},
	{"Feedback Delay Line (Maximum Delay 5s)", QObject::tr("Delay")},
	{"Feedback Delay Line (Maximum Delay 60s)", QObject::tr("Delay")},
	{"Flanger", QObject::tr("Flanger")},
	{"Foldover distortion", QObject::tr("Distortion")},
	{"Fractionally Addressed Delay Line", QObject::tr("Delay")},
	{"Freeverb (Version 3)", QObject::tr("Reverb")},
	{"GLAME Butterworth Highpass", QObject::tr("Filter")},
	{"GLAME Butterworth Lowpass", QObject::tr("Filter")},
	{"GSM simulator", QObject::tr("Distortion")},
	{"Gate", QObject::tr("Gate")},
	{"Giant Flange", QObject::tr("Flanger")},
	{"Glame Bandpass Analog Filter", QObject::tr("Filter")},
	{"Glame Bandpass Filter", QObject::tr("Filter")},
	{"Glame Highpass Filter", QObject::tr("Filter")},
	{"Glame Lowpass Filter", QObject::tr("Filter")},
	{"Gong beater", QObject::tr("Other")},
	{"Gong model", QObject::tr("Other")},
	{"Granular Scatter Processor", QObject::tr("Stereo")},
	{"Hard Gate", QObject::tr("Gate")},
	{"Hard Limiter", QObject::tr("Limiter")},
	{"Harmonic generator", QObject::tr("Other")},
	{"Hermes Filter", QObject::tr("Filter")},
	{"High Pass Filter (One Pole)", QObject::tr("Filter")},
	{"Higher Quality Pitch Scaler", QObject::tr("Pitch")},
	{"Identity (Audio)", QObject::tr("Tool")},
	{"Impulse convolver", QObject::tr("Reverb")},
	{"Inverter", QObject::tr("Tool")},
	{"Karaoke", QObject::tr("Tool")},
	{"L/C/R Delay", QObject::tr("Delay")},
	{"LFO Phaser", QObject::tr("Phaser")},
	{"LS Filter", QObject::tr("Filter")},
	{"Lo Fi", QObject::tr("Distortion")},
	{"Low Pass Filter (One Pole)", QObject::tr("Filter")},
	{"Mag's Notch Filter", QObject::tr("Filter")},
	{"Matrix Spatialiser", QObject::tr("Stereo")},
	{"Modulatable delay", QObject::tr("Delay")},
	{"Multiband EQ", QObject::tr("Equalization")},
	{"Multivoice Chorus", QObject::tr("Chorus")},
	{"Pitch Scaler", QObject::tr("Pitch")},
	{"Pointer cast distortion", QObject::tr("Distortion")},
	{"Rate shifter", QObject::tr("Pitch")},
	{"Retro Flanger", QObject::tr("Flanger")},
	{"Reverse Delay (5s max)", QObject::tr("Delay")},
	{"Ringmod with LFO", QObject::tr("LFO")},
	{"Ringmod with two inputs", QObject::tr("LFO")},
	{"SC1", QObject::tr("Compressor")},
	{"SC4", QObject::tr("Compressor")},
	{"SC4 mono", QObject::tr("Compressor")},
	{"SE4", QObject::tr("Stereo")},
	{"Signal sifter", QObject::tr("Distortion")},
	{"Simple Compressor (Peak Envelope Tracking)", QObject::tr("Compressor")},
	{"Simple Compressor (RMS Envelope Tracking)", QObject::tr("Compressor")},
	{"Simple Expander (Peak Envelope Tracking)", QObject::tr("Stereo")},
	{"Simple Expander (RMS Envelope Tracking)", QObject::tr("Stereo")},
	{"Simple Limiter (Peak Envelope Tracking)", QObject::tr("Limiter")},
	{"Simple Limiter (RMS Envelope Tracking)", QObject::tr("Limiter")},
	{"Simple amplifier", QObject::tr("Amplifier")},
	{"Simple delay line, cubic spline interpolation", QObject::tr("Delay")},
	{"Simple delay line, linear interpolation", QObject::tr("Delay")},
	{"Simple delay line, noninterpolating", QObject::tr("Delay")},
	{"Single band parametric", QObject::tr("Filter")},
	{"Sinus wavewrapper", QObject::tr("Distortion")},
	{"Smooth Decimator", QObject::tr("Distortion")},
	{"State Variable Filter", QObject::tr("Filter")},
	{"Stereo Gate", QObject::tr("Gate")},
	{"TAP AutoPanner", QObject::tr("Stereo")},
	{"TAP Chorus/Flanger", QObject::tr("Flanger")},
	{"TAP DeEsser", QObject::tr("Deesser")},
	{"TAP Dynamics (M)", QObject::tr("Distortion")},
	{"TAP Dynamics (St)", QObject::tr("Distortion")},
	{"TAP Equalizer", QObject::tr("Equalization")},
	{"TAP Equalizer/BW", QObject::tr("Equalization")},
	{"TAP Fractal Doubler", QObject::tr("Delay")},
	{"TAP Pink/Fractal Noise", QObject::tr("Noise")},
	{"TAP Pitch Shifter", QObject::tr("Pitch")},
	{"TAP Reflector", QObject::tr("Delay")},
	{"TAP Reverberator", QObject::tr("Delay")},
	{"TAP Rotary Speaker", QObject::tr("Stereo")},
	{"TAP Scaling Limiter", QObject::tr("Limiter")},
	{"TAP Sigmoid Booster", QObject::tr("Equalization")},
	{"TAP Stereo Echo", QObject::tr("Stereo")},
	{"TAP Tremolo", QObject::tr("Pitch")},
	{"TAP TubeWarmth", QObject::tr("Amplifier")},
	{"TAP Vibrato", QObject::tr("Pitch")},
	{"Tape Delay Simulation", QObject::tr("Delay")},
	{"Transient mangler", QObject::tr("Other")},
	{"Triple band parametric with shelves", QObject::tr("Equalization")},
	{"VCF 303", QObject::tr("Filter")},
	{"Valve rectifier", QObject::tr("Distortion")},
	{"Valve saturation", QObject::tr("Distortion")},
	{"Vocoder", QObject::tr("Vocoder")},
	{"VyNil (Vinyl Effect)", QObject::tr("Distortion")},
	{"Wave Shaper (Sine-Based)", QObject::tr("Distortion")},
	{"Wave shaper", QObject::tr("Distortion")},
	{"z-1", QObject::tr("Delay")},
};
const QString defaultCategory = "Other";

std::unique_ptr<EffectCategory> EffectCategory::s_instance;
QStringList m_categories;

EffectCategory* EffectCategory::instance()
{
	if (s_instance == nullptr) 
	{
		s_instance = std::make_unique<EffectCategory>(); 
	}
	return s_instance.get();
}

EffectCategory* getEffectCategory()
{
	return EffectCategory::instance(); 
}

QString EffectCategory::getCategoryName(QString effectName)
{
	if (lmmsEffects.find(effectName) != lmmsEffects.end()) { return lmmsEffects.at(effectName); }
	if (ladspaEffects.find(effectName) != ladspaEffects.end()) { return ladspaEffects.at(effectName); }
	return defaultCategory;
}

QStringList EffectCategory::getCategories() 
{
	if (m_categories.isEmpty()) 
	{
		m_categories = getCategoriesFromMap(lmmsEffects);
		QStringList ladspaCategories = getCategoriesFromMap(ladspaEffects);
		for (const QString& category : ladspaCategories) 
		{
			if (!m_categories.contains(category)) 
			{
				m_categories.append(category);
			}
		}
	}
	m_categories.sort();
	return m_categories;
}

QStringList EffectCategory::getCategoriesFromMap(std::map<QString, QString> map)
{
	auto* categories = new QStringList();
	for (auto& it : map) 
	{
		if (!categories->contains(it.second)) 
		{
			categories->append(it.second);
		}
	}
	return *categories;
}

} // namespace lmms
