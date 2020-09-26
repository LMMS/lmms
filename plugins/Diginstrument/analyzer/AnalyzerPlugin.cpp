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
std::string AnalyzerPlugin::setAudioFile(const QString &_audio_file)
{
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

	auto test = SplineSpectrum<float, 4>(PiecewiseBSpline<float, 4>(), 12);

	std::ofstream output;

	output.open("raw.txt");

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

		//process the complex result of the CWT into "amplitude and phase spectrum"
		for (int j = momentarySpectrum.size() - 1; j >= 0; j--)
		{
			const double & re = momentarySpectrum[j].second.first;
			const double & im = momentarySpectrum[j].second.second;
			const double frequency = (double)m_sampleBuffer.sampleRate() / (momentarySpectrum[j].first);
			const double mag = (re*re + im*im);
			//TODO: maybe: do I need phase?
			//const double phase = atan2(im, re);
			//tmp: to reduce oscillations in tiny peaks, set magnitude treshold
			//TODO: add with 0, or just leave out?
			//tmp: amp
			const double amp = (sqrt( (frequency * mag) / (double)m_sampleBuffer.sampleRate()));
			//if(mag>0.0001){rawSpectrum.emplace_back(std::vector<double>{frequency, phase, mag}); }
			//if(amp>0.001){rawSpectrum.emplace_back(std::vector<double>{frequency, phase, amp}); }
			if(amp>0.001){rawSpectrum.emplace_back(std::vector<double>{frequency, amp}); }
			else{ rawSpectrum.emplace_back(std::vector<double>{frequency, 0}); }
			//tmp: raw output
			(*dataRow)[index].setPosition(QVector3D(frequency,amp /*TMP*/, (double)i/(double)m_sampleBuffer.sampleRate()));
			index++;
		}

		*data << dataRow;

		//tmp: note: no checks ,just rvalue insert
		//tmp: TODO: no true peak approximation
		//TODO: tf is this?
		//const auto peaksAndValleys = Analyzer::PeakAndValleyApproximation(Extrema::Differential::intermixed(rawSpectrum.begin(), rawSpectrum.end()));
		//const auto peaksAndValleys = Extrema::Differential::intermixed(rawSpectrum.begin(), rawSpectrum.end());
		const auto peaks = Extrema::Differential::maxima(rawSpectrum.begin(), rawSpectrum.end());
		//tmp: peak output
		for (auto p : peaks)
		{
			//tmp: why is this here?
			if(p.pointType==Extrema::Differential::CriticalPoint::PointType::maximum)
			{
				//auto Y = Interpolation::CubicLagrange(rawSpectrum[p.index-1][0], rawSpectrum[p.index-1][2], rawSpectrum[p.index][0], rawSpectrum[p.index][2], rawSpectrum[p.index+1][0], rawSpectrum[p.index+1][2], rawSpectrum[p.index+2][0], rawSpectrum[p.index+2][2], p.x);
			}
			//tmp: extrema visualization
			if(p.pointType==Extrema::Differential::CriticalPoint::PointType::maximum)
			{
				//TODO: relative path
				visualization->addCustomItem(new QCustom3DItem("/home/mate/projects/lmms/plugins/Diginstrument/analyzer/resources/marker_mesh.obj",
											QVector3D(rawSpectrum[p.index][0], rawSpectrum[p.index][1],(double)i/(double)m_sampleBuffer.sampleRate()),
											QVector3D(0.025f, 0.025f, 0.025f),
											QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 45.0f),
											colorRed));
			}
			if(p.pointType==Extrema::Differential::CriticalPoint::PointType::minimum)
			{
				visualization->addCustomItem(new QCustom3DItem("/home/mate/projects/lmms/plugins/Diginstrument/analyzer/resources/marker_mesh.obj",
											QVector3D(rawSpectrum[p.index][0], rawSpectrum[p.index][1],(double)i/(double)m_sampleBuffer.sampleRate()),
											QVector3D(0.025f, 0.025f, 0.025f),
											QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 45.0f),
											colorBlue));
			}
			if(p.pointType==Extrema::Differential::CriticalPoint::PointType::rising)
			{
				visualization->addCustomItem(new QCustom3DItem("/home/mate/projects/lmms/plugins/Diginstrument/analyzer/resources/marker_mesh.obj",
											QVector3D(rawSpectrum[p.index][0], rawSpectrum[p.index][1],(double)i/(double)m_sampleBuffer.sampleRate()),
											QVector3D(0.025f, 0.025f, 0.025f),
											QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 45.0f),
											colorGreen));
			}
			if(p.pointType==Extrema::Differential::CriticalPoint::PointType::falling)
			{
				visualization->addCustomItem(new QCustom3DItem("/home/mate/projects/lmms/plugins/Diginstrument/analyzer/resources/marker_mesh.obj",
											QVector3D(rawSpectrum[p.index][0], rawSpectrum[p.index][1],(double)i/(double)m_sampleBuffer.sampleRate()),
											QVector3D(0.025f, 0.025f, 0.025f),
											QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 45.0f),
											colorOrange));
			}
			
		}
		//NOTE: there is no true peak approximation yet!
		auto spline = fitter.peakFit(rawSpectrum, peaks);
		//only add "valid" splines
		if (spline.getPieces().size() > 0 /* TMP: i want at least SOMETHING in; && spline.getBegin() <= 12 && spline.getEnd() > 21000*/)
		{
			//inst.addSpectrum(SplineSpectrum(std::move(spline), (double)i/(double)m_sampleBuffer.sampleRate()), {label, (double)i / (double)m_sampleBuffer.sampleRate()});
			spectra.emplace_back(std::move(spline), (double)i/(double)m_sampleBuffer.sampleRate());
		}
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
	
	//TMP: output the synthesised signal and the inverse-CWT of the signal for comparison
	/*auto icwt = transform.inverseTransform();
	for (int i = 0; i < icwt.size()-1; i++)
	{
		const double time = (double)i / (double)m_sampleBuffer.sampleRate();
		auto rec = synth.playNote(inst.getSpectrum({label, time}), 1 ,i, (double)m_sampleBuffer.sampleRate());
		oss << std::fixed << rec.front() << " " << icwt[i] << std::endl;
	}*/
	if(spectra.size()>0) std::cout<<"rejected splines: "<<rejected<<"/"<<spectra.size()<<" ("<<100*rejected/spectra.size()<<"%)"<<std::endl;
	if(rejected>0){
	std::cout<<"cause: no peaks: "<<noComponents<<"/"<<rejected<<" ("<<100*noComponents/rejected<<"%)"<<std::endl;
	std::cout<<"cause: empty spline: "<<emptySplines<<"/"<<rejected<<" ("<<100*emptySplines/rejected<<"%)"<<std::endl;
	std::cout<<"cause: incomplete: "<<incomplete<<"/"<<rejected<<" ("<<100*incomplete/rejected<<"%)"<<std::endl;
	std::cout<<"cause: good begin, bad end: "<<goodBeginBadEnd<<"/"<<rejected<<" ("<<100*goodBeginBadEnd/rejected<<"%)"<<std::endl;
	std::cout<<"cause: good end, bad begin: "<<badBedginGoodEnd<<"/"<<rejected<<" ("<<100*badBedginGoodEnd/rejected<<"%)"<<std::endl;
	}

	//TODO: trim spectrum?
	//TODO: how to mix the output with consistent levels without clipping

	//tmp

	//TODO: get rid of beégetett 4

	//tmp: close files
	output.close();
	visualization->setSurfaceData(data);
	visualization->show();

	return "TODO";
}

