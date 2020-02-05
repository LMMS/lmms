
/*tmp*/
#include "CWT.hpp"
#include "Extrema.hpp"
#include "Approximation.hpp"
#include "SplineFitter.hpp"
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
	const int level = 12;
	CWT transform("morlet", 6, level);
	transform(sample);

	std::ostringstream oss;
	std::vector<std::pair<double, double>> points;
	points.reserve(level*11);
	std::vector<double> values;

	oss<<"Magnitude spectrum:"<<std::endl;
	for(auto e : transform[20]){
		double re = e.second.first;
		double im = e.second.second;
		double scale = e.first;
		double modulus = sqrt(re*re + im*im); //TODO: is this correct?
		points.emplace_back(scale, modulus);
		values.push_back(modulus);
		oss<<std::fixed<<scale<<" "<<modulus<<std::endl;
	}
	oss<<std::endl<<std::endl;

	std::pair<std::vector<unsigned int>, std::vector<unsigned int>> extrema = Extrema::Both(values.begin(), values.end(), 0);

	oss<<"Minima:"<<std::endl;
	for(int m : extrema.first){
		oss<<std::fixed<<points[m].first<<" "<<points[m].second<<std::endl;
	}
	oss<<std::endl;
	oss<<"Minima approximation:"<<std::endl;
	for(int m : extrema.first){
		if(m>0 && m<=points.size()-2){
			auto apr = Approximation::Parabolic(points[m-1].first, points[m-1].second, points[m].first, points[m].second, points[m+1].first, points[m+1].second);
			oss<<std::fixed<<apr.first<<" "<<apr.second<<std::endl;
			continue;
		}
	}
	oss<<std::endl;
	oss<<"Maxima:"<<std::endl;
	for(int m : extrema.second){
		oss<<std::fixed<<points[m].first<<" "<<points[m].second<<std::endl;
	}
	oss<<std::endl;
	oss<<"Maxima approximation:"<<std::endl;
	for(int m : extrema.second){
		if(m>0 && m<=points.size()-2){
			auto apr = Approximation::Parabolic(points[m-1].first, points[m-1].second, points[m].first, points[m].second, points[m+1].first, points[m+1].second);
			oss<<std::fixed<<apr.first<<" "<<apr.second<<std::endl;
			continue;
		}
	}

	BSpline<double, 4> spline = SplineFitter<double, 4>::fit({
		std::pair<double, double>(0,7),
		std::pair<double, double>(0.25,5),
		std::pair<double, double>(0.45,4.5),
		std::pair<double, double>(0.55,4.3),
		std::pair<double, double>(0.65,4.1),
		std::pair<double, double>(0.7,4),
		std::pair<double, double>(0.75,4.2),
		std::pair<double, double>(0.85,5),
		std::pair<double, double>(0.9,6),
		std::pair<double, double>(0.95,8),
		std::pair<double, double>(1,12)
	}, 7);

	oss<<std::endl;
	oss<<"Spline evaluation:"<<std::endl;
	for(double i = 0; i<=1; i+=0.01){
		const auto p = spline[i];
		oss<<"("<<p.first<<","<<p.second<<"),";
	}
	const auto p = spline[1];
	oss<<"("<<p.first<<","<<p.second<<"),";

	//TODO: normalize?
	//TODO: trim?
	//DONE: true peak
	//TODO: normalize here?
	//TODO: merged b-spline

	return oss.str();
}
