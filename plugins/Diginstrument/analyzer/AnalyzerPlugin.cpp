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
std::string AnalyzerPlugin::setAudioFile(const QString &_audio_file, vector<pair<string, double>> coordinates)
{
	//TMP: keep for visualization
	spectra.clear();
	//tmp
	visualization = new Diginstrument::InstrumentVisualizationWindow(this);
	std::vector<QVector3D> extremaVisualization;
	QImage colorRed = QImage(2, 2, QImage::Format_RGB32);
	colorRed.fill(Qt::red);
	QImage colorBlue = QImage(2, 2, QImage::Format_RGB32);
	colorBlue.fill(Qt::blue);
	QImage colorGreen = QImage(2, 2, QImage::Format_RGB32);
	colorGreen.fill(Qt::green);
	QImage colorOrange = QImage(2, 2, QImage::Format_RGB32);
	colorOrange.fill(Qt::darkYellow);

	m_sampleBuffer.setAudioFile(_audio_file);
	std::vector<double> sample(m_sampleBuffer.frames());
	for (int i = 0; i < sample.size(); i++)
	{
		//tmp: left only
		sample[i] = m_sampleBuffer.data()[i][0];
	}
	const int level =24;
	CWT transform("morlet", 6, level);
	transform(sample);

	//tmp: statistics
	int rejected = 0;
	int noComponents = 0;
	int incomplete = 0;
	int emptySplines = 0;
	int goodBeginBadEnd = 0;
	int badBedginGoodEnd = 0;

	const double transformStep = 0.01*(double)m_sampleBuffer.sampleRate();
	SpectrumFitter<double, 4> fitter(1.25);

	//tmp
	QSurfaceDataArray * data = new QSurfaceDataArray;
	data->reserve(m_sampleBuffer.frames()/transformStep);

	//new loop
	for (int i = 0; i<m_sampleBuffer.frames(); i+=transformStep)
	{
		const auto momentarySpectrum = transform[i];
		std::vector<std::vector<double>> rawSpectrum;
		rawSpectrum.reserve(level * 11);
		//tmp
		QSurfaceDataRow *dataRow = new QSurfaceDataRow(level * 11);
		int index = 0;

		//process the complex result of the CWT into a magnitude spectrum
		for (int j = momentarySpectrum.size() - 1; j >= 0; j--)
		{
			const double & re = momentarySpectrum[j].second.first;
			const double & im = momentarySpectrum[j].second.second;
			const double frequency = (double)m_sampleBuffer.sampleRate() / (momentarySpectrum[j].first);
			const double mag = (re*re + im*im);
			//const double phase = atan2(im, re);
			//tmp: to reduce oscillations in tiny peaks, set magnitude treshold
			//tmp: magnitude spectrogram
			if(mag>0.01){rawSpectrum.emplace_back(std::vector<double>{frequency, mag}); }
			else{ rawSpectrum.emplace_back(std::vector<double>{frequency, 0}); }
			//tmp: amp
			//const double amp = (sqrt( (frequency * mag) / (double)m_sampleBuffer.sampleRate()));
			//tmp: raw output
			///tmp: magnitude spectrogram
			(*dataRow)[index].setPosition(QVector3D(frequency,mag /*TMP amp*/, (double)i/(double)m_sampleBuffer.sampleRate()));
			index++;
		}

		*data << dataRow;

		//seek critical points with discrete differential, then approximate hidden/overlapping peaks and filter to only include maxima
		//const auto peaks = Diginstrument::PeakApproximation(Extrema::Differential::intermixed(rawSpectrum.begin(), rawSpectrum.end()));
		//tmp: trying to fix weird interpolation problem, just use maxima for now, as hidden peaks are still primitive
		const auto peaks = Extrema::Differential::maxima(rawSpectrum.begin(), rawSpectrum.end());
		//TODO: is this the best place to convert to amp?
		//after determining peaks, convert magnitude to amplitude
		//TMP: magnitude spectrogram
		//TMP: fit to magnitude!
		/*for(auto & p : rawSpectrum)
		{
			p[1] = (sqrt( (p[0] * p[1]) / (double)m_sampleBuffer.sampleRate()));
		}*/
		//tmp: visualize peaks
		for (auto p : peaks)
		{
			const auto Y = Interpolation::CubicLagrange(rawSpectrum[p.index-1][0], rawSpectrum[p.index-1][1], rawSpectrum[p.index][0], rawSpectrum[p.index][1], rawSpectrum[p.index+1][0], rawSpectrum[p.index+1][1], rawSpectrum[p.index+2][0], rawSpectrum[p.index+2][1], p.x);
			if(p.pointType==Extrema::Differential::CriticalPoint::PointType::maximum)
			{
				//TODO: relative path
				visualization->addCustomItem(new QCustom3DItem("/home/mate/projects/lmms/plugins/Diginstrument/analyzer/resources/marker_mesh.obj",
											QVector3D(p.x, Y,(double)i/(double)m_sampleBuffer.sampleRate()),
											QVector3D(0.025f, 0.025f, 0.025f),
											QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 45.0f),
											colorRed));
			}
		}
	
		//fit spline to raw spectrum
		auto spline = fitter.peakFit(rawSpectrum, peaks);
		//only add "valid" splines
		if (spline.getPieces().size() > 0 && spline.getBegin() <= 12 && spline.getEnd() > 21000)
		{
			//TMP: keep for visualization
			spectra.emplace_back(spline,std::vector<std::pair<std::string, double>>{std::make_pair("time",(double)i/(double)m_sampleBuffer.sampleRate())});
			auto coordinatesCopy = coordinates;
			coordinatesCopy.emplace_back("time",(double)i/(double)m_sampleBuffer.sampleRate());
			inst.add(SplineSpectrum<double, 4>(
				std::move(spline),
				std::move(coordinatesCopy)
				));
		}
		//TMP: rejection statistics
		else
		{
			if(spline.getPeaks().size()==0 && spline.getEnd()>0) {noComponents++;}
			if(spline.getPieces().size()==0) {emptySplines++;}
			if((spline.getBegin() > 12 && spline.getBegin()>0) || (spline.getEnd() < 21000 && spline.getEnd()>0)) {incomplete++;}
			if(spline.getBegin()<12 && spline.getEnd()<21000 && spline.getBegin()>0 && spline.getEnd()>0) { goodBeginBadEnd++; }
			if(spline.getBegin()>12 && spline.getEnd()>21000 && spline.getBegin()>0 && spline.getEnd()>0) { badBedginGoodEnd++; }
			rejected++;
		}
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
	//tmp: show raw visualization
	visualization->setSurfaceData(data);
	visualization->show();

	//tmp:
	inst.dimensions.emplace_back("pitch",20,22000);
	inst.dimensions.emplace_back("time",0,((double)m_sampleBuffer.frames()/(double)m_sampleBuffer.sampleRate())*1000);
	writeInstrumentToFile("test2.json");

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

