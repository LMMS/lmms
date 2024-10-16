#include "SynchroSynth.h"

#include <QDomElement>
#include <cmath>

#include "AudioEngine.h"
#include "Engine.h"
#include "Plugin.h"
#include "InstrumentTrack.h"
#include "lmms_basics.h"
#include "plugin_export.h"

namespace lmms
{

extern "C"
{
	Plugin::Descriptor PLUGIN_EXPORT synchro_plugin_descriptor =
	{
		LMMS_STRINGIFY(PLUGIN_NAME),
		"Synchro",
		QT_TRANSLATE_NOOP("PluginBrowser", "2-oscillator PM synth"),
		"Fawn <rubiefawn/at/gmail/dot/com>",
		0x0100, // plugin version, why hexadecimal?
		Plugin::Type::Instrument,
		new PluginPixmapLoader("logo"),
		nullptr, nullptr
	};

	PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *m, void *) { return new Synchro(static_cast<InstrumentTrack*>(m)); }
}

// static inline float reducePhase(float phase) { return fmod(lmms::F_2PI + fmod(phase, lmms::F_2PI), lmms::F_2PI); }
static inline float reducePhase(float phase) { return fmod(1.f + fmod(phase, 1.f), 1.f); }

// Expects phase in the range of 0..1, preferably offset by 0.25
// If the phase is is radians just multiply it by 1 / τ
static inline float tri(float phase) { return 4 * abs(phase - floor(phase + .5f)) - 1; }

// This is a naive but readable version of the waveform function.
// phase is expected to be in the range 0..1.
[[maybe_unused]]
static float ezsauce(float phase, float drive, float sync, float pulse)
{
	const float trianglewave = tri(phase * sync + .25f); // offset so waveform starts at 0
	const float saturated = tanh(drive * trianglewave) / tanh(drive);
	const float attenuation = pow(1 - phase, pulse); // attenuation towards the end of the waveform
	return saturated * attenuation;
}

//TODO actually document the parameters
// This is a simplified but tremendously less-readable version of the function.
// Faster approximation functions in place of std::exp() and std::pow() may yield better performance.
// `phase` is expected to be in the range 0..1
static float sauce(float phase, float drive, float sync, float pulse)
{
	const float a = exp(tri(phase * sync + .25f) * drive * 2);
	const float b = exp(drive * 2);
	const float c = pow(1 - phase, pulse);
	return ((a - 1) * (b + 1)) / ((a + 1) * (b - 1)) * c;
}

//TODO Document the arbitrary Magic Numbers
static float sauce(float phase, float drive, float sync, float pulse, float harmonics)
{
	const float synced = phase * sync + .25f;
	const float t = tri(synced);
	const float h = tri(synced * 32) * .5f + tri(synced * 38) * .03f;
	const float a = exp((t + h * harmonics) * drive * 2);
	const float b = exp(drive * 2);
	const float c = pow(1 - phase, pulse);
	return ((a - 1) * (b + 1)) / ((a + 1) * (b - 1)) * c;
}

Synchro::Synchro(InstrumentTrack *track) :
	Instrument(track, &synchro_plugin_descriptor),
	m_carrier(this, "carrier"),
	m_modulator(this, "modulator"),
	m_modAmt(0.f, 0.f, 1.f, .00001f, this, tr("modulation amount")),
	m_modScale(1.f, -2.f, 2.f, .25f, this, tr("modulation scale")),
	m_harmonics(0.f, 0.f, 1.f, 0.00001f, this, tr("harmonics")),
	m_octaveRatio(-1, -4, 0, 1, this, tr("octave ratio")),
	m_oversampling(2),
	m_carrierWaveform(-1.f, 1.f, SYNCHRO_GRAPH_RESOLUTION, this),
	m_modulatorWaveform(-1.f, 1.f, SYNCHRO_GRAPH_RESOLUTION, this),
	m_resultingWaveform(-1.f, 1.f, SYNCHRO_GRAPH_RESOLUTION, this)
{
	m_modulator.drive.setInitValue(2.f);
	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(effectiveSampleRateChanged()));
	//TODO connect oversampling slot once it has UI controls
	// connect(&m_oversampling, SIGNAL(dataChanged()), this, SLOT(effectiveSampleRateChanged()));
	connect(&m_carrier.drive, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_carrier.sync, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_carrier.pulse, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_modulator.drive, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modulator.sync, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modulator.pulse, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modAmt, SIGNAL(dataChanged()), this, SLOT(eitherOscChanged()));
	connect(&m_octaveRatio, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_harmonics, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modScale, SIGNAL(dataChanged()), this, SLOT(eitherOscChanged()));

	carrierChanged();
	modulatorChanged();
	effectiveSampleRateChanged();	
}

