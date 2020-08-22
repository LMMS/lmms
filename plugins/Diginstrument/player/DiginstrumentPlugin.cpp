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
	if(inst_data.type == "discrete")
	{
		auto spectrum = inst.getSpectrum({noteHandle->frequency(), startTime});
		audioData = this->synth.playNote(spectrum, noteHandle->framesLeftForCurrentPeriod(), noteHandle->totalFramesPlayed(), /*tmp*/ 44100);
	}
	else
	{
		auto spectrum = spline_inst.getSpectrum({noteHandle->frequency(), startTime});
		audioData = this->synth.playNote(spectrum, noteHandle->framesLeftForCurrentPeriod(), noteHandle->totalFramesPlayed(), /*tmp*/ 44100);
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
		inst.clear();
		std::vector<pair<string,bool>> dimensions;
		std::vector<string> dimension_labels;
		for(auto d : inst_data._json["dimensions"])
		{
			dimensions.emplace_back(d["label"],d["shifting"]);
			dimension_labels.emplace_back(d["label"]);
		}
		inst.setDimensions(dimensions);
		//TODO: actual dynamic loading/parsing
		//TODO: actually use "coordinates"
		for(auto s : inst_data._json["spectra"])
		{
			std::vector<Diginstrument::Component<double>> components;
			for(auto c : s["components"])
			{
				components.push_back({c[0], 0, c[1]});
			}
			//TODO: dynamic label
			Diginstrument::NoteSpectrum<double> spectrum{s[dimension_labels.back()], components, {}};
			vector<double> spectrum_coordinates;
			for(string label : dimension_labels)
			{
				spectrum_coordinates.emplace_back(s[label]);
			}
			inst.addSpectrum(spectrum, spectrum_coordinates);
		}
		return true;
	}
	else return false;
}
