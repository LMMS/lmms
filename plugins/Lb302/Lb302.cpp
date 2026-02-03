/*
 * Lb302.cpp - implementation of class Lb302 which is a bass synth attempting
 *             to emulate the Roland TB-303 bass synth
 *
 * Copyright (c) 2006-2008 Paul Giblock <pgib/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * Lb302FilterIIR2 is based on the gsyn filter code by Andy Sloane.
 *
 * Lb302Filter3Pole is based on the TB-303 instrument written by
 *   Josep M Comajuncosas for the CSounds library
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

#include "Lb302.h"

#include <cmath>
#include <numbers>

#include <QDebug>

#include "AutomatableButton.h"
#include "BandLimitedWave.h"
#include "DspEffectLibrary.h"
#include "Engine.h"
#include "embed.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "PixmapButton.h"
#include "plugin_export.h"

#define LB_24_IGNORE_ENVELOPE
//#define LB_24_RES_TRICK


namespace
{
// Helper to get the phase increment per sample, given a note's frequency and the current sample rate
static inline float phaseInc(float freq) { return freq / lmms::Engine::audioEngine()->outputSampleRate(); }
}


namespace lmms
{

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT lb302_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"LB302",
	QT_TRANSLATE_NOOP("PluginBrowser", "Incomplete monophonic imitation TB-303"),
	"Paul Giblock <pgib/at/users.sf.net>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
};

PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* m, void*)
{
	return new Lb302Synth(static_cast<InstrumentTrack*>(m));
}

} // extern "C"

//
// Lb302Filter
//

void Lb302Filter::recalc()
{
	vcf_e1 = std::exp(6.109f + 1.5876f * fs->envmod + 2.1553f * fs->cutoff - 1.2f * (1.0f - fs->reso));
	vcf_e0 = std::exp(5.613f - 0.8f * fs->envmod + 2.1553f * fs->cutoff - 0.7696f * (1.0f - fs->reso));
	const float pi_sr = std::numbers::pi_v<float> / Engine::audioEngine()->outputSampleRate();
	vcf_e0 *= pi_sr;
	vcf_e1 *= pi_sr;
	vcf_e1 -= vcf_e0;

	vcf_rescoeff = std::exp(-1.20f + 3.455f * fs->reso);
};


void Lb302Filter::envRecalc()
{
	vcf_c0 *= fs->envdecay; // Filter Decay. vcf_decay is adjusted for Hz and s_envInc
	// vcf_rescoeff = std::exp(-1.20f + 3.455f * fs->reso); moved above to Lb302Filter::recalc()
};


void Lb302Filter::playNote() { vcf_c0 = vcf_e1; }


//
// Lb302FilterIIR2
//

Lb302FilterIIR2::Lb302FilterIIR2(Lb302FilterKnobState* p_fs)
	: Lb302Filter(p_fs)
	, m_dist{std::make_unique<DspEffectLibrary::Distortion>(1.f, 1.f)}
{};


void Lb302FilterIIR2::recalc()
{
	Lb302Filter::recalc();
	//m_dist->setThreshold(0.5f + (fs->dist * 2.f));
	m_dist->setThreshold(fs->dist * 75.f);
};


void Lb302FilterIIR2::envRecalc()
{
	Lb302Filter::envRecalc();
	const float w = vcf_e0 + vcf_c0; // e0 is adjusted for Hz and doesn't need s_envInc
	const float k = std::exp(-w / vcf_rescoeff); // Does this mean c0 is inheritantly?

	vcf_a = 2.f * std::cos(2.f * w) * k;
	vcf_b = -k * k;
	vcf_c = 1.f - vcf_a - vcf_b;
}


sample_t Lb302FilterIIR2::process(const sample_t& samp)
{
	sample_t ret = vcf_a * vcf_d1 + vcf_b * vcf_d2 + vcf_c * samp;
	// Delayed samples for filter
	vcf_d2 = vcf_d1;
	vcf_d1 = ret;

	if (fs->dist > 0.f) { ret = m_dist->nextSample(ret); }

	// output = IIR2 + dry
	return ret;
}


//
// Lb302Filter3Pole
//


void Lb302Filter3Pole::recalc()
{
	// DO NOT CALL BASE CLASS
	vcf_e0 = 0.000001f;
	vcf_e1 = 1.f;
}


// TODO: Try using k instead of vcf_reso
void Lb302Filter3Pole::envRecalc()
{
	Lb302Filter::envRecalc();

	// e0 is adjusted for Hz and doesn't need s_envInc
	float w = vcf_e0 + vcf_c0;
	float k = std::min(fs->cutoff, 0.975f);
	// sampleRateCutoff should not be changed to anything dynamic that is outside the
	// scope of LB302 (like e.g. the audio engine's sample rate) as this changes the filter's cutoff
	// behavior without any modification to its controls.
	constexpr float sampleRateCutoff = 44100.0f;
	float kfco = 50.f + k * (
		(2300.f - 1600.f * fs->envmod)
		+ w * (700.f + 1500.f * k + (1500.f + k * (sampleRateCutoff / 2.f - 6000.f)) * fs->envmod)
	); // + iacc * (0.3f + 0.7f * kfco * kenvmod) * kaccent * kaccurve * 2000.f

#ifdef LB_24_IGNORE_ENVELOPE
	// kfcn = fs->cutoff;
	kfcn = 2.f * kfco / Engine::audioEngine()->outputSampleRate();
#else
	kfcn = w;
#endif
	kp   = ((-2.7528f * kfcn + 3.0429f) * kfcn + 1.718f) * kfcn - 0.9984f;
	kp1  = kp + 1.f;
	kp1h = 0.5f * kp1;
#ifdef LB_24_RES_TRICK
	k = std::exp(-w / vcf_rescoeff);
	kres = k * (((-2.7079f * kp1 + 10.963f) * kp1 - 14.934f) * kp1 + 8.4974f);
#else
	kres = fs->reso * (((-2.7079f * kp1 + 10.963f) * kp1 - 14.934f) * kp1 + 8.4974f);
#endif
	value = 1.f + (fs->dist * (1.5f + 2.f * kres * (1.f - kfcn))); // ENVMOD was DIST
}


sample_t Lb302Filter3Pole::process(const sample_t& samp)
{
	float ax1  = lastin;
	float ay11 = ay1;
	float ay31 = ay2;
	lastin  = samp - std::tanh(kres * aout);
	ay1     = kp1h * (lastin + ax1) - kp * ay1;
	ay2     = kp1h * (ay1 + ay11) - kp * ay2;
	aout    = kp1h * (ay2 + ay31) - kp * aout;

	return std::tanh(aout * value) * s_volAdjust / (1.f + fs->dist);
}


//
// LBSynth
//

Lb302Synth::Lb302Synth(InstrumentTrack* instrumentTrack)
	: Instrument(instrumentTrack, &lb302_plugin_descriptor, nullptr, Flag::IsSingleStreamed)
	, vcf_cut_knob(0.75f, 0.0f, 1.5f, 0.005f, this, tr("VCF Cutoff Frequency"))
	, vcf_res_knob(0.75f, 0.0f, 1.25f, 0.005f, this, tr("VCF Resonance"))
	, vcf_mod_knob(0.1f, 0.0f, 1.0f, 0.005f, this, tr("VCF Envelope Mod"))
	, vcf_dec_knob(0.1f, 0.0f, 1.0f, 0.005f, this, tr("VCF Envelope Decay"))
	, dist_knob(0.0f, 0.0f, 1.0f, 0.01f, this, tr("Distortion"))
	, wave_shape(8.0f, 0.0f, 11.0f, this, tr("Waveform"))
	, slide_dec_knob(0.6f, 0.0f, 1.0f, 0.005f, this, tr("Slide Decay"))
	, slideToggle(false, this, tr("Slide"))
	, accentToggle(false, this, tr("Accent"))
	, deadToggle(false, this, tr("Dead"))
	, db24Toggle(false, this, tr("24dB/oct Filter"))
	, vcfs{std::make_unique<Lb302FilterIIR2>(&fs), std::make_unique<Lb302Filter3Pole>(&fs)}
{
	connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, this, &Lb302Synth::filterChanged);
	connect(&vcf_cut_knob, &FloatModel::dataChanged, this, &Lb302Synth::filterChanged);
	connect(&vcf_res_knob, &FloatModel::dataChanged, this, &Lb302Synth::filterChanged);
	connect(&vcf_mod_knob, &FloatModel::dataChanged, this, &Lb302Synth::filterChanged);
	connect(&vcf_dec_knob, &FloatModel::dataChanged, this, &Lb302Synth::filterChanged);
	connect(&db24Toggle, &BoolModel::dataChanged, this, &Lb302Synth::db24Toggled);
	connect(&dist_knob, &FloatModel::dataChanged, this, &Lb302Synth::filterChanged);

	// db24Toggled(); // TODO: Remove? This just calls recalcFilter(), which also happens during filterChanged()
	filterChanged();

	Engine::audioEngine()->addPlayHandle(new InstrumentPlayHandle(this, instrumentTrack));
}


void Lb302Synth::saveSettings(QDomDocument& doc, QDomElement& el)
{
	vcf_cut_knob.saveSettings(doc, el, "vcf_cut");
	vcf_res_knob.saveSettings(doc, el, "vcf_res");
	vcf_mod_knob.saveSettings(doc, el, "vcf_mod");
	vcf_dec_knob.saveSettings(doc, el, "vcf_dec");

	wave_shape.saveSettings(doc, el, "shape");
	dist_knob.saveSettings(doc, el, "dist");
	slide_dec_knob.saveSettings(doc, el, "slide_dec");

	slideToggle.saveSettings(doc, el, "slide");
	deadToggle.saveSettings(doc, el, "dead");
	db24Toggle.saveSettings(doc, el, "db24");
}


void Lb302Synth::loadSettings(const QDomElement& el)
{
	vcf_cut_knob.loadSettings(el, "vcf_cut");
	vcf_res_knob.loadSettings(el, "vcf_res");
	vcf_mod_knob.loadSettings(el, "vcf_mod");
	vcf_dec_knob.loadSettings(el, "vcf_dec");

	dist_knob.loadSettings(el, "dist");
	slide_dec_knob.loadSettings(el, "slide_dec");
	wave_shape.loadSettings(el, "shape");
	slideToggle.loadSettings(el, "slide");
	deadToggle.loadSettings(el, "dead");
	db24Toggle.loadSettings(el, "db24");

 	db24Toggled();
	filterChanged();
}

// TODO: Split into one function per knob.  envdecay doesn't require
// recalcFilter.
void Lb302Synth::filterChanged()
{
	fs.cutoff = vcf_cut_knob.value();
	fs.reso   = vcf_res_knob.value();
	fs.envmod = vcf_mod_knob.value();
	fs.dist   = dist_knob.value() * s_distRatio;

	float d = 0.2f + (2.3f * vcf_dec_knob.value());

	d *= Engine::audioEngine()->outputSampleRate(); // d *= smpl rate
	fs.envdecay = std::pow(0.1f, 1.0f / d * s_envInc); // vcf_envdecay is now adjusted for both sampling rate and s_envInc
	recalcFilter();
}


void Lb302Synth::db24Toggled() { recalcFilter(); } // These recalcFilter calls might suck


QString Lb302Synth::nodeName() const { return lb302_plugin_descriptor.name; }


// OBSOLETE. Break apart once we get Q_OBJECT to work. >:[
void Lb302Synth::recalcFilter()
{
	vcf().recalc();

	// THIS IS OLD 3pole/24dB code, I may reintegrate it.  Don't need it
	// right now.   Should be toggled by LB_24_RES_TRICK at the moment.

	/*kfcn = 2.f * (vcf_cutoff * 3000) / engine::audioEngine()->outputSampleRate();
	kp   = ((-2.7528f * kfcn + 3.0429f) * kfcn + 1.718f) * kfcn - 0.9984f;
	kp1  = kp + 1.f;
	kp1h = 0.5f * kp1;
	kres = vcf_reso * (((-2.7079f * kp1 + 10.963f) * kp1 - 14.934f) * kp1 + 8.4974f);
	value = 1.f + (0.f * (1.5f + 2.f * kres * (1.f - kfcn))); // ENVMOD was DIST*/

	vcf_envpos = s_envInc; // Trigger filter update in process()
}


