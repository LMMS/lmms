#include "AnalyzerPlugin.h"

extern "C"
{

	Plugin::Descriptor PLUGIN_EXPORT diginstrument_spectral_analyzer_plugin_descriptor =
		{
			STRINGIFY(PLUGIN_NAME),
			"Diginstrument Spectral Analyzer",
			QT_TRANSLATE_NOOP("pluginBrowser",
							  "WIP"
							  "Test"),
			"Máté Szokolai",
			0x0110,
			Plugin::Tool,
			new PluginPixmapLoader("logo"),
			NULL,
			NULL};

	// necessary for getting instance out of shared lib
	PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *model, void *)
	{
		return new AnalyzerPlugin();
	}
}

AnalyzerPlugin::AnalyzerPlugin() : ToolPlugin(&diginstrument_spectral_analyzer_plugin_descriptor, NULL)
{
	/*TODO */
}

AnalyzerPlugin::~AnalyzerPlugin() {}

PluginView *AnalyzerPlugin::instantiateView(QWidget *_parent)
{
	return new AnalyzerView(this);
}

void AnalyzerPlugin::loadSettings(const QDomElement &_this) {}
void AnalyzerPlugin::saveSettings(QDomDocument &_doc, QDomElement &_parent) {}

QString AnalyzerPlugin::nodeName() const
{
	return "TEST";
}

QString AnalyzerPlugin::fullDisplayName() const
{
	return "TEST";
}