gui::PluginView *Synchro::instantiateView(QWidget *parent) { return new gui::SynchroView(this, parent); }

void Synchro::playNote(NotePlayHandle *nph, SampleFrame *buf)
{
	if (!nph->m_pluginData) { nph->m_pluginData = new std::array<float, 2>; }
	
	std::array<float, 2> *phases = static_cast<std::array<float, 2>*>(nph->m_pluginData);
	const fpp_t len = nph->framesLeftForCurrentPeriod();
	const f_cnt_t offset = nph->noteOffset();
	const sample_rate_t internalSampleRate = Engine::audioEngine()->outputSampleRate() * m_oversampling;
	const float phasePerSample = nph->frequency() / internalSampleRate;
	//TODO Experiment with exp2 approximation function (2^x), stdlib compromises speed for accuracy
	const float pitchDiff = exp2(m_octaveRatio.value());

	//FIXME there's currently a bug where the modulator will reset its own phase every time the carrier completes a
	// cycle. The shape of the modulator in each of those windows is correct. If the modulator is -1 octave, it should
	// cycle once per two carrier cycles, but instead it just plays the first half of its cycle twice.
	// help
	for (f_cnt_t i = 0; i < len * m_oversampling; ++i)
	{
		auto frame = offset + (i / m_oversampling);
		(*phases)[0] = reducePhase((*phases)[0] + phasePerSample);
		(*phases)[1] = reducePhase((*phases)[1] + phasePerSample * pitchDiff);
		const float modulation = sauce(
			(*phases)[1],
			m_modulator.driveExact(frame),
			m_modulator.syncExact(frame),
			m_modulator.pulseExact(frame),
			harmonicsExact(frame)
		);
		//TODO The current modulation method is to apply the modulation as an
		// additive offset to the phase when generating the waveform, and does not
		// affect the "true" phase of the carrier. Try applying the modulation
		// as a multiplicative offset to `carrierPhaseInc` and see if that sounds
		// and/or behaves any better.
		const float phase = reducePhase((*phases)[0] + modulation * modAmtExact(frame) * m_modScale.value());
		m_buf[0][i] = sauce(phase, m_carrier.driveExact(frame), m_carrier.syncExact(frame), m_carrier.pulseExact(frame));
	}

	auto w = 0; // double buffer index, should only ever be 0 or 1
	for (fpp_t len2 = len * m_oversampling >> 1; len2 >= len; len2 >>= 1)
	{
		for (f_cnt_t i = 0, j = 0; i < len2; ++i, j = i << 1)
		{
			//FIXME use hiir downsampling
			m_buf[w^1][i] = (m_buf[w][j] + m_buf[w][1+j]) / 2.f;
		}
		w ^= 1;
	}

	// I would just memcpy twice but unfortunately buf is one array of two-channel samples rather than
	// two arrays of single-channel samples. There's probably a clever way to work around this during downsampling but
	// I'm not that clever
	for (f_cnt_t f = 0; f < len; ++f)	{	buf[f+offset] = SampleFrame(m_buf[w][f]); }
	instrumentTrack()->processAudioBuffer(buf, offset + len, nph);
}

void Synchro::deleteNotePluginData(NotePlayHandle *nph) { delete static_cast<std::array<float, 2>*>(nph->m_pluginData); };

QString Synchro::nodeName() const { return synchro_plugin_descriptor.displayName; }

void Synchro::saveSettings(QDomDocument &doc, QDomElement &parent)
{
	parent.setAttribute("version", synchro_plugin_descriptor.version);
	m_modScale.saveSettings(doc, parent, "modulation scale");
	m_harmonics.saveSettings(doc, parent, "harmonics");
	m_octaveRatio.saveSettings(doc, parent, "octave ratio");

	m_carrier.drive.saveSettings(doc, parent, "carrier drive");
	m_carrier.sync.saveSettings(doc, parent, "carrier sync");
	m_carrier.pulse.saveSettings(doc, parent, "carrier pulse");

	m_modulator.drive.saveSettings(doc, parent, "modulator drive");
	m_modulator.sync.saveSettings(doc, parent, "modulator sync");
	m_modulator.pulse.saveSettings(doc, parent, "modulator pulse");
}