void Lb302Synth::process(SampleFrame* outbuf, const fpp_t size)
{
	const float sampleRatio = 44100.f / Engine::audioEngine()->outputSampleRate();
	Lb302Filter& filter = vcf(); // Hold on to the current VCF, and use it throughout this period

	if (release_frame.load(std::memory_order_relaxed) == 0 || !m_playingNote) { vca_mode = VcaMode::Decay; }
	if (new_freq)
	{
		new_freq = false;
		const bool noteIsDead = deadToggle.value();
		// catch_decay = 0;
		vco_inc = phaseInc(true_freq);

		// Always reset vca on non-dead notes, and only reset vca on decaying (decayed) and never-played
		if (!noteIsDead || (vca_mode == VcaMode::Decay || vca_mode == VcaMode::NeverPlayed))
		{
			sample_cnt = 0;
			vca_mode = VcaMode::Attack;
		}
		else { vca_mode = VcaMode::Idle; }

		if (vco_slideinc != 0.f)
		{
			// Initiate Slide
			vco_slide = vco_inc - vco_slideinc; // Slide amount
			vco_slidebase = vco_inc; // The REAL frequency
		}
		else { vco_slide = 0.f; }

		// Slide-from note, save inc for next note
		// May need to equal vco_slidebase + vco_slide if last note slid
		if (slideToggle.value()) { vco_slideinc = vco_inc; }

		recalcFilter();
		if (!noteIsDead)
		{
			// Swap next two blocks??
			vcf().playNote();
			vcf_envpos = s_envInc; // Ensure envelope is recalculated
		}
	}

	// TODO: NORMAL RELEASE
	// vca_mode = VcaMode::Decay;

	// Note: this has to be computed during processing and cannot be initialized
	// in the constructor because it's dependent on the sample rate and that might
	// change during rendering!
	//
	// At 44.1 kHz this will compute something very close to the previously
	// hard coded value of 0.99897516.
	constexpr auto computeDecayFactor = [](float decayTimeInSeconds, float targetedAttenuation) -> float
	{
		// This is the number of samples that correspond to the decay time in seconds
		auto samplesNeededForDecay = decayTimeInSeconds * Engine::audioEngine()->outputSampleRate();

		// This computes the factor that's needed to make a signal with a value of 1 decay to the
		// targeted attenuation over the time in number of samples.
		return std::pow(targetedAttenuation, 1.f / samplesNeededForDecay);
	};
	constexpr auto gateThreshold = 1.f / 65536.f; // Signal below this value is silenced
	const auto decay = computeDecayFactor(0.245260770975f, gateThreshold);

	for (f_cnt_t i = 0; i < size; ++i)
	{
		// start decay if we're past release
		if (i >= release_frame.load(std::memory_order_relaxed)) { vca_mode = VcaMode::Decay; }

		// update vcf
		if (vcf_envpos >= s_envInc)
		{
			filter.envRecalc();
			vcf_envpos = 0;

			if (vco_slide)
			{
				vco_inc = vco_slidebase - vco_slide;
				// Calculate coeff from dec_knob on knob change.
				// TODO: Adjust for s_envInc
				vco_slide -= vco_slide * (0.1f - slide_dec_knob.value() * 0.0999f) * sampleRatio;
			}
		}

		sample_cnt++;
		vcf_envpos++;

		// f_cnt_t decay_frames = 128;

		// update vco
		vco_c += vco_inc;
		if (vco_c > 0.5f) { vco_c -= 1.f; }
		vco_shape = static_cast<VcoShape>(wave_shape.value());

		// add vco_shape_param the changes the shape of each curve.
		// merge sawtooths with triangle and square with round square?
		switch (vco_shape)
		{
			// p0: curviness of line
			// Is this sawtooth backwards?
			case VcoShape::Sawtooth: vco_k = vco_c; break;

			// p0: duty rev.saw<->triangle<->saw
			// p1: curviness
			case VcoShape::Triangle:
				vco_k = vco_c * 2.f + 0.5f;
				if (vco_k > 0.5f) { vco_k = 1.f - vco_k; }
				break;

			// p0: slope of top
			case VcoShape::Square:
				vco_k = vco_c < 0.f ? 0.5f : -0.5f;
				break;

			// p0: width of round
			case VcoShape::RoundSquare:
				vco_k = vco_c < 0.f ? std::sqrt(1.f - (vco_c * vco_c * 4.f)) - 0.5f : -0.5f;
				break;

			// Maybe the fall should be exponential/sinsoidal instead of quadric.
			// [-0.5, 0]: Rise, [0,0.25]: Slope down, [0.25,0.5]: Low
			case VcoShape::Moog:
				vco_k = vco_c * 2.f + 0.5f;
				if (vco_k > 1.f) { vco_k = -0.5f; }
				else if (vco_k > 0.5f)
				{
					float w = 2.f * (vco_k - 0.5f) - 1.f;
					vco_k = 0.5f - std::sqrt(1.f - (w * w));
				}
				vco_k *= 2.f; // MOOG wave gets filtered away
				break;

			// [-0.5, 0.5] : [-pi, pi]
			case VcoShape::Sine: vco_k = 0.5f * Oscillator::sinSample(vco_c); break;
			case VcoShape::Exponential: vco_k = 0.5f * Oscillator::expSample(vco_c); break;
			case VcoShape::WhiteNoise: vco_k = 0.5f * Oscillator::noiseSample(vco_c); break;

			// The next cases all use the BandLimitedWave class which uses the oscillator increment `vco_inc` to compute samples.
			// If that oscillator increment is 0 we return a 0 sample because calling BandLimitedWave::pdToLen(0) leads to a
			// division by 0 which in turn leads to floating point exceptions.
			case VcoShape::BLSawtooth:
				vco_k = vco_inc == 0.f ? 0.f : BandLimitedWave::oscillate(vco_c + 0.5f, BandLimitedWave::pdToLen(vco_inc), BandLimitedWave::Waveform::BLSaw) * 0.5f;
				break;

			case VcoShape::BLSquare:
				vco_k = vco_inc == 0.f ? 0.f : BandLimitedWave::oscillate(vco_c + 0.5f, BandLimitedWave::pdToLen(vco_inc), BandLimitedWave::Waveform::BLSquare) * 0.5f;
				break;

			case VcoShape::BLTriangle:
				vco_k = vco_inc == 0.f ? 0.f : BandLimitedWave::oscillate(vco_c + 0.5f, BandLimitedWave::pdToLen(vco_inc), BandLimitedWave::Waveform::BLTriangle) * 0.5f;
				break;

			case VcoShape::BLMoog:
				vco_k = vco_inc == 0.f ? 0.f : BandLimitedWave::oscillate(vco_c + 0.5f, BandLimitedWave::pdToLen(vco_inc), BandLimitedWave::Waveform::BLMoog);
				break;
		}

		// vca_a = 0.5f;

		// Write out samples.
		//samp = vcf->process(vco_k) * 2.f * vca_a;
		//samp = vcf->process(vco_k) * 2.f;
		sample_t samp = filter.process(vco_k) * vca_a;

		//samp = vco_k * vca_a;
		// if (sample_cnt <= 4) { vca_a = 0.f; }

		// float releaseFrames = desiredReleaseFrames();
		// samp *= (releaseFrames - catch_decay) / releaseFrames;
		// samp *= static_cast<float>(decay_frames - catch_decay) / static_cast<float>(decay_frames); // LB302

		for (ch_cnt_t c = 0; c < DEFAULT_CHANNELS; c++) { outbuf[i][c] = samp; }

		// Handle Envelope
		if (vca_mode == VcaMode::Attack)
		{
			vca_a += (s_vcaA0 - vca_a) * s_vcaAttack;
			if (sample_cnt >= 0.5f * Engine::audioEngine()->outputSampleRate()) { vca_mode = VcaMode::Idle; }
		}
		else if (vca_mode == VcaMode::Decay)
		{
			vca_a *= decay;

			// the following line actually speeds up processing
			if (vca_a < gateThreshold)
			{
				vca_a = 0;
				vca_mode = VcaMode::NeverPlayed;
			}
		}
	}
}


