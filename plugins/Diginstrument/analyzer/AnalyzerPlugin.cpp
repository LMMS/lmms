#include "AnalyzerPlugin.h"

using namespace QtDataVisualization;

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
std::string AnalyzerPlugin::analyzeSample(const QString &_audio_file, vector<pair<string, double>> coordinates)
{
	//TMP: keep for visualization
	spectra.clear();

	m_sampleBuffer.setAudioFile(_audio_file);
	std::vector<double> sample(m_sampleBuffer.frames());
	for (int i = 0; i < sample.size(); i++)
	{
		//tmp: left only
		sample[i] = m_sampleBuffer.data()[i][0];
	}

	//tmp:visualization
	visualization = new Diginstrument::InstrumentVisualizationWindow(this);

	analyze(sample, subtractiveAnalysis(sample, m_sampleBuffer.sampleRate(), coordinates), coordinates);

	//tmp: show raw visualization
	visualization->show();

	return "TODO";
}

QtDataVisualization::QSurfaceDataArray * AnalyzerPlugin::getSurfaceData(double minTime, double maxTime, double minFreq, double maxFreq, int timeSamples, int freqSamples)
{
	const double stepX = (maxFreq - minFreq) / double(freqSamples - 1);
    const double stepZ = (maxTime - minTime) / double(timeSamples - 1);

	QSurfaceDataArray * data = new QSurfaceDataArray;
	data->reserve(timeSamples);
	for(int i = 0; i<timeSamples;i++)
	{
		QSurfaceDataRow *dataRow = new QSurfaceDataRow(freqSamples);
		double z = qMin(maxTime, (i * stepZ + minTime));
		int index = 0;
		//TODO: new instrument model invalidated this: needs coordinates now
		auto it = std::lower_bound(spectra.begin(), spectra.end(), SplineSpectrum<double, 4>(std::vector<std::pair<std::string, double>>{std::make_pair("time", z)}));
		//TMP
		if(it == spectra.end()) break;
		//TMP
		const auto spectrum = *it;
		for (int j = 0; j < freqSamples; j++) {
			double x = qMin(maxFreq, (j * stepX + minFreq));
			(*dataRow)[index++].setPosition(QVector3D(x, spectrum[x].amplitude, z));
		}
		//tmp: identical to discrete
		for(const auto & c : spectrum.getComponents(0))
		{
			//NOTE: BUGHUNT: missing inbetween piece causes segfault
			if(c.frequency<=minFreq || c.frequency>=maxFreq) continue;
			(*dataRow)[std::round((c.frequency-minFreq)/((maxFreq-minFreq)/(double)freqSamples))].setPosition(QVector3D(c.frequency,c.amplitude, z));
		}
		
		*data<<dataRow;
	}

	return data;
}

void AnalyzerPlugin::writeInstrumentToFile(std::string filename)
{
	ofstream file(filename);
	if(file.is_open())
	{
		//TMP: pretty printed
		file<<fixed<<inst.toString(4);
		file.close();
	}
}

