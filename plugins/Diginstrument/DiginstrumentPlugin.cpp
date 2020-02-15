
/*tmp*/
#include "CWT.hpp"
#include "Extrema.hpp"
#include "Approximation.hpp"
#include "SplineFitter.hpp"
#include "PiecewiseBSpline.hpp"
#include "SpectrumFitter.hpp"
#include "SplineSpectrum.hpp"
#include <string>
#include <sstream>
#include <iostream>

#include "DiginstrumentPlugin.h"

extern "C"
{

	Plugin::Descriptor PLUGIN_EXPORT diginstrument_plugin_descriptor =
		{
			STRINGIFY(PLUGIN_NAME),
			"Diginstrument",
			QT_TRANSLATE_NOOP("pluginBrowser",
							  "WIP"
							  "Test"),
			"Máté Szokolai",
			0x0110,
			Plugin::Instrument,
			new PluginPixmapLoader("logo"),
			NULL,
			NULL};

	// necessary for getting instance out of shared lib
	PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *model, void *)
	{
		return new DiginstrumentPlugin(static_cast<InstrumentTrack *>(model));
	}
}

DiginstrumentPlugin::DiginstrumentPlugin(InstrumentTrack *_instrument_track) : Instrument(_instrument_track, &diginstrument_plugin_descriptor)
{
	/*TODO */
	synth.setSampleRate(Engine::mixer()->processingSampleRate());
}

DiginstrumentPlugin::~DiginstrumentPlugin() {}

PluginView *DiginstrumentPlugin::instantiateView(QWidget *_parent)
{
	return new DiginstrumentView(this, _parent);
}

void DiginstrumentPlugin::loadSettings(const QDomElement &_this) {}
void DiginstrumentPlugin::saveSettings(QDomDocument &_doc, QDomElement &_parent) {}

QString DiginstrumentPlugin::nodeName() const
{
	return "TEST";
}

void DiginstrumentPlugin::playNote(NotePlayHandle *noteHandle,
								   sampleFrame *_working_buf)
{
	/*TMP*/
	double time = (noteHandle->framesLeftForCurrentPeriod() + noteHandle->totalFramesPlayed()) / (double)/*tmp*/ Engine::mixer()->processingSampleRate();
	auto audioData = this->synth.playNote(inst.getSpectrum({noteHandle->frequency(), time}), noteHandle->framesLeftForCurrentPeriod(), noteHandle->totalFramesPlayed());
	/*tmp: stereo*/
	unsigned int counter = 0;
	unsigned int offset = noteHandle->noteOffset();
	for (auto frame : audioData)
	{
		_working_buf[counter + offset][0] = _working_buf[counter + offset][1] = frame;
		counter++;
	}
	applyRelease(_working_buf, noteHandle);
	instrumentTrack()->processAudioBuffer(_working_buf, audioData.size() + noteHandle->noteOffset(), noteHandle);
}

void DiginstrumentPlugin::deleteNotePluginData(NotePlayHandle *_note_to_play)
{
}

f_cnt_t DiginstrumentPlugin::beatLen(NotePlayHandle *_n) const
{
	return 0;
}

QString DiginstrumentPlugin::fullDisplayName() const
{
	return "TEST";
}

void DiginstrumentPlugin::sampleRateChanged()
{
	/*TODO*/
	this->synth.setSampleRate(Engine::mixer()->processingSampleRate());
}

//TMP
std::string DiginstrumentPlugin::setAudioFile(const QString &_audio_file)
{
	m_sampleBuffer.setAudioFile(_audio_file);
	std::vector<double> sample(m_sampleBuffer.frames());
	for (int i = 0; i < sample.size(); i++)
	{
		//tmp: left only
		sample[i] = m_sampleBuffer.data()[i][0];
	}
	const int level = 12;
	CWT transform("morlet", 6, level);
	transform(sample);

	std::ostringstream oss;
	std::vector<std::pair<double, double>> points;
	points.reserve(level * 11);

	//TMP: static label for frequency
	double label = 440;

	inst = Diginstrument::Interpolator<double, SplineSpectrum<double>>();
	inst.addSpectrum(SplineSpectrum<double>(label), {label, (double)m_sampleBuffer.frames() / (double)m_sampleBuffer.sampleRate()});
	for (int i = 0; i < m_sampleBuffer.frames(); /*tmp i+=0.001*(double)m_sampleBuffer.sampleRate()*/ i++)
	{
		const auto momentarySpectrum = transform[i];
		std::vector<std::pair<double, double>> points;
		points.reserve(level * 11);

		for (int i = momentarySpectrum.size() - 1; i >= 0; i--)
		{
			double re = momentarySpectrum[i].second.first;
			double im = momentarySpectrum[i].second.second;
			double frequency = (double)m_sampleBuffer.sampleRate() / (momentarySpectrum[i].first);
			//tmp: convert to amplitude here
			//double modulus = sqrt(re*re + im*im);
			const double amp = sqrt(frequency * (re * re + im * im)) / 200.0;
			points.emplace_back(frequency, amp);
			//output magnitude spectrum
			//oss<<std::fixed<<"("<<frequency<<","<<amp<<"),";
			//oss<<std::fixed<<scale<<" "<<modulus<<std::endl;
		}
		//fit spline to magnitude spectrum
		SpectrumFitter<double, 4> fitter(2);
		SplineSpectrum<double> spectrum(fitter.fit(points), label);
		//only add "valid" splines
		if (spectrum.getHarmonics().size() > 0 && spectrum.getBegin() <= 20 && spectrum.getEnd() > 20000)
		{
			inst.addSpectrum(spectrum, {label, (double)i / (double)m_sampleBuffer.sampleRate()});
		}
	}

	//output the synthesised signal and the inverse-CWT of the signal for comparison
	auto icwt = transform.inverseTransform();
	for (int i = 0; i < icwt.size(); i++)
	{
		const double time = (double)i / (double)m_sampleBuffer.sampleRate();
		auto rec = synth.playNote(inst.getSpectrum({label, time}), 1, i);
		oss << std::fixed << rec.front() << " " << icwt[i] << std::endl;
	}

	//TODO: trim spectrum?
	//TODO: how to mix the output with consistent levels without clipping

	//TODO: get rid of std::pair

	//tmp
	std::cout << oss.str() << std::endl;

	return oss.str();
}
