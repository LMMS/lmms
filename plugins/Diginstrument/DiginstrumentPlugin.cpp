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
	const double endTime = (noteHandle->framesLeftForCurrentPeriod() + noteHandle->totalFramesPlayed()) / (double)/*tmp*/ Engine::mixer()->processingSampleRate();
	const double startTime = noteHandle->totalFramesPlayed() / (double)Engine::mixer()->processingSampleRate();
	//TODO: store previous spectrum
	const auto startSpectrum = inst.getSpectrum({noteHandle->frequency(), startTime});
	const auto endSpectrum = inst.getSpectrum({noteHandle->frequency(), endTime});
	auto audioData = this->synth.playNote(startSpectrum, endSpectrum, noteHandle->framesLeftForCurrentPeriod(), noteHandle->totalFramesPlayed());
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
	const int level = 24;
	CWT transform("morlet", 6, level);
	transform(sample);

	std::ostringstream oss;

	//TMP: static label for frequency
	double label = 440;

	//TMP: debug phase
	std::ostringstream origPhases;
	std::ostringstream phases;
	std::ostringstream resStream;
	std::ostringstream intPhases;

	inst = Diginstrument::Interpolator<double, SplineSpectrum<double, 4>>();
	inst.addSpectrum(SplineSpectrum<double, 4>((double)m_sampleBuffer.frames() / (double)m_sampleBuffer.sampleRate()), {label, (double)m_sampleBuffer.frames() / (double)m_sampleBuffer.sampleRate()});

	std::vector<std::vector<std::vector<double>>> spectra;
	std::vector<std::pair<std::vector<unsigned int>, std::vector<unsigned int>>> extremaVector;

	//TODO: reserve based on how many moments are processed
	//spectra.reserve(?);
	//extrema.reserve(?);
	const double transformStep = 0.01*(double)m_sampleBuffer.sampleRate();
	//TODO: make it possible to skip transforms
	// 1) process transforms into spectra
		// 1.1) simultaniously get maxima
		// 1.2) simultaniously approximate true maxima
		std::vector<int> phaseFloors(level * 11, 0);
		std::vector<double> prevPhases(level * 11);
	for (int i = 0; i < m_sampleBuffer.frames(); i++)
	{
		const auto momentarySpectrum = transform[i];
		std::vector<std::vector<double>> points;
		points.reserve(level * 11);
		constexpr double alpha = M_PI;
		std::vector<double> values;
		values.reserve(level * 11);

		//TMP: debug phase
		//NOTE: TODO: WIP: best results so far: no phase int. in true peak, then time interpolation in second round
		//OR NOT: sine is a bit fucked then
		//TODO: compare these methods BEORE Interpolator
		//1) no rejection, no interpolation
		//2) no rejection, first interpolation
		//3) no rejection, second interpolation <-skip for now
		//4) no rejection, both interpolation <-skip


		//convert transform samples into points and values
		for (int j = momentarySpectrum.size() - 1; j >= 0; j--)
		{
			double re = momentarySpectrum[j].second.first;
			double im = momentarySpectrum[j].second.second;
			double frequency = (double)m_sampleBuffer.sampleRate() / (momentarySpectrum[j].first);
			const double amp = sqrt(frequency * (re * re + im * im)) / 200.0;
			double phase = atan2(im, re);
			if(i == 0) { phase = atan2(im, re); prevPhases[j] = phase; }
			else
			{
				const double prevPhase = prevPhases[j];
				const double currPhase = atan2(im, re);
				prevPhases[j] = currPhase;
				
				if(fabs(currPhase-prevPhase)>alpha)
				{
					if(currPhase<prevPhase)
					{
						phaseFloors[j]++;
					}
					else
					{
						phaseFloors[j]--;
					}			
				}
				phase+= 2*M_PI*phaseFloors[j];
				//TMP:debug
				if(amp>0.8)
				{
					origPhases<<std::fixed<<atan2(im,re)<<" ";
					phases<<std::fixed<<phase<<" ";
				}
				
			}		
			points.emplace_back(std::vector<double>{frequency, phase, amp});
			values.emplace_back(amp);
		}
		//determine local extrema based on amplitude
		const auto extrema = Extrema::Both(values.begin(), values.end(), 0);
		//Approximate true maxima on frequency-amplitude plane
		//IDEA: TODO: wrong phase interpolation: try on time, not frequency
		for (auto &index : extrema.second)
		{
			if (index == 0 || index == points.size() - 1)
			{
				continue;
			}
			const auto point = Approximation::Parabolic(points[index - 1][0], points[index - 1][2], points[index][0], points[index][2], points[index + 1][0], points[index + 1][2]);
			//tmp
			//Note: this interpolation seems to give good results
			//OR NOT
			//maybe between two neighbors, so not including current, just neighbors
			//conclusion: dont interpolate phasee fam
			//OR MAYBE DO, those two inbetweens can be cuz of freq change
			double phase;
			if(point.first >= points[index][0])
			{
				phase = Interpolation::Linear(points[index][0],points[index][1],points[index+1][0],points[index+1][1], point.first);
			}else{
				phase = Interpolation::Linear(points[index-1][0],points[index-1][1],points[index][0],points[index][1], point.first);
			}
			//tmp
			//points[index] = {point.first, phase, point.second};
			points[index] = {point.first, points[index][1], point.second};
			if(point.second>0.8)
				{
					intPhases<<std::fixed<<phase<<" ";
					//tmp
					std::cout<<point.first<<std::endl;
				}
		}
		//add the spectrum to the spectra
		spectra.push_back(std::move(points));
		extremaVector.push_back(std::move(extrema));
	}

	//Spectrum is now available
	// 2) interpolate phase linearly on time-phase plane
	/*std::vector<std::vector<double>> phaseApproximations(spectra.size());
	for (int i = 1; i < m_sampleBuffer.frames()-1; i++)
	{
		phaseApproximations[i].reserve(extremaVector[i].second.size());
		for(auto & index : extremaVector[i].second)
		{
			phaseApproximations[i].emplace_back(Interpolation::Linear((double)i-1, spectra[i-1][index][1], (double)i+1, spectra[i+1][index][1], (double)i));
		}
	}*/
	//TODO: revise steps and their grouping
	// 3) insert calculated phase
	//tmp: debug
	/*int bigJumps = 0;
	int allSamples = 0;
	for (int i = 1; i < m_sampleBuffer.frames()-1; i++)
	{
		int c = 0;
		for(auto & index : extremaVector[i].second)
		{
			spectra[i][index][1] = phaseApproximations[i][c];
			c++;
			allSamples++;
			if(spectra[i][index][2]>0.8)
				{
					std::cout<<std::fixed<<spectra[i][index][1]<<" ";
				}
		}
	}
	std::cout<<std::endl;*/
	int rejected = 0;
	//fit splines to spectra
	SpectrumFitter<double, 4> fitter(1.25);
	//SplineSpectrum<double> spectrum(fitter.fit(spectra, extremaVector), label);
	//auto splines = fitter.fit(spectra, extremaVector);
	for(int i = 0; i<spectra.size()-transformStep+1; i+=transformStep)
	{
		auto spline = fitter.fit(spectra[i], extremaVector[i]);
		//only add "valid" splines
		if (spline.getPeaks().size() > 0 && spline.getBegin() <= 12 && spline.getEnd() > 21000)
		{
			//tmp:debug
			/*if(i>regionStart && i<regionEnd)
			{
				std::cout<<"("<<i<<",1),";
			}*/
			inst.addSpectrum(SplineSpectrum(std::move(spline), (double)i/(double)m_sampleBuffer.sampleRate()), {label, (double)i / (double)m_sampleBuffer.sampleRate()});
		}
		else{
			
			//tmp:debug
			/*if(i>regionStart && i<regionEnd)
			{
				std::cout<<"("<<i<<",0),";
			}*/
			rejected++;
			}
	}
	
	/*std::cout<<origPhases.str()<<std::endl;
	std::cout<<phases.str()<<std::endl;
	std::cout<<intPhases.str()<<std::endl;*/
	
	//output the synthesised signal and the inverse-CWT of the signal for comparison
	//TMP: debug:
	double prev = 0;
	int counter = 0;
	auto icwt = transform.inverseTransform();
	for (int i = 0; i < icwt.size()-1; i++)
	{
		const double time = (double)i / (double)m_sampleBuffer.sampleRate();
		const double endtime = (double)(i+1) / (double)m_sampleBuffer.sampleRate();
		//TMP:Debug:
		auto spline = inst.getSpectrum({label, time});
		auto components = spline.getComponents(0);
		auto rec = synth.playNote(inst.getSpectrum({label, time}), inst.getSpectrum({label, endtime}), 1 ,i);/*NOTE: time is the first coordinate, as we interpolate on time OR MAYBE NOT*/
		oss << std::fixed << rec.front() << " " << icwt[i] << std::endl;
		//tmp: debug:
		//if(rec.front()==prev) {std::cout<<"CONSTANT VALUE at: "<<i<<std::endl;}
		if(rec.front()==prev) {counter++;}
		prev = rec.front();
	}
	//std::cout<<"Identical subsequent samples: "<<counter<<"/"<<icwt.size()<<std::endl;
	//std::cout<<"rejected splines: "<<rejected<<"/"<<icwt.size()<<std::endl;
	//std::cout<<"From transform:"<<std::endl<<origPhases.str()<<std::endl<<std::endl;
	//std::cout<<"From interpolation:"<<std::endl<<intPhases.str()<<std::endl<<std::endl;
	//std::cout<<std::endl<<"Result:"<<std::endl<<resStream.str()<<std::endl;


	//TODO: trim spectrum?
	//TODO: how to mix the output with consistent levels without clipping

	//TODO: maybe: complex b-spline?

	//tmp
	//std::cout << oss.str() << std::endl;

	//TODO: get rid of beégetett 4

	return oss.str();
}
