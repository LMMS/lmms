
/*tmp*/
#include "CWT.hpp"
#include "Extrema.hpp"
#include "Approximation.hpp"
#include "SplineFitter.hpp"
#include "PiecewiseBSpline.hpp"
#include "SpectrumFitter.hpp"
#include "SplineSpectrum.hpp"
#include <string>
#include <sstream>
#include <iostream>

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

	/*oss<<"Magnitude spectrum:"<<std::endl;
	const auto momentarySpectrum = transform[20];
	for(int i = momentarySpectrum.size()-1;i>=0;i--){
		double re = momentarySpectrum[i].second.first;
		double im = momentarySpectrum[i].second.second;
		double scale = (double)m_sampleBuffer.sampleRate()/(momentarySpectrum[i].first);
		double modulus = sqrt(re*re + im*im); //TODO: is this correct?
		points.emplace_back(scale, modulus);
		values.push_back(modulus);
		//oss<<std::fixed<<"("<<scale<<","<<modulus<<"),";
		oss<<std::fixed<<scale<<" "<<modulus<<std::endl;
	}
	oss<<std::endl<<std::endl;*/

	/*SpectrumFitter<double, 4> fitter(2);
	SplineSpectrum<double> spectrum = fitter.fit(points);*/

	/*oss<<"Spectrum harmoics:"<<std::endl;
	for(auto h : spectrum.getHarmonics())
	{
		//oss<<"("<<h.first<<","<<h.second<<"),";
		oss<<std::fixed<<h.first<<" "<<h.second<<std::endl;
	}*/

	oss<<std::endl;
	/*oss<<"Spline evaluation:"<<std::endl;
	for(double i = 15; i<=24000; i+=5){
		const auto p = spectrum[i];
		//oss<<"("<<p.first<<","<<p.second<<"),";
		oss<<std::fixed<<p.first<<" "<<p.second<<std::endl;
	}*/

	auto icwt = transform.inverseTransform();
	std::vector<double> reconstruction(icwt.size(),0);
	 unsigned int tableSize = m_sampleBuffer.sampleRate();
    std::vector<float> sinetable(tableSize);
    for(int i = 0; i<tableSize; i++){
        sinetable[i] = (float)sin(((double)i / (double)tableSize) * M_PI * 2.0);
    }

	for(int i = 0; i<icwt.size();i++)
	{
		const auto momentarySpectrum = transform[i];
		std::vector<std::pair<double, double>> points;
		points.reserve(level*11);
		std::vector<double> values;
		for(int i = momentarySpectrum.size()-1;i>=0;i--){
			double re = momentarySpectrum[i].second.first;
			double im = momentarySpectrum[i].second.second;
			double scale = (double)m_sampleBuffer.sampleRate()/(momentarySpectrum[i].first);
			double modulus = sqrt(re*re + im*im); //TODO: is this correct?
			points.emplace_back(scale, modulus);
			values.push_back(modulus);
			//oss<<std::fixed<<"("<<scale<<","<<modulus<<"),";
			//oss<<std::fixed<<scale<<" "<<modulus<<std::endl;
		}
		SpectrumFitter<double, 4> fitter(2);
		SplineSpectrum<double> spectrum = fitter.fit(points);
		//harmonics
		for(auto h : spectrum.getHarmonics())
		{
			const unsigned int step = h.first * (tableSize / (float)m_sampleBuffer.sampleRate());
        	unsigned int pos = (i * step) % tableSize;
            reconstruction[i] += sinetable[pos] * h.second;
		}
		oss<<std::fixed<<reconstruction[i]<<" "<<icwt[i]<<std::endl;
	}
	/*for(auto s : icwt)
	{
		oss<<std::fixed<<s<<std::endl;
	}*/

	//TODO: trim spectrum?
	//TODO: normalize spectrum?

	//tmp
	std::cout<<oss.str()<<std::endl;

	return oss.str();
}
