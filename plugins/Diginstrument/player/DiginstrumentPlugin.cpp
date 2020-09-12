#include "DiginstrumentPlugin.h"

using namespace QtDataVisualization;

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

void DiginstrumentPlugin::loadSettings(const QDomElement &_this)
{
	setInstrumentFile(_this.attribute("fileName"));
	loadInstrumentFile();
}

void DiginstrumentPlugin::saveSettings(QDomDocument &_doc, QDomElement &_this)
{
	_this.setAttribute("fileName", fileName.c_str());
}

QString DiginstrumentPlugin::nodeName() const
{
	return "TEST";
}

void DiginstrumentPlugin::playNote(NotePlayHandle *noteHandle,
								   sampleFrame *_working_buf)
{
	/*TMP*/
	//TMP: TODO: BAD spectrum type selection, BAD!
	const double startTime = noteHandle->totalFramesPlayed() / (double)Engine::mixer()->processingSampleRate();
	vector<float> audioData;
	vector<double> coordinates = {noteHandle->frequency()};
	coordinates.reserve(this->coordinates.size()+2);
	for(auto c : this->coordinates)
	{
		coordinates.emplace_back(c);
	}
	coordinates.emplace_back(startTime);
	if(inst_data.type == "discrete")
	{
		auto spectrum = inst.getSpectrum(coordinates);
		//TODO: maybe should pass spectrum, not the components
		audioData = this->synth.playNote(spectrum.getComponents(0), noteHandle->framesLeftForCurrentPeriod(), noteHandle->totalFramesPlayed(), /*tmp*/ 44100);
	}
	else
	{
		auto spectrum = spline_inst.getSpectrum(coordinates);
		audioData = this->synth.playNote(spectrum.getComponents(0), noteHandle->framesLeftForCurrentPeriod(), noteHandle->totalFramesPlayed(), /*tmp*/ 44100);
	}
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

bool DiginstrumentPlugin::setInstrumentFile(const QString & fileName)
{
	this->fileName = fileName.toStdString();
	return true;
}

/**
 * Construct the instrument from the file given with setInstrumentFile()
 **/
//TODO: spline spectrum type
//TODO: what fields to populate, information to display
//TODO: actually finish discrete type loading
//TODO: possible refactoring after multiple supported types
bool DiginstrumentPlugin::loadInstrumentFile()
{
	QFile file(QString{fileName.c_str()});
	if(file.open(QIODevice::ReadOnly))
	{
		QByteArray arr = file.readAll();
		file.close();

		//tmp
		//TODO: label
		//TODO: separate into loading from file and loading saved
		//TODO: catch?
		inst_data._json = json::parse(arr.toStdString());
		inst_data.name = inst_data._json["name"];
		inst_data.type = inst_data._json["spectrum_type"];
		//TODO: better spectrum type separation!
		inst.clear();
		spline_inst.clear();
		std::vector<Diginstrument::Dimension> dimensions;
		std::vector<string> dimension_labels;
		for(auto d : inst_data._json["dimensions"])
		{
			if(!d["default"].is_null())
			{
				dimensions.emplace_back(d["label"],d["min"],d["max"],d["shifting"], d["default"]);
			}
			else
			{
				dimensions.emplace_back(d["label"],d["min"],d["max"],d["shifting"]);
			}
			dimension_labels.emplace_back(d["label"]);
		}
		//TODO: better spectrum type separation!
		inst.setDimensions(dimensions);
		spline_inst.setDimensions(dimensions);
		//TODO: actual dynamic loading/parsing
		//TODO: actually use "coordinates"
		//TODO: better spline/discrete separation PLEASE!
		for(auto s : inst_data._json["spectra"])
		{
			vector<double> spectrum_coordinates;
			for(string label : dimension_labels)
			{
				spectrum_coordinates.emplace_back(s[label]);
			}
			if(inst_data.type == "discrete")
			{
				std::vector<Diginstrument::Component<double>> components;
				for(auto c : s["components"])
				{
					components.push_back({c[0], 0, c[1]});
				}
				//TODO: dynamic label
				Diginstrument::NoteSpectrum<double> spectrum{s[dimension_labels.back()], components, {}};
				inst.addSpectrum(spectrum, spectrum_coordinates);
			}
			if(inst_data.type == "spline")
			{
				PiecewiseBSpline<double, 4> piecewise;
				for(auto piece : s["pieces"])
					{
					std::vector<std::vector<double>> controlPoints;
					controlPoints.reserve(piece["control_points"].size());
					std::vector<double> knotVector;
					knotVector.reserve(piece["knot_vector"].size());
					for( auto cp : piece["control_points"] )
					{
						controlPoints.push_back(cp);
					}
					for( auto knot : piece["knot_vector"] )
					{
						knotVector.push_back(knot);
					}
					BSpline<double, 4> spline;
					spline.setKnotVector(knotVector);
					spline.setControlPoints(controlPoints);
					piecewise.add(spline);
				}
				SplineSpectrum<double, 4> spectrum{piecewise, s[dimension_labels.back()]};
				spline_inst.addSpectrum(spectrum, spectrum_coordinates);
			}

		}
		return true;
	}
	else return false;
}

QtDataVisualization::QSurfaceDataArray * DiginstrumentPlugin::getInstrumentSurfaceData(float minTime, float maxTime, float minFreq, float maxFreq, int timeSamples, int freqSamples, std::vector<double> coordinates)
{
	//TODO: better/refactoring
	coordinates.push_back(0);

	const float stepX = (maxFreq - minFreq) / float(freqSamples - 1);
    const float stepZ = (maxTime - minTime) / float(timeSamples - 1);

	QSurfaceDataArray * data = new QSurfaceDataArray;
	data->reserve(timeSamples);
	for(int i = 0; i<timeSamples;i++)
	{
		QSurfaceDataRow *dataRow = new QSurfaceDataRow(freqSamples);
		float z = qMin(maxTime, (i * stepZ + minTime));
		coordinates.back() = z;

		//TODO: better spectrum type distinction, please!
		//discrete instrument
		if(inst_data.type == "discrete")
		{
			//initialize row to zeroes
			int index = 0;
			for (int j = 0; j < freqSamples; j++) {
				float x = qMin(maxFreq, (j * stepX + minFreq));
				(*dataRow)[index++].setPosition(QVector3D(x, 0, z));
			}
			//project components into the row based on frequency
			for(const auto & c : inst.getSpectrum(coordinates).getComponents(0))
			{
				if(c.frequency<minFreq || c.frequency>maxFreq) continue;
				//TODO: BUG: while sliding towards the end, this can be out of range
				(*dataRow)[std::round((c.frequency-minFreq)/((maxFreq-minFreq)/(float)freqSamples))].setPosition(QVector3D(c.frequency,c.amplitude, z));
			}
		}
		//spline instrument
		if(inst_data.type == "spline")
		{
			int index = 0;
			const auto spectrum = spline_inst.getSpectrum(coordinates);
			for (int j = 0; j < freqSamples; j++) {
				float x = qMin(maxFreq, (j * stepX + minFreq));
				(*dataRow)[index++].setPosition(QVector3D(x, spectrum[x].amplitude, z));
			}
			//tmp: identical to discrete
			for(const auto & c : spectrum.getComponents(0))
			{
				if(c.frequency<=minFreq || c.frequency>=maxFreq) continue;
				(*dataRow)[std::round((c.frequency-minFreq)/((maxFreq-minFreq)/(float)freqSamples))].setPosition(QVector3D(c.frequency,c.amplitude, z));
			}
			//TODO: should i visualize peaks explicitly? (similarly to discrete above)
		}
		*data<<dataRow;
	}

	return data;
}