void AnalyzerPlugin::analyze(const std::vector<double> & signal, std::vector<std::vector<Diginstrument::Component<double>>> partials, vector<pair<string, double>> coordinates)
{
	//do CWT
	const int level = 18;
	CWT transform("morlet", 6, level, m_sampleBuffer.sampleRate());
	const double normalizationConstant = CWT::calculateMagnitudeNormalizationConstant(6);
	transform(signal);

	const double transformStep = 0.01*(double)m_sampleBuffer.sampleRate();
	SpectrumFitter<double, 4> fitter(1.25);

	//tmp: statistics
	int rejected = 0;
	int noComponents = 0;
	int incomplete = 0;
	int emptySplines = 0;
	int goodBeginBadEnd = 0;
	int badBedginGoodEnd = 0;	

	//tmp: visualization
	QImage colorRed = QImage(2, 2, QImage::Format_RGB32);
	colorRed.fill(Qt::red);
	QSurfaceDataArray * data = new QSurfaceDataArray;
	data->reserve(m_sampleBuffer.frames()/transformStep);

	//for each instance in time
	for (int i = 0; i<m_sampleBuffer.frames(); i+=transformStep)
	{
		//get the coefficients in time
		const auto momentarySpectrum = transform[i];
		std::vector<std::pair<double, double>> rawSpectrum;
		rawSpectrum.reserve(level * CWT::octaves);
		//tmp: visualization
		QSurfaceDataRow *dataRow = new QSurfaceDataRow(level * CWT::octaves);
		int dataRowIndex = 0;

		//process the complex coefficients of the momentary CWT into a magnitude spectrum
		for (int j = momentarySpectrum.size() - 1; j >= 0; j--)
		{
			const auto & timeInstance = momentarySpectrum[j];
			const double frequency = 1.0 / (timeInstance.period);
			//TODO: is phase needed in residual?
			//const double phase = std::ang(timeInstance.value);
			//normalize magnitude
			//TODO: source of constant?
			const double mag = (std::abs(timeInstance.value*normalizationConstant) / (sqrt(timeInstance.scale*m_sampleBuffer.sampleRate())));
			rawSpectrum.emplace_back(frequency, mag);
			///tmp: visualization
			(*dataRow)[dataRowIndex].setPosition(QVector3D(frequency, mag, (double)i/(double)m_sampleBuffer.sampleRate()));
			dataRowIndex++;
		}
		//tmp:: visualization
		*data << dataRow;

		//TODO: fix and reintroduce peaks?

		//seek critical points with discrete differential, then approximate hidden/overlapping peaks and filter to only include maxima
		//const auto peaks = Diginstrument::PeakApproximation(Extrema::Differential::intermixed(rawSpectrum.begin(), rawSpectrum.end()));
		//tmp: trying to fix weird interpolation problem, just use maxima for now, as hidden peaks are still primitive
		//const auto peaks = Extrema::Differential::maxima(rawSpectrum.begin(), rawSpectrum.end(), 0.001);
		const auto currentPartials = partials[i];
		//TODO: is this the best place to convert to amp?
		//after determining peaks, convert magnitude to amplitude
		//TMP: magnitude spectrogram
		//TMP: fit to magnitude!
		/*for(auto & p : rawSpectrum)
		{
			p[1] = (sqrt( (p[0] * p[1]) / (double)m_sampleBuffer.sampleRate()));
		}*/
		//tmp: visualize peaks
		/*for (auto p : peaks)
		{
			const auto Y = Interpolation::CubicLagrange(rawSpectrum[p.index-1].first, rawSpectrum[p.index-1].second, rawSpectrum[p.index].first, rawSpectrum[p.index].second, rawSpectrum[p.index+1].first, rawSpectrum[p.index+1].second, rawSpectrum[p.index+2].first, rawSpectrum[p.index+2].second, p.x);
			if(p.pointType==Extrema::Differential::CriticalPoint::PointType::maximum)
			{
				//TODO: relative path
				visualization->addCustomItem(new QCustom3DItem("/home/mate/projects/lmms/plugins/Diginstrument/analyzer/resources/marker_mesh.obj",
											QVector3D(p.x, Y,(double)i/(double)m_sampleBuffer.sampleRate()),
											QVector3D(0.025f, 0.025f, 0.025f),
											QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 45.0f),
											colorRed));
			}
		}*/
	
		//fit spline to raw spectrum
		//tmp: just conver pairs back into vectors
		/*vector<vector<double>> convertedRawSpectrum;
		for(auto & p : rawSpectrum)
		{
			convertedRawSpectrum.push_back({p.first, p.second});
		}
		auto spline = fitter.peakFit(convertedRawSpectrum, peaks);*/
		//auto spline = fitter.fitToPartials(rawSpectrum, currentPartials);
		//only add "valid" splines
		//inst.add(Diginstrument::TimeSlice<double, 4>(currentPartials, std::vector<std::pair<std::string, double>>{std::make_pair("time",(double)i/(double)m_sampleBuffer.sampleRate())}));
		/*if (spline.getPieces().size() > 0 && spline.getBegin() <= 12 && spline.getEnd() > 21000)
		{
			//TMP: keep for visualization
			//spectra.emplace_back(spline,std::vector<std::pair<std::string, double>>{std::make_pair("time",(double)i/(double)m_sampleBuffer.sampleRate())});
			auto coordinatesCopy = coordinates;
			coordinatesCopy.emplace_back("time",(double)i/(double)m_sampleBuffer.sampleRate());
			inst.add(SplineSpectrum<double, 4>(
				std::move(spline),
				std::move(coordinatesCopy)
				));
		}*/
		//TMP: rejection statistics
		/*else
		{
			if(spline.getPeaks().size()==0 && spline.getEnd()>0) {noComponents++;}
			if(spline.getPieces().size()==0) {emptySplines++;}
			if((spline.getBegin() > 12 && spline.getBegin()>0) || (spline.getEnd() < 21000 && spline.getEnd()>0)) {incomplete++;}
			if(spline.getBegin()<12 && spline.getEnd()<21000 && spline.getBegin()>0 && spline.getEnd()>0) { goodBeginBadEnd++; }
			if(spline.getBegin()>12 && spline.getEnd()>21000 && spline.getBegin()>0 && spline.getEnd()>0) { badBedginGoodEnd++; }
			rejected++;
		}*/
	}
	//tmp: add an empty spline at the end for silence
	auto coordinatesCopy = coordinates;
	coordinatesCopy.emplace_back("time",((double)m_sampleBuffer.frames()/(double)m_sampleBuffer.sampleRate()));
	inst.add(SplineSpectrum<double, 4>(PiecewiseBSpline<double, 4>(),coordinatesCopy));
	
	//tmp: debug
	//if(spectra.size()>0) std::cout<<"rejected splines: "<<rejected<<"/"<<spectra.size()<<" ("<<100*rejected/spectra.size()<<"%)"<<std::endl;
	if(rejected>0){
		std::cout<<"cause: no peaks: "<<noComponents<<"/"<<rejected<<" ("<<100*noComponents/rejected<<"%)"<<std::endl;
		std::cout<<"cause: empty spline: "<<emptySplines<<"/"<<rejected<<" ("<<100*emptySplines/rejected<<"%)"<<std::endl;
		std::cout<<"cause: incomplete: "<<incomplete<<"/"<<rejected<<" ("<<100*incomplete/rejected<<"%)"<<std::endl;
		std::cout<<"cause: good begin, bad end: "<<goodBeginBadEnd<<"/"<<rejected<<" ("<<100*goodBeginBadEnd/rejected<<"%)"<<std::endl;
		std::cout<<"cause: good end, bad begin: "<<badBedginGoodEnd<<"/"<<rejected<<" ("<<100*badBedginGoodEnd/rejected<<"%)"<<std::endl;
	}

	//TODO: get rid of beégetett 4
	//tmp: set raw visualization data
	visualization->setSurfaceData(data);
}

