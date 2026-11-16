/*
 * LadspaSubPluginFeatures.cpp - derivation from
 *                               Plugin::Descriptor::SubPluginFeatures for
 *                               hosting LADSPA-plugins
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "LadspaSubPluginFeatures.h"

#include <QHBoxLayout>
#include <QLabel>

#include "Engine.h"
#include "Ladspa2LMMS.h"
#include "LadspaBase.h"
#include "Plugin.h"
#include "lmms_constants.h"

namespace lmms {

const std::map<QString, Plugin::DefaultCategories> defaultLadspaPlugins =
{
	{"4 x 4 pole allpass", Plugin::DefaultCategories::Filter},
	{"AM pitchshifter", Plugin::DefaultCategories::Pitch},
	{"Aliasing", Plugin::DefaultCategories::Distortion},
	{"Allpass delay line, cubic spline interpolation", Plugin::DefaultCategories::Delay},
	{"Allpass delay line, linear interpolation", Plugin::DefaultCategories::Delay},
	{"Allpass delay line, noninterpolating", Plugin::DefaultCategories::Delay},
	{"Amplifier (Mono)", Plugin::DefaultCategories::Amplifier},
	{"Amplifier (Stereo)", Plugin::DefaultCategories::Amplifier},
	{"Artificial latency", Plugin::DefaultCategories::Other},
	{"Audio Divider (Suboctave Generator)", Plugin::DefaultCategories::Distortion},
	{"Auto phaser", Plugin::DefaultCategories::Phaser},
	{"Barry's Satan Maximiser", Plugin::DefaultCategories::Distortion},
	{"C* AmpIII - Tube amp", Plugin::DefaultCategories::Amplifier},
	{"C* AmpIV - Tube amp + tone controls", Plugin::DefaultCategories::Amplifier},
	{"C* AmpV - Tube amp", Plugin::DefaultCategories::Amplifier},
	{"C* AmpVTS - Tube amp + Tone stack", Plugin::DefaultCategories::Amplifier},
	{"C* AutoFilter - Self-modulating resonant filter", Plugin::DefaultCategories::Filter},
	{"C* AutoWah - Resonant envelope-following filter", Plugin::DefaultCategories::Wah},
	{"C* CabinetI - Loudspeaker cabinet emulation", Plugin::DefaultCategories::Amplifier},
	{"C* CabinetII - Refined loudspeaker cabinet emulation", Plugin::DefaultCategories::Amplifier},
	{"C* CabinetIII - Simplistic loudspeaker cabinet emulation", Plugin::DefaultCategories::Amplifier},
	{"C* CabinetIV - Idealised loudspeaker cabinet", Plugin::DefaultCategories::Amplifier},
	{"C* ChorusI - Mono chorus/flanger", Plugin::DefaultCategories::Chorus},
	{"C* ChorusII - Mono chorus/flanger modulated by a fractal", Plugin::DefaultCategories::Chorus},
	{"C* Clip - Hard clipper, 8x oversampled", Plugin::DefaultCategories::Distortion},
	{"C* Compress - Mono compressor", Plugin::DefaultCategories::Compressor},
	{"C* CompressX2 - Stereo compressor and saturating limiter", Plugin::DefaultCategories::Compressor},
	{"C* Eq - 10-band equalizer", Plugin::DefaultCategories::Equalization},
	{"C* Eq10 - 10-band equaliser", Plugin::DefaultCategories::Equalization},
	{"C* Eq10X2 - Stereo 10-band equaliser", Plugin::DefaultCategories::Equalization},
	{"C* Eq2x2 - stereo 10-band equalizer", Plugin::DefaultCategories::Equalization},
	{"C* Eq4p - 4-band parametric shelving equaliser", Plugin::DefaultCategories::Equalization},
	{"C* EqFA4p - 4-band parametric eq", Plugin::DefaultCategories::Equalization},
	{"C* Narrower - Stereo image width reduction", Plugin::DefaultCategories::Stereo},
	{"C* Noisegate - Attenuating hum and noise", Plugin::DefaultCategories::Gate},
	{"C* PhaserI - Mono phaser", Plugin::DefaultCategories::Phaser},
	{"C* PhaserII - Mono phaser modulated by a Lorenz fractal", Plugin::DefaultCategories::Phaser},
	{"C* Plate2x2 - Versatile plate reverb, stereo inputs", Plugin::DefaultCategories::Reverb},
	{"C* PlateX2 - Versatile plate reverb, stereo inputs", Plugin::DefaultCategories::Reverb},
	{"C* PreampIII - Tube preamp emulation", Plugin::DefaultCategories::Amplifier},
	{"C* PreampIV - Tube preamp emulation + tone controls", Plugin::DefaultCategories::Amplifier},
	{"C* Saturate - Various static nonlinearities, 8x oversampled", Plugin::DefaultCategories::Distortion},
	{"C* Spice - Not an exciter", Plugin::DefaultCategories::Equalization},
	{"C* SpiceX2 - Not an exciter either", Plugin::DefaultCategories::Equalization},
	{"C* SweepVFI - Resonant filter swept by a Lorenz fractal", Plugin::DefaultCategories::Filter},
	{"C* SweepVFII - Resonant filter, f and Q swept by a Lorenz fractal", Plugin::DefaultCategories::Filter},
	{"C* ToneStack - Tone stack emulation", Plugin::DefaultCategories::Filter},
	{"C* ToneStackLT - Tone stack emulation, lattice filter 44.1", Plugin::DefaultCategories::Filter},
	{"Calf Analyzer LADSPA", Plugin::DefaultCategories::Tool},
	{"Calf Bass Enhancer LADSPA", Plugin::DefaultCategories::Equalization},
	{"Calf Compensation Delay Line LADSPA", Plugin::DefaultCategories::Delay},
	{"Calf Compressor LADSPA", Plugin::DefaultCategories::Compressor},
	{"Calf Crusher LADSPA", Plugin::DefaultCategories::Distortion},
	{"Calf Deesser LADSPA", Plugin::DefaultCategories::Deesser},
	{"Calf Emphasis LADSPA", Plugin::DefaultCategories::Filter},
	{"Calf Envelope Filter LADSPA", Plugin::DefaultCategories::Wah},
	{"Calf Equalizer 12 Band LADSPA", Plugin::DefaultCategories::Equalization},
	{"Calf Equalizer 30 Band LADSPA", Plugin::DefaultCategories::Equalization},
	{"Calf Equalizer 5 Band LADSPA", Plugin::DefaultCategories::Equalization},
	{"Calf Equalizer 8 Band LADSPA", Plugin::DefaultCategories::Equalization},
	{"Calf Exciter LADSPA", Plugin::DefaultCategories::Distortion},
	{"Calf Filter LADSPA", Plugin::DefaultCategories::Filter},
	{"Calf Filterclavier LADSPA", Plugin::DefaultCategories::Filter},
	{"Calf Flanger LADSPA", Plugin::DefaultCategories::Flanger},
	{"Calf Gate LADSPA", Plugin::DefaultCategories::Gate},
	{"Calf Haas Stereo Enhancer LADSPA", Plugin::DefaultCategories::Stereo},
	{"Calf Limiter LADSPA", Plugin::DefaultCategories::Limiter},
	{"Calf Mono Compressor LADSPA", Plugin::DefaultCategories::Compressor},
	{"Calf Multi Chorus LADSPA", Plugin::DefaultCategories::Chorus},
	{"Calf Multi Spread LADSPA", Plugin::DefaultCategories::Stereo},
	{"Calf Multiband Compressor LADSPA", Plugin::DefaultCategories::Compressor},
	{"Calf Multiband Enhancer LADSPA", Plugin::DefaultCategories::Distortion},
	{"Calf Multiband Gate LADSPA", Plugin::DefaultCategories::Gate},
	{"Calf Multiband Limiter LADSPA", Plugin::DefaultCategories::Limiter},
	{"Calf Phaser LADSPA", Plugin::DefaultCategories::Phaser},
	{"Calf Psychoachoustic Clipper LADSPA", Plugin::DefaultCategories::Distortion},
	{"Calf Pulsator LADSPA", Plugin::DefaultCategories::Stereo},
	{"Calf Reverb LADSPA", Plugin::DefaultCategories::Reverb},
	{"Calf Reverse Delay LADSPA", Plugin::DefaultCategories::Delay},
	{"Calf Ring Modulator LADSPA", Plugin::DefaultCategories::LowFrequencyOscillator},
	{"Calf Rotary Speaker LADSPA", Plugin::DefaultCategories::Stereo},
	{"Calf Saturator LADSPA", Plugin::DefaultCategories::Distortion},
	{"Calf Sidechain Compressor LADSPA", Plugin::DefaultCategories::Compressor},
	{"Calf Sidechain Gate LADSPA", Plugin::DefaultCategories::Gate},
	{"Calf Sidechain Limiter LADSPA", Plugin::DefaultCategories::Limiter},
	{"Calf Stereo Tools LADSPA", Plugin::DefaultCategories::Stereo},
	{"Calf Tape Simulator LADSPA", Plugin::DefaultCategories::Distortion},
	{"Calf Transient Designer LADSPA", Plugin::DefaultCategories::Equalization},
	{"Calf Vintage Delay LADSPA", Plugin::DefaultCategories::Delay},
	{"Canyon Delay", Plugin::DefaultCategories::Delay},
	{"Chebyshev distortion", Plugin::DefaultCategories::Distortion},
	{"Comb Filter", Plugin::DefaultCategories::Filter},
	{"Comb delay line, cubic spline interpolation", Plugin::DefaultCategories::Delay},
	{"Comb delay line, linear interpolation", Plugin::DefaultCategories::Delay},
	{"Comb delay line, noninterpolating", Plugin::DefaultCategories::Delay},
	{"Constant Signal Generator", Plugin::DefaultCategories::Tool},
	{"Crossover distortion", Plugin::DefaultCategories::Distortion},
	{"DC Offset Remover", Plugin::DefaultCategories::Tool},
	{"DJ EQ", Plugin::DefaultCategories::Equalization},
	{"DJ EQ (mono)", Plugin::DefaultCategories::Equalization},
	{"DJ flanger", Plugin::DefaultCategories::Flanger},
	{"Debug Plugin", Plugin::DefaultCategories::Tool},
	{"Decimator", Plugin::DefaultCategories::Distortion},
	{"Declipper", Plugin::DefaultCategories::Distortion},
	{"Delayorama", Plugin::DefaultCategories::Delay},
	{"Diode Processor", Plugin::DefaultCategories::Distortion},
	{"Disintegrator", Plugin::DefaultCategories::Distortion},
	{"Dyson compressor", Plugin::DefaultCategories::Compressor},
	{"Echo Delay Line (Maximum Delay 0.01s)", Plugin::DefaultCategories::Delay},
	{"Echo Delay Line (Maximum Delay 0.1s)", Plugin::DefaultCategories::Delay},
	{"Echo Delay Line (Maximum Delay 1s)", Plugin::DefaultCategories::Delay},
	{"Echo Delay Line (Maximum Delay 5s)", Plugin::DefaultCategories::Delay},
	{"Echo Delay Line (Maximum Delay 60s)", Plugin::DefaultCategories::Delay},
	{"Exponential signal decay", Plugin::DefaultCategories::Delay},
	{"Fast Lookahead limiter", Plugin::DefaultCategories::Limiter},
	{"Fast overdrive", Plugin::DefaultCategories::Distortion},
	{"Feedback Delay Line (Maximum Delay 0.01s)", Plugin::DefaultCategories::Delay},
	{"Feedback Delay Line (Maximum Delay 0.1s)", Plugin::DefaultCategories::Delay},
	{"Feedback Delay Line (Maximum Delay 1s)", Plugin::DefaultCategories::Delay},
	{"Feedback Delay Line (Maximum Delay 5s)", Plugin::DefaultCategories::Delay},
	{"Feedback Delay Line (Maximum Delay 60s)", Plugin::DefaultCategories::Delay},
	{"Flanger", Plugin::DefaultCategories::Flanger},
	{"Foldover distortion", Plugin::DefaultCategories::Distortion},
	{"Fractionally Addressed Delay Line", Plugin::DefaultCategories::Delay},
	{"Freeverb (Version 3)", Plugin::DefaultCategories::Reverb},
	{"GLAME Butterworth Highpass", Plugin::DefaultCategories::Filter},
	{"GLAME Butterworth Lowpass", Plugin::DefaultCategories::Filter},
	{"GSM simulator", Plugin::DefaultCategories::Distortion},
	{"Gate", Plugin::DefaultCategories::Gate},
	{"Giant Flange", Plugin::DefaultCategories::Flanger},
	{"Glame Bandpass Analog Filter", Plugin::DefaultCategories::Filter},
	{"Glame Bandpass Filter", Plugin::DefaultCategories::Filter},
	{"Glame Highpass Filter", Plugin::DefaultCategories::Filter},
	{"Glame Lowpass Filter", Plugin::DefaultCategories::Filter},
	{"Gong beater", Plugin::DefaultCategories::Other},
	{"Gong model", Plugin::DefaultCategories::Other},
	{"Granular Scatter Processor", Plugin::DefaultCategories::Stereo},
	{"Hard Gate", Plugin::DefaultCategories::Gate},
	{"Hard Limiter", Plugin::DefaultCategories::Limiter},
	{"Harmonic generator", Plugin::DefaultCategories::Other},
	{"Hermes Filter", Plugin::DefaultCategories::Filter},
	{"High Pass Filter (One Pole)", Plugin::DefaultCategories::Filter},
	{"Higher Quality Pitch Scaler", Plugin::DefaultCategories::Pitch},
	{"Identity (Audio)", Plugin::DefaultCategories::Tool},
	{"Impulse convolver", Plugin::DefaultCategories::Reverb},
	{"Inverter", Plugin::DefaultCategories::Tool},
	{"Karaoke", Plugin::DefaultCategories::Tool},
	{"L/C/R Delay", Plugin::DefaultCategories::Delay},
	{"LFO Phaser", Plugin::DefaultCategories::Phaser},
	{"LS Filter", Plugin::DefaultCategories::Filter},
	{"Lo Fi", Plugin::DefaultCategories::Distortion},
	{"Low Pass Filter (One Pole)", Plugin::DefaultCategories::Filter},
	{"Mag's Notch Filter", Plugin::DefaultCategories::Filter},
	{"Matrix Spatialiser", Plugin::DefaultCategories::Stereo},
	{"Modulatable delay", Plugin::DefaultCategories::Delay},
	{"Multiband EQ", Plugin::DefaultCategories::Equalization},
	{"Multivoice Chorus", Plugin::DefaultCategories::Chorus},
	{"Pitch Scaler", Plugin::DefaultCategories::Pitch},
	{"Pointer cast distortion", Plugin::DefaultCategories::Distortion},
	{"Rate shifter", Plugin::DefaultCategories::Pitch},
	{"Retro Flanger", Plugin::DefaultCategories::Flanger},
	{"Reverse Delay (5s max)", Plugin::DefaultCategories::Delay},
	{"Ringmod with LFO", Plugin::DefaultCategories::LowFrequencyOscillator},
	{"Ringmod with two inputs", Plugin::DefaultCategories::LowFrequencyOscillator},
	{"SC1", Plugin::DefaultCategories::Compressor},
	{"SC4", Plugin::DefaultCategories::Compressor},
	{"SC4 mono", Plugin::DefaultCategories::Compressor},
	{"SE4", Plugin::DefaultCategories::Stereo},
	{"Signal sifter", Plugin::DefaultCategories::Distortion},
	{"Simple Compressor (Peak Envelope Tracking)", Plugin::DefaultCategories::Compressor},
	{"Simple Compressor (RMS Envelope Tracking)", Plugin::DefaultCategories::Compressor},
	{"Simple Expander (Peak Envelope Tracking)", Plugin::DefaultCategories::Stereo},
	{"Simple Expander (RMS Envelope Tracking)", Plugin::DefaultCategories::Stereo},
	{"Simple Limiter (Peak Envelope Tracking)", Plugin::DefaultCategories::Limiter},
	{"Simple Limiter (RMS Envelope Tracking)", Plugin::DefaultCategories::Limiter},
	{"Simple amplifier", Plugin::DefaultCategories::Amplifier},
	{"Simple delay line, cubic spline interpolation", Plugin::DefaultCategories::Delay},
	{"Simple delay line, linear interpolation", Plugin::DefaultCategories::Delay},
	{"Simple delay line, noninterpolating", Plugin::DefaultCategories::Delay},
	{"Single band parametric", Plugin::DefaultCategories::Filter},
	{"Sinus wavewrapper", Plugin::DefaultCategories::Distortion},
	{"Smooth Decimator", Plugin::DefaultCategories::Distortion},
	{"State Variable Filter", Plugin::DefaultCategories::Filter},
	{"Stereo Gate", Plugin::DefaultCategories::Gate},
	{"TAP AutoPanner", Plugin::DefaultCategories::Stereo},
	{"TAP Chorus/Flanger", Plugin::DefaultCategories::Flanger},
	{"TAP DeEsser", Plugin::DefaultCategories::Deesser},
	{"TAP Dynamics (M)", Plugin::DefaultCategories::Distortion},
	{"TAP Dynamics (St)", Plugin::DefaultCategories::Distortion},
	{"TAP Equalizer", Plugin::DefaultCategories::Equalization},
	{"TAP Equalizer/BW", Plugin::DefaultCategories::Equalization},
	{"TAP Fractal Doubler", Plugin::DefaultCategories::Delay},
	{"TAP Pink/Fractal Noise", Plugin::DefaultCategories::Noise},
	{"TAP Pitch Shifter", Plugin::DefaultCategories::Pitch},
	{"TAP Reflector", Plugin::DefaultCategories::Delay},
	{"TAP Reverberator", Plugin::DefaultCategories::Delay},
	{"TAP Rotary Speaker", Plugin::DefaultCategories::Stereo},
	{"TAP Scaling Limiter", Plugin::DefaultCategories::Limiter},
	{"TAP Sigmoid Booster", Plugin::DefaultCategories::Equalization},
	{"TAP Stereo Echo", Plugin::DefaultCategories::Stereo},
	{"TAP Tremolo", Plugin::DefaultCategories::Pitch},
	{"TAP TubeWarmth", Plugin::DefaultCategories::Amplifier},
	{"TAP Vibrato", Plugin::DefaultCategories::Pitch},
	{"Tape Delay Simulation", Plugin::DefaultCategories::Delay},
	{"Transient mangler", Plugin::DefaultCategories::Other},
	{"Triple band parametric with shelves", Plugin::DefaultCategories::Equalization},
	{"VCF 303", Plugin::DefaultCategories::Filter},
	{"Valve rectifier", Plugin::DefaultCategories::Distortion},
	{"Valve saturation", Plugin::DefaultCategories::Distortion},
	{"Vocoder", Plugin::DefaultCategories::Vocoder},
	{"VyNil (Vinyl Effect)", Plugin::DefaultCategories::Distortion},
	{"Wave Shaper (Sine-Based)", Plugin::DefaultCategories::Distortion},
	{"Wave shaper", Plugin::DefaultCategories::Distortion},
	{"z-1", Plugin::DefaultCategories::Delay},
};

LadspaSubPluginFeatures::LadspaSubPluginFeatures(Plugin::Type _type)
	: SubPluginFeatures(_type)
{
}

QString LadspaSubPluginFeatures::displayName(const Plugin::Descriptor::SubPluginFeatures::Key& k) const
{
	const ladspa_key_t& lkey = subPluginKeyToLadspaKey(&k);
	Ladspa2LMMS* lm = Engine::getLADSPAManager();
	return lm->getName(lkey);
}

void LadspaSubPluginFeatures::fillDescriptionWidget(QWidget* _parent, const Key* _key) const
{
	const ladspa_key_t& lkey = subPluginKeyToLadspaKey(_key);
	Ladspa2LMMS* lm = Engine::getLADSPAManager();
	const auto ldesc = lm->getDescription(lkey);

	// HACK: Markup inside translation strings due to RTL not being handled correctly.
	// Move the markup out of the translation strings and into the QString template as soon as RTL layout works properly
	auto labelText = QString{
		"<p>%1%2</p>" // Name
		"<p>%3<code>%4</code></p>" // File
		"<p>%5%6</p>" // Author
		"<p>%7%8</p>" // Copyright
		"<p>%9%10</p>" // Channels
	}.arg(
		QWidget::tr("<b>Name: </b>"), lm->getName(lkey),
		QWidget::tr("<b>File: </b>"), lkey.first,
		QWidget::tr("<b>Author: </b>"), lm->getMaker(lkey).replace(" at ", "@").replace(" dot ", ".").toHtmlEscaped(),
		QWidget::tr("<b>Copyright: </b>"), lm->getCopyright(lkey),
		QWidget::tr("<b>Channels: </b>"), QWidget::tr("%1 in, %2 out").arg(ldesc->inputChannels).arg(ldesc->outputChannels)
	);

	if (lm->hasRealTimeDependency(lkey))
	{
		labelText += QString{"<p>%1%2</p>"}.arg(QWidget::tr("<b>Real-time Dependency: </b>"),
			QWidget::tr("This plugin has a real-time dependency (e.g. listens to a MIDI device) so its output must "
						"not be cached or subject to significant latency."));
	}

	if (!lm->isRealTimeCapable(lkey))
	{
		labelText += QString{"<p>%1%2</p>"}.arg(QWidget::tr("<b>Not Real-time Capable: </b>"),
			QWidget::tr("This plugin is not suitable for use in a &lsquo;hard real-time&rsquo; environment."));
	}

	auto label = new QLabel(labelText, _parent);
	label->setWordWrap(true);
}

void LadspaSubPluginFeatures::listSubPluginKeys(const Plugin::Descriptor* _desc, KeyList& _kl) const
{
	Ladspa2LMMS* lm = Engine::getLADSPAManager();

	l_sortable_plugin_t plugins;
	switch (m_type)
	{
	case Plugin::Type::Instrument:
		plugins = lm->getInstruments();
		break;
	case Plugin::Type::Effect:
		plugins = lm->getValidEffects();
		// plugins += lm->getInvalidEffects();
		break;
	case Plugin::Type::Tool:
		plugins = lm->getAnalysisTools();
		break;
	case Plugin::Type::Other:
		plugins = lm->getOthers();
		break;
	default:
		break;
	}

	for (l_sortable_plugin_t::const_iterator it = plugins.begin(); it != plugins.end(); ++it)
	{
		if (lm->getDescription((*it).second)->inputChannels <= DEFAULT_CHANNELS)
		{
			_kl.push_back(ladspaKeyToSubPluginKey(_desc, (*it).first, (*it).second));
		}
	}
}

Plugin::DefaultCategories getDefaultEffectCategory(const Plugin::Descriptor* descriptor)
{
	if (defaultLadspaPlugins.find(descriptor->name) != defaultLadspaPlugins.end())
	{
		return defaultLadspaPlugins.at(descriptor->name);
	}
	return Plugin::DefaultCategories::Other;
}

ladspa_key_t LadspaSubPluginFeatures::subPluginKeyToLadspaKey(const Key* _key)
{
	QString file = _key->attributes["file"];
	return (ladspa_key_t(file.remove(QRegularExpression("\\.so$")).remove(QRegularExpression("\\.dll$")) +
#if defined(LMMS_BUILD_WIN32) || defined(LMMS_BUILD_CYGWIN)
			".dll"
#else
			".so"
#endif
		,
		_key->attributes["plugin"]));
}

} // namespace lmms