void Lb302Synth::initSlide()
{
	if (vco_slideinc == 0.f)
	{
		vco_slide = 0.f;
		return;
	}

	// Initiate Slide
	vco_slide = vco_inc - vco_slideinc; // Slide amount
	vco_slidebase = vco_inc; // The REAL frequency
	vco_slideinc = 0.f; // reset from-note
}


void Lb302Synth::playNote(NotePlayHandle* nph, SampleFrame*)
{
	if (nph->isMasterNote() || (nph->hasParent() && nph->isReleased())) { return; }

	auto tries = s_maxNoteEnqueueRetries;
	auto write_claimed_expected = m_notesWriteClaimed.load(std::memory_order_relaxed);
	size_t index, next_index;
	do
	{
		retry_send_note:
		if (!tries--)
		{
			qDebug() << "Lb302: Note dropped due to catastrophically poor performance! This should never happen!";
			return;
		}
		const ptrdiff_t occupied = write_claimed_expected - m_notesReadSeq.load(std::memory_order_acquire);
		assert(occupied >= 0);
		// If full, wait for room
		if (static_cast<size_t>(occupied) >= s_maxPendingNotes)
		{
			busy_wait_hint();
			// goto rather than continue since we do not want to evaluate the compare_exchange_strong().
			// However, the CAS is what normally updates write_claimed_expected, so load it manually
			write_claimed_expected = m_notesWriteClaimed.load(std::memory_order_relaxed);
			goto retry_send_note;
		}
		index = write_claimed_expected; next_index = write_claimed_expected + 1;
	}
	while (!m_notesWriteClaimed.compare_exchange_strong(write_claimed_expected, next_index, std::memory_order_acquire));

	m_notes[index & NotesBufMask] = nph;
	release_frame.store(
		std::max(release_frame.load(std::memory_order_acquire), nph->framesLeft() + nph->offset()),
		std::memory_order_release
	);

	size_t write_committed_expected = index;
	while (!m_notesWriteCommitted.compare_exchange_strong(write_committed_expected, next_index, std::memory_order_release))
	{
		write_committed_expected = index; // Reset this as the CAS will have changed it
		busy_wait_hint();
	}
}



