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
	const double startTime = noteHandle->totalFramesPlayed() / (double)Engine::mixer()->processingSampleRate();
	vector<double> coordinates = {noteHandle->frequency()};
	//TODO: first coordinate is freq, might not be correct?
	coordinates.reserve(this->coordinates.size()+2);
	for(auto c : this->coordinates)
	{
		coordinates.emplace_back(c);
	}
	auto partials = interpolator.getPartials(coordinates, noteHandle->totalFramesPlayed(), noteHandle->framesLeftForCurrentPeriod());
	coordinates.emplace_back(startTime);
	auto spectrum = interpolator.getSpectrum(coordinates);
	vector<float> audioData = this->synth.playNote(partials, noteHandle->framesLeftForCurrentPeriod(), noteHandle->totalFramesPlayed(), /*TMP*/ 44100);

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
//TODO: what fields to populate, information to display
bool DiginstrumentPlugin::loadInstrumentFile()
{
	QFile file(QString{fileName.c_str()});
	if(file.open(QIODevice::ReadOnly))
	{
		QByteArray arr = file.readAll();
		file.close();
		//TODO: separate into loading from file and loading saved
		//TODO: catch?
		instrument = Diginstrument::Instrument<SplineSpectrum<double, 4>, double>::fromJSON(json::parse(arr.toStdString()));
		//tmp:
		interpolator.clear();
		interpolator.addSpectra(instrument.getSpectra());
		interpolator.addPartialSets(instrument.getPartialSets());
		interpolator.setDimensions(instrument.dimensions);
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
		
		int index = 0;
		const auto spectrum = interpolator.getSpectrum(coordinates);
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
		*data<<dataRow;
	}

	return data;
}