QtDataVisualization::QSurfaceDataArray * AnalyzerPlugin::getSurfaceData(float minTime, float maxTime, float minFreq, float maxFreq, int timeSamples, int freqSamples)
{
	const float stepX = (maxFreq - minFreq) / float(freqSamples - 1);
    const float stepZ = (maxTime - minTime) / float(timeSamples - 1);

	QSurfaceDataArray * data = new QSurfaceDataArray;
	data->reserve(timeSamples);
	for(int i = 0; i<timeSamples;i++)
	{
		QSurfaceDataRow *dataRow = new QSurfaceDataRow(freqSamples);
		float z = qMin(maxTime, (i * stepZ + minTime));
		int index = 0;
		auto it = std::lower_bound(spectra.begin(), spectra.end(), z);
		//TMP
		if(it == spectra.end()) break;
		const auto spectrum = *it; /*TMP; TODO*/
		for (int j = 0; j < freqSamples; j++) {
			float x = qMin(maxFreq, (j * stepX + minFreq));
			(*dataRow)[index++].setPosition(QVector3D(x, spectrum[x].amplitude, /*z*/ spectrum.getLabel()));
			//TMP: components only
			//(*dataRow)[index++].setPosition(QVector3D(x, 0, z ));
		}
		//tmp: identical to discrete
		for(const auto & c : spectrum.getComponents(0))
		{
			//TMP: BUGHUNT: missing inbetween piece causes segfault
			if(c.frequency<=minFreq || c.frequency>=maxFreq) continue;
			//(*dataRow)[std::round((c.frequency-minFreq)/((maxFreq-minFreq)/(float)freqSamples))].setPosition(QVector3D(c.frequency,c.amplitude, spectrum.getLabel()));
		}
		
		*data<<dataRow;
	}

	return data;
}