void Lb302Synth::processNote(NotePlayHandle* nph)
{
	/// Start a new note.
	if (nph->m_pluginData != this)
	{
		m_playingNote = nph;
		new_freq = true;
		nph->m_pluginData = this;
	}
	
	if (!m_playingNote && !nph->isReleased() && release_frame.load(std::memory_order_relaxed) > 0)
	{
		m_playingNote = nph;
		if (slideToggle.value()) { vco_slideinc = phaseInc(nph->frequency()); }
	}

	// Check for slide
	if (m_playingNote == nph)
	{
		true_freq = nph->frequency();
		const auto true_inc = phaseInc(true_freq);
		if (slideToggle.value()) { vco_slidebase = true_inc; } else { vco_inc = true_inc; }
	}
}



void Lb302Synth::play(SampleFrame* working_buffer)
{
	const auto readIdx = m_notesReadSeq.load(std::memory_order_relaxed);
	const auto writeCommitted = m_notesWriteCommitted.load(std::memory_order_acquire);
	// Process notes, but process new notes last
	for (size_t i = readIdx; i < writeCommitted; ++i)
	{
		const auto& nph = m_notes[i & NotesBufMask];
		if (nph->totalFramesPlayed() == 0) { continue; }
		processNote(nph);
	}
	for (size_t i = readIdx; i < writeCommitted; ++i)
	{
		const auto& nph = m_notes[i & NotesBufMask];
		if (nph->totalFramesPlayed() != 0) { continue; }
		processNote(nph);
	}
	// Mark the processed notes as having been read so that playNote() calls can overwrite them
	m_notesReadSeq.fetch_add(writeCommitted - readIdx, std::memory_order_release);

	process(working_buffer, Engine::audioEngine()->framesPerPeriod());
}