void Synchro::loadSettings(const QDomElement &thisElement)
{
	//TODO Check if preset was made with an older version, handle if necesary
	m_modScale.loadSettings(thisElement, "modulation scale");
	m_harmonics.loadSettings(thisElement, "harmonics");
	m_octaveRatio.loadSettings(thisElement, "octave ratio");

	m_carrier.drive.loadSettings(thisElement, "carrier drive");
	m_carrier.sync.loadSettings(thisElement, "carrier sync");
	m_carrier.pulse.loadSettings(thisElement, "carrier pulse");

	m_modulator.drive.loadSettings(thisElement, "modulator drive");
	m_modulator.sync.loadSettings(thisElement, "modulator sync");
	m_modulator.pulse.loadSettings(thisElement, "modulator pulse");
}

void Synchro::effectiveSampleRateChanged()
{
	//TODO Set up HIIR downsampling filter
	m_buf[0].resize(Engine::audioEngine()->framesPerPeriod() * m_oversampling);
	m_buf[1].resize(Engine::audioEngine()->framesPerPeriod() * m_oversampling);
}

void Synchro::carrierChanged()
{
	const float pitchDiff = exp2(-m_octaveRatio.value());
	for (auto i = 0; i < SYNCHRO_GRAPH_RESOLUTION; ++i)
	{
		const float phase = (float)i / SYNCHRO_GRAPH_RESOLUTION;
		const float sample = sauce(
			reducePhase(phase * pitchDiff),
			m_carrier.drive.value(),
			m_carrier.sync.value(),
			m_carrier.pulse.value()
		);
		m_carrierWaveform.setSampleAt(i, sample);
	}
	eitherOscChanged();
}

void Synchro::modulatorChanged()
{
	for (auto i = 0; i < SYNCHRO_GRAPH_RESOLUTION; ++i)
	{
		const float sample = sauce(
			(float)i / SYNCHRO_GRAPH_RESOLUTION,
			m_modulator.drive.value(),
			m_modulator.sync.value(),
			m_modulator.pulse.value(),
			m_harmonics.value() * 0.15
		);
		m_modulatorWaveform.setSampleAt(i, sample);
	}
	eitherOscChanged();
}

//TODO add oversampling for the graphs lol they get so screwed up
void Synchro::eitherOscChanged()
{
	const float pitchDiff = exp2(-m_octaveRatio.value());
	for (auto i = 0; i < SYNCHRO_GRAPH_RESOLUTION; ++i)
	{
		const float phase = (float)i / SYNCHRO_GRAPH_RESOLUTION;
		const float modAmt = sauce(phase, m_modulator.drive.value(), m_modulator.sync.value(), m_modulator.pulse.value(), m_harmonics.value() * 0.15f);
		const float carrierPhase = reducePhase(phase * pitchDiff + modAmt * m_modAmt.value() * m_modScale.value());
		m_resultingWaveform.setSampleAt(i, sauce(carrierPhase, m_carrier.drive.value(), m_carrier.sync.value(), m_carrier.pulse.value()));
	}
}