//TMP
std::string AnalyzerPlugin::setAudioFile(const QString &_audio_file)
{
	m_sampleBuffer.setAudioFile(_audio_file);
	/*std::vector<double> sample(m_sampleBuffer.frames());
	for (int i = 0; i < sample.size(); i++)
	{
		//tmp: left only
		sample[i] = m_sampleBuffer.data()[i][0];
	}
	const int level =24;
	CWT transform("morlet", 6, level);
	transform(sample);*/

	//tmp: outputs
	//TODO
	std::ostringstream oss;
	std::ofstream raw;
	std::ofstream peaks;
	std::ofstream spline;

	raw.open("raw.txt");
	peaks.open("peaks.txt");

	//TMP: static label for frequency
	const double label = 440;
	const double transformStep = 0.001*(double)m_sampleBuffer.sampleRate();

	//inst = Analyzer::Interpolator<double, SplineSpectrum<double, 4>>();
	//add empty spectrum to end
	//inst.addSpectrum(SplineSpectrum<double, 4>((double)m_sampleBuffer.frames() / (double)m_sampleBuffer.sampleRate()), {label, (double)m_sampleBuffer.frames() / (double)m_sampleBuffer.sampleRate()});
	//SpectrumFitter<double, 4> fitter(1.25);

	//new loop
	/*for (int i = 0; i<m_sampleBuffer.frames(); i+=transformStep)
	{
		const auto momentarySpectrum = transform[i];
		std::vector<std::vector<double>> rawSpectrum;
		rawSpectrum.reserve(level * 11);
		//process the complex result of the CWT into "amplitude and phase spectrum"
		for (int j = momentarySpectrum.size() - 1; j >= 0; j--)
		{
			const double & re = momentarySpectrum[j].second.first;
			const double & im = momentarySpectrum[j].second.second;
			const double frequency = (double)m_sampleBuffer.sampleRate() / (momentarySpectrum[j].first);
			const double mag = (re*re + im*im);
			//TODO: maybe: do I need phase?
			const double phase = atan2(im, re);
			//tmp: to reduce oscillations in tiny peaks, set magnitude treshold
			//TODO: add with 0, or just leave out?
			//tmp: amp
			//const double amp = (sqrt( (frequency * component.amplitude) / (double)sampleRate));
			if(mag>0.0001){rawSpectrum.emplace_back(std::vector<double>{frequency, phase, mag}); }
			//if(amp>0.001){rawSpectrum.emplace_back(std::vector<double>{frequency, phase, amp}); }
			else{ rawSpectrum.emplace_back(std::vector<double>{frequency, phase, 0}); }

			//tmp: raw output
			raw<<std::fixed<<(double)i/(double)m_sampleBuffer.sampleRate()<<" "<<frequency<<" "<<mag<<std::endl;
		}

		//tmp: note: no checks ,just rvalue insert
		//tmp: no peak approximation
		//const auto peaksAndValleys = Analyzer::PeakAndValleyApproximation(Extrema::Differential::intermixed(rawSpectrum.begin(), rawSpectrum.end()));
		const auto peaksAndValleys = Extrema::Differential::intermixed(rawSpectrum.begin(), rawSpectrum.end());
		//tmp: peak output
		for (auto p : peaksAndValleys)
		{
			//tmp
			if(p.pointType==Extrema::Differential::CriticalPoint::PointType::maximum)
			{
				auto Y = Interpolation::CubicLagrange(rawSpectrum[p.index-1][0], rawSpectrum[p.index-1][2], rawSpectrum[p.index][0], rawSpectrum[p.index][2], rawSpectrum[p.index+1][0], rawSpectrum[p.index+1][2], rawSpectrum[p.index+2][0], rawSpectrum[p.index+2][2], p.x);
				peaks<<std::fixed<<(double)i/(double)m_sampleBuffer.sampleRate()<<" "<<p.x<<" "<<Y<<std::endl;
			}
		}
		const auto spline = fitter.peakValleyFit(rawSpectrum, peaksAndValleys);
		inst.addSpectrum(
			SplineSpectrum(
				//spline
				spline,
				//spline label (time)
				(double)i/(double)m_sampleBuffer.sampleRate()
			)
			//coordinates: {pitch, time}
			//tmp: fix frequency label
			, {label, (double)i / (double)m_sampleBuffer.sampleRate()}
		);
	}*/

	// int rejected = 0;
	// int noComponents = 0;
	// int incomplete = 0;
	// int emptySplines = 0;
	// int goodBeginBadEnd = 0;
	// int badBedginGoodEnd = 0;
	// 	//only add "valid" splines
	// 	if (spline.getPieces().size() > 0 /*&& spline.getBegin() <= 12 && spline.getEnd() > 21000*/)
	// 	{
	// 		inst.addSpectrum(SplineSpectrum(std::move(spline), (double)i/(double)m_sampleBuffer.sampleRate()), {label, (double)i / (double)m_sampleBuffer.sampleRate()});
	// 	}
	// 	else
	// 	{
	// 		if(spline.getPeaks().size()==0 && spline.getEnd()>0) {noComponents++;}
	// 		if(spline.getPieces().size()==0) {emptySplines++;}
	// 		if((spline.getBegin() > 12 && spline.getBegin()>0) || (spline.getEnd() < 21000 && spline.getEnd()>0)) {incomplete++;}
	// 		if(spline.getBegin()<12 && spline.getEnd()<21000 && spline.getBegin()>0 && spline.getEnd()>0) { goodBeginBadEnd++; }
	// 		if(spline.getBegin()>12 && spline.getEnd()>21000 && spline.getBegin()>0 && spline.getEnd()>0) { badBedginGoodEnd++; }
	// 		rejected++;
	// 	}
	// }
	
	//TMP: output the synthesised signal and the inverse-CWT of the signal for comparison
	/*auto icwt = transform.inverseTransform();
	for (int i = 0; i < icwt.size()-1; i++)
	{
		const double time = (double)i / (double)m_sampleBuffer.sampleRate();
		auto rec = synth.playNote(inst.getSpectrum({label, time}), 1 ,i, (double)m_sampleBuffer.sampleRate());
		oss << std::fixed << rec.front() << " " << icwt[i] << std::endl;
	}*/
	/*std::cout<<"rejected splines: "<<rejected<<"/"<<spectra.size()/transformStep<<" ("<<100*rejected/(spectra.size()/transformStep)<<"%)"<<std::endl;
	if(rejected>0){
	std::cout<<"cause: no peaks: "<<noComponents<<"/"<<rejected<<" ("<<100*noComponents/rejected<<"%)"<<std::endl;
	std::cout<<"cause: empty spline: "<<emptySplines<<"/"<<rejected<<" ("<<100*emptySplines/rejected<<"%)"<<std::endl;
	std::cout<<"cause: incomplete: "<<incomplete<<"/"<<rejected<<" ("<<100*incomplete/rejected<<"%)"<<std::endl;
	std::cout<<"cause: good begin, bad end: "<<goodBeginBadEnd<<"/"<<rejected<<" ("<<100*goodBeginBadEnd/rejected<<"%)"<<std::endl;
	std::cout<<"cause: good end, bad begin: "<<badBedginGoodEnd<<"/"<<rejected<<" ("<<100*badBedginGoodEnd/rejected<<"%)"<<std::endl;
	}*/

	//TODO: trim spectrum?
	//TODO: how to mix the output with consistent levels without clipping

	//tmp
	std::cout << oss.str() << std::endl;

	//TODO: get rid of beégetett 4

	//tmp: close files
	raw.close();
	peaks.close();

	return oss.str();
}