void Lb302Synth::deleteNotePluginData(NotePlayHandle* nph)
{
	if (m_playingNote == nph) { m_playingNote = nullptr; }
}


gui::PluginView * Lb302Synth::instantiateView(QWidget* parent)
{
	return new gui::Lb302SynthView(this, parent);
}

namespace gui
{


Lb302SynthView::Lb302SynthView(Instrument* instrument, QWidget* parent)
	: InstrumentViewFixedSize(instrument, parent)
{
	setAutoFillBackground(true);
	static auto s_artwork = PLUGIN_NAME::getIconPixmap("artwork");
	QPalette pal;
	pal.setBrush(backgroundRole(), s_artwork);
	setPalette(pal);

	// GUI
	m_vcfCutKnob = new Knob(KnobType::Bright26, this);
	m_vcfCutKnob->move(75, 130);
	m_vcfCutKnob->setHintText(tr("Cutoff Freq:"), "");

	m_vcfResKnob = new Knob(KnobType::Bright26, this);
	m_vcfResKnob->move(120, 130);
	m_vcfResKnob->setHintText(tr("Resonance:"), "");

	m_vcfModKnob = new Knob(KnobType::Bright26, this);
	m_vcfModKnob->move(165, 130);
	m_vcfModKnob->setHintText(tr("Env Mod:"), "");

	m_vcfDecKnob = new Knob(KnobType::Bright26, this);
	m_vcfDecKnob->move(210, 130);
	m_vcfDecKnob->setHintText(tr("Decay:"), "");

	m_slideToggle = new LedCheckBox("", this);
	m_slideToggle->move(10, 180);

	// accent removed pending real implementation - no need for non-functional buttons
	/* m_accentToggle = new LedCheckBox("", this);
	   m_accentToggle->move(10, 200); */

	m_deadToggle = new LedCheckBox("", this);
	m_deadToggle->move(10, 200);

	m_db24Toggle = new LedCheckBox("", this);
	m_db24Toggle->move(10, 150);
	m_db24Toggle->setToolTip(tr("303-es-que, 24dB/octave, 3 pole filter"));

	m_slideDecKnob = new Knob(KnobType::Bright26, this);
	m_slideDecKnob->move(210, 75);
	m_slideDecKnob->setHintText(tr("Slide Decay:"), "");

	m_distKnob = new Knob(KnobType::Bright26, this);
	m_distKnob->move(210, 190);
	m_distKnob->setHintText(tr("DIST:"), "");

	// Shapes
	// move to 120,75
	const int waveBtnX = 10;
	const int waveBtnY = 96;
	m_waveBtnGrp = new AutomatableButtonGroup(this);

	auto sawWaveBtn = new PixmapButton(this, tr("Saw wave"));
	sawWaveBtn->move(waveBtnX, waveBtnY);
	sawWaveBtn->setActiveGraphic(embed::getIconPixmap("saw_wave_active"));
	sawWaveBtn->setInactiveGraphic(embed::getIconPixmap("saw_wave_inactive"));
	sawWaveBtn->setToolTip(tr("Click here for a saw-wave."));
	m_waveBtnGrp->addButton(sawWaveBtn);

	auto triangleWaveBtn = new PixmapButton(this, tr("Triangle wave"));
	triangleWaveBtn->move(waveBtnX + (16 * 1), waveBtnY);
	triangleWaveBtn->setActiveGraphic(embed::getIconPixmap("triangle_wave_active"));
	triangleWaveBtn->setInactiveGraphic(embed::getIconPixmap("triangle_wave_inactive"));
	triangleWaveBtn->setToolTip(tr("Click here for a triangle-wave."));
	m_waveBtnGrp->addButton(triangleWaveBtn);

	auto sqrWaveBtn = new PixmapButton(this, tr("Square wave"));
	sqrWaveBtn->move(waveBtnX + (16 * 2), waveBtnY);
	sqrWaveBtn->setActiveGraphic(embed::getIconPixmap("square_wave_active"));
	sqrWaveBtn->setInactiveGraphic(embed::getIconPixmap("square_wave_inactive"));
	sqrWaveBtn->setToolTip(tr("Click here for a square-wave."));
	m_waveBtnGrp->addButton(sqrWaveBtn);

	auto roundSqrWaveBtn = new PixmapButton(this, tr("Rounded square wave"));
	roundSqrWaveBtn->move(waveBtnX + (16 * 3), waveBtnY);
	roundSqrWaveBtn->setActiveGraphic( embed::getIconPixmap("round_square_wave_active"));
	roundSqrWaveBtn->setInactiveGraphic( embed::getIconPixmap("round_square_wave_inactive"));
	roundSqrWaveBtn->setToolTip(tr("Click here for a square-wave with a rounded end."));
	m_waveBtnGrp->addButton(roundSqrWaveBtn);

	auto moogWaveBtn = new PixmapButton(this, tr("Moog wave"));
	moogWaveBtn->move(waveBtnX + (16 * 4), waveBtnY);
	moogWaveBtn->setActiveGraphic(embed::getIconPixmap("moog_saw_wave_active"));
	moogWaveBtn->setInactiveGraphic(embed::getIconPixmap("moog_saw_wave_inactive"));
	moogWaveBtn->setToolTip(tr("Click here for a moog-like wave."));
	m_waveBtnGrp->addButton(moogWaveBtn);

	auto sinWaveBtn = new PixmapButton(this, tr("Sine wave"));
	sinWaveBtn->move(waveBtnX + (16 * 5), waveBtnY);
	sinWaveBtn->setActiveGraphic(embed::getIconPixmap("sin_wave_active"));
	sinWaveBtn->setInactiveGraphic(embed::getIconPixmap("sin_wave_inactive"));
	sinWaveBtn->setToolTip(tr("Click for a sine-wave."));
	m_waveBtnGrp->addButton(sinWaveBtn);

	auto exponentialWaveBtn = new PixmapButton(this, tr("White noise wave"));
	exponentialWaveBtn->move(waveBtnX + (16 * 6), waveBtnY);
	exponentialWaveBtn->setActiveGraphic(embed::getIconPixmap("exp_wave_active"));
	exponentialWaveBtn->setInactiveGraphic(embed::getIconPixmap("exp_wave_inactive"));
	exponentialWaveBtn->setToolTip(tr("Click here for an exponential wave."));
	m_waveBtnGrp->addButton(exponentialWaveBtn);

	auto whiteNoiseWaveBtn = new PixmapButton(this, tr("White noise wave"));
	whiteNoiseWaveBtn->move(waveBtnX + (16 * 7), waveBtnY);
	whiteNoiseWaveBtn->setActiveGraphic(embed::getIconPixmap("white_noise_wave_active"));
	whiteNoiseWaveBtn->setInactiveGraphic(embed::getIconPixmap("white_noise_wave_inactive"));
	whiteNoiseWaveBtn->setToolTip(tr("Click here for white-noise."));
	m_waveBtnGrp->addButton(whiteNoiseWaveBtn);

	auto blSawWaveBtn = new PixmapButton(this, tr("Bandlimited saw wave"));
	blSawWaveBtn->move(waveBtnX + (16 * 9) - 8, waveBtnY);
	blSawWaveBtn->setActiveGraphic(embed::getIconPixmap("saw_wave_active"));
	blSawWaveBtn->setInactiveGraphic(embed::getIconPixmap("saw_wave_inactive"));
	blSawWaveBtn->setToolTip(tr("Click here for bandlimited saw wave."));
	m_waveBtnGrp->addButton(blSawWaveBtn);

	auto blSquareWaveBtn = new PixmapButton(this, tr("Bandlimited square wave"));
	blSquareWaveBtn->move(waveBtnX + (16 * 10) - 8, waveBtnY);
	blSquareWaveBtn->setActiveGraphic(embed::getIconPixmap("square_wave_active"));
	blSquareWaveBtn->setInactiveGraphic(embed::getIconPixmap("square_wave_inactive"));
	blSquareWaveBtn->setToolTip(tr("Click here for bandlimited square wave."));
	m_waveBtnGrp->addButton(blSquareWaveBtn);

	auto blTriangleWaveBtn = new PixmapButton(this, tr("Bandlimited triangle wave"));
	blTriangleWaveBtn->move(waveBtnX + (16 * 11) - 8, waveBtnY);
	blTriangleWaveBtn->setActiveGraphic(embed::getIconPixmap("triangle_wave_active"));
	blTriangleWaveBtn->setInactiveGraphic(embed::getIconPixmap("triangle_wave_inactive"));
	blTriangleWaveBtn->setToolTip(tr("Click here for bandlimited triangle wave."));
	m_waveBtnGrp->addButton(blTriangleWaveBtn);

	auto blMoogWaveBtn = new PixmapButton(this, tr("Bandlimited moog saw wave"));
	blMoogWaveBtn->move(waveBtnX + (16 * 12) - 8, waveBtnY);
	blMoogWaveBtn->setActiveGraphic(embed::getIconPixmap("moog_saw_wave_active"));
	blMoogWaveBtn->setInactiveGraphic(embed::getIconPixmap("moog_saw_wave_inactive"));
	blMoogWaveBtn->setToolTip(tr("Click here for bandlimited moog saw wave."));
	m_waveBtnGrp->addButton(blMoogWaveBtn);
}


void Lb302SynthView::modelChanged()
{
	auto syn = castModel<Lb302Synth>();

	m_vcfCutKnob->setModel(&syn->vcf_cut_knob);
	m_vcfResKnob->setModel(&syn->vcf_res_knob);
	m_vcfDecKnob->setModel(&syn->vcf_dec_knob);
	m_vcfModKnob->setModel(&syn->vcf_mod_knob);
	m_slideDecKnob->setModel(&syn->slide_dec_knob);

	m_distKnob->setModel(&syn->dist_knob);
	m_waveBtnGrp->setModel(&syn->wave_shape);

	m_slideToggle->setModel(&syn->slideToggle);
	// m_accentToggle->setModel(&syn->accentToggle);
	m_deadToggle->setModel(&syn->deadToggle);
	m_db24Toggle->setModel(&syn->db24Toggle);
}


} // namespace gui

} // namespace lmms
