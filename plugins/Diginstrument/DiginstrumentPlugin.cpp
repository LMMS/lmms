
/*tmp*/
#include "CWT.hpp"
#include <string>
#include <sstream>

#include "DiginstrumentPlugin.h"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT diginstrument_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Diginstrument",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"WIP"
				"Test" ),
	"Máté Szokolai",
	0x0110,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
};

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model* model, void * )
{
	return new DiginstrumentPlugin( static_cast<InstrumentTrack *>( model ) );
}

}

DiginstrumentPlugin::DiginstrumentPlugin( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &diginstrument_plugin_descriptor )
{
    /*TODO */
	synth.setSampleRate(Engine::mixer()->processingSampleRate());
}

DiginstrumentPlugin::~DiginstrumentPlugin(){}

PluginView * DiginstrumentPlugin::instantiateView( QWidget * _parent )
{
	return new DiginstrumentView( this, _parent );
}

void DiginstrumentPlugin::loadSettings( const QDomElement & _this ){}
void DiginstrumentPlugin::saveSettings( QDomDocument & _doc, QDomElement & _parent ){}

QString DiginstrumentPlugin::nodeName() const{
    return "TEST";
}

void DiginstrumentPlugin::playNote( NotePlayHandle * noteHandle,
					            sampleFrame * _working_buf )
{
	/*TMP*/
	auto audioData = this->synth.playNote({noteHandle->frequency()}, noteHandle->framesLeftForCurrentPeriod(), noteHandle->totalFramesPlayed());
	/*tmp: stereo*/
	unsigned int counter = 0;
	unsigned int offset = noteHandle->noteOffset();
	for (auto frame : audioData){
		_working_buf[counter + offset][0] = _working_buf[counter + offset][1] = frame;
		counter++;
	}
	applyRelease( _working_buf, noteHandle );
	instrumentTrack()->processAudioBuffer( _working_buf, audioData.size() + noteHandle->noteOffset(), noteHandle );
}

void DiginstrumentPlugin::deleteNotePluginData( NotePlayHandle * _note_to_play )
{

}

f_cnt_t DiginstrumentPlugin::beatLen( NotePlayHandle * _n ) const
{
    return 0;
}

QString DiginstrumentPlugin::fullDisplayName() const
{
    return "TEST";
}

void DiginstrumentPlugin::sampleRateChanged(){
	/*TODO*/
	this->synth.setSampleRate(Engine::mixer()->processingSampleRate());
}


//TMP
std::string DiginstrumentPlugin::setAudioFile( const QString & _audio_file)
{	
	m_sampleBuffer.setAudioFile( _audio_file );
	std::vector<double> sample(m_sampleBuffer.frames());
	for(int i = 0; i<sample.size(); i++){
		//tmp: left only
		sample[i] = m_sampleBuffer.data()[i][0];
	}
	CWT transform("morlet", 6, 12);
	transform(sample);

	std::ostringstream oss;

	for(auto &e : transform[20]){
		double re = e.second.first;
		double im = e.second.second;
		double scale = e.first;
		double modulus = sqrt(re*re + im*im); //TODO: is this correct?
		oss<<std::fixed<<"("<<log10(44100.0f/scale)<<","<<modulus<<"),";
		
	}
	return oss.str();
}