//TODO: current output: spectra over time
//problems: FULL output = samples*partials
//phase + magnitude
//TODO: maybe reduce samples by excluding places of linear phase? and linear mag? then i will need to include time? possibly useless? only zero magnitude?
std::vector<std::vector<Diginstrument::Component<double>>> AnalyzerPlugin::subtractiveAnalysis(std::vector<double> & signal, unsigned int sampleRate, vector<pair<string, double>> coordinates)
{
	//tmp: FFT visualization
	QImage colorBlue = QImage(2, 2, QImage::Format_RGB32);
	colorBlue.fill(Qt::black);

	//TODO: avg energy to detect broad-changing frequencies
	//calculate FFT magnitudes of the signal
	Diginstrument::FFT fft(signal.size());
	const auto mags = fft(signal, m_sampleBuffer.sampleRate());
	//find peaks in FFT, indicating areas of significance
	//TODO: make parameters variable
	const auto maxima = Extrema::Differential::maxima(mags.begin(), mags.end(), 0.002, 2);
	//TMP: visualize found frequencies
	for(auto p : maxima)
	{
		//tmp: debug
		//cout<<p.x<<": ("<<mags[p.index].first<<", "<<mags[p.index].second<<")"<<endl;
		const auto Y = Interpolation::CubicLagrange(mags[p.index-1].first, mags[p.index-1].second, mags[p.index].first, mags[p.index].second, mags[p.index+1].first, mags[p.index+1].second, mags[p.index+2].first, mags[p.index+2].second, p.x);
		visualization->addCustomItem(new QCustom3DItem("/home/mate/projects/lmms/plugins/Diginstrument/analyzer/resources/marker_mesh.obj",
												QVector3D(p.x, Y, 0),
												QVector3D(0.025f, 0.025f, 0.025f),
												QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 45.0f),
												colorBlue));
	}

	vector<vector<double>> phases;
	vector<vector<double>> amps;
	std::vector<std::vector<Diginstrument::Component<double>>> partials;
	std::vector<std::vector<Diginstrument::Component<double>>> res(signal.size());
	//tmp: visualization
	const auto palette = Diginstrument::ColorPalette::generatePaletteTextures(maxima.size());

	constexpr double parameter = 6;
	const double normalizationConstant = CWT::calculateMagnitudeNormalizationConstant(parameter);
	//for each selected frequency
	for(int j = 0; j<maxima.size(); j++)
	{
		const double fr = maxima[j].x;
		//TODO: tie wavelet parameters together
		const double scale = (parameter+sqrt(2+parameter*parameter)) / (4*M_PI*fr);
		//perform a single center frequency CWT
		const auto cfs = CWT::singleScaleCWT(signal, scale, sampleRate);
		phases.push_back(vector<double>(signal.size(),0));
		amps.push_back(vector<double>(signal.size(),0));
		//for each instance in time
		for(int i = 0; i<cfs.size(); i++)
		{
			//process the complex coefficient
			const auto & c = cfs[i];
			//TODO: this has been the most successful equation, as it resulted in the same amp for both components in 440+50.wav
			//I have no idea where the constant comes from; probably ties into the wavelet parameter
			const double amp = (std::abs(normalizationConstant*c) / (sqrt(scale*sampleRate)));
			amps.back()[i] = amp;
			phases.back()[i] = std::arg(c);
			
		}
		//unwrap phase
		Diginstrument::Phase::unwrapInPlace(phases.back());
		//partial initialization
		vector<Diginstrument::Component<double>> partial;
		partial.reserve(amps.size());
		for(int i = 0; i<signal.size(); i++)
		{
			//tmp: add partial to instrument
			partial.emplace_back(fr, phases.back()[i], amps.back()[i]);
			//subtract extracted partial to get residual signal
			signal[i]-=cos(phases.back()[i])*amps.back()[i];
			//TODO: rethink output
			res[i].emplace_back(fr, phases.back()[i], amps.back()[i]);
			//tmp: visualization
			if(i%440 == 1 && amps.back()[i]>0.001)
			{
				//calculate freq for visualization from diff of phase
				const double freq = abs(((phases.back()[i] - phases.back()[i-1]) * m_sampleBuffer.sampleRate()) / (2*M_PI));
				visualization->addCustomItem(new QCustom3DItem("/home/mate/projects/lmms/plugins/Diginstrument/analyzer/resources/marker_mesh.obj",
												QVector3D(freq, amps.back()[i],(double)i/(double)sampleRate),
												QVector3D(0.01f, 0.01f, 0.01f),
												QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 45.0f),
												palette[j]));
			}
		}
		//add partial to set
		partials.push_back(std::move(partial));
	}
	//add partial set to instrument
	//TODO: checks
	inst.add(PartialSet<double>(std::move(partials), std::move(coordinates), m_sampleBuffer.sampleRate()));

	return res;
}