gui::SynchroView::SynchroView(Instrument *instrument, QWidget *parent) :
	InstrumentViewFixedSize(instrument, parent)
{
	setAutoFillBackground(true);
	QPalette pal;
	//TODO use svg background once svg support is complete
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);

	constexpr int graph_w = SYNCHRO_GRAPH_RESOLUTION, graph_h = 77, graph_x = 18;
	#define SYNCHRO_GRAPH_INIT(IT) do {\
	IT->setAutoFillBackground(false);\
	IT->setEnabled(false); } while (0)

	m_carrierWaveform = new Graph(this, Graph::Style::LinearNonCyclic, graph_w, graph_h);
	m_carrierWaveform->setGraphColor(SYNCHRO_CYAN);
	m_carrierWaveform->move(graph_x, 165);
	SYNCHRO_GRAPH_INIT(m_carrierWaveform);

	m_modulatorWaveform = new Graph(this, Graph::Style::LinearNonCyclic, graph_w, graph_h);
	m_modulatorWaveform->setGraphColor(SYNCHRO_RED);
	m_modulatorWaveform->move(graph_x, 262);
	SYNCHRO_GRAPH_INIT(m_modulatorWaveform);

	m_resultingWaveform = new Graph(this, Graph::Style::LinearNonCyclic, graph_w, graph_h);
	m_resultingWaveform->setGraphColor(SYNCHRO_YELLOW);
	m_resultingWaveform->move(graph_x, 68);
	SYNCHRO_GRAPH_INIT(m_resultingWaveform);

	constexpr int knob_xy = -3; //HACK get rid of this shit
	constexpr int knob_x[] = { 220, 285, 350, 416 };
	constexpr int knob_y[] = { 86, 183, 280 };

	//TODO custom styled knobs that use the colors of their corresponding UI section
	m_modAmt = new Knob(KnobType::Dark28, this);
	m_modAmt->move(knob_x[0] + knob_xy, knob_y[0] + knob_xy);
	m_modAmt->setHintText(tr("modulation"), "×"); //TODO make the UI show 0-100%

	m_modScale = new Knob(KnobType::Dark28, this);
	m_modScale->move(knob_x[1] + knob_xy, knob_y[0] + knob_xy);
	m_modScale->setHintText(tr("modulation scale"), "×"); //TODO make the UI show 0-100%

	m_harmonics = new Knob(KnobType::Dark28, this);
	m_harmonics->move(knob_x[3] + knob_xy, knob_y[2] + knob_xy);
	m_harmonics->setHintText(tr("harmonics"), "×"); //TODO make the UI show 0-100%

	m_octaveRatio = new Knob(KnobType::Dark28, this);
	m_octaveRatio->move(knob_x[3] + knob_xy, knob_y[1] + knob_xy);
	m_octaveRatio->setHintText(tr("octave ratio"), "octaves");

	m_carrierDrive = new Knob(KnobType::Dark28, this);
	m_carrierDrive->move(knob_x[0] + knob_xy, knob_y[1] + knob_xy);
	m_carrierDrive->setHintText(tr("carrier drive"), "×");

	m_carrierSync = new Knob(KnobType::Dark28, this);
	m_carrierSync->move(knob_x[1] + knob_xy, knob_y[1] + knob_xy);
	m_carrierSync->setHintText(tr("carrier sync"), "×");

	m_carrierPulse = new Knob(KnobType::Dark28, this);
	m_carrierPulse->move(knob_x[2] + knob_xy, knob_y[1] + knob_xy);
	m_carrierPulse->setHintText(tr("carrier pulse"), "^");

	m_modulatorDrive = new Knob(KnobType::Dark28, this);
	m_modulatorDrive->move(knob_x[0] + knob_xy, knob_y[2] + knob_xy);
	m_modulatorDrive->setHintText(tr("modulator drive"), "×");

	m_modulatorSync = new Knob(KnobType::Dark28, this);
	m_modulatorSync->move(knob_x[1] + knob_xy, knob_y[2] + knob_xy);
	m_modulatorSync->setHintText(tr("modulator sync"), "×");

	m_modulatorPulse = new Knob(KnobType::Dark28, this);
	m_modulatorPulse->move(knob_x[2] + knob_xy, knob_y[2] + knob_xy);
	m_modulatorPulse->setHintText(tr("modulator pulse"), "^");
}

void gui::SynchroView::modelChanged()
{
	Synchro *model = castModel<Synchro>();	
	m_carrierWaveform->setModel(&model->m_carrierWaveform);
	m_modulatorWaveform->setModel(&model->m_modulatorWaveform);
	m_resultingWaveform->setModel(&model->m_resultingWaveform);
	m_modAmt->setModel(&model->m_modAmt);
	m_modScale->setModel(&model->m_modScale);
	m_harmonics->setModel(&model->m_harmonics);
	m_octaveRatio->setModel(&model->m_octaveRatio);
	m_carrierDrive->setModel(&model->m_carrier.drive);
	m_carrierSync->setModel(&model->m_carrier.sync);
	m_carrierPulse->setModel(&model->m_carrier.pulse);
	m_modulatorDrive->setModel(&model->m_modulator.drive);
	m_modulatorSync->setModel(&model->m_modulator.sync);
	m_modulatorPulse->setModel(&model->m_modulator.pulse);
}

}
