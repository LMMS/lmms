/*
 * Slicer.cpp - instrument which uses a usereditable wavetable
 *
 * Copyright (c) 2006-2008 Andreas Brandmaier <andy/at/brandmaier/dot/de>
 * 
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


// #include <QDomElement>
#include <stdio.h>
#include "SlicerT.h"


// #include "AudioEngine.h"
// #include "base64.h"
// #include "Engine.h"
// #include "Graph.h"
#include "InstrumentTrack.h"
// #include "Knob.h"
// #include "LedCheckBox.h"
// #include "NotePlayHandle.h"
// #include "PixmapButton.h"
// #include "Song.h"
// #include "interpolation.h"


#include "embed.h"
#include "plugin_export.h"




namespace lmms
{



extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT slicert_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"SlicerT",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"A cool testPlugin" ),
	"Daniel Kauss Serna>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	nullptr,
	nullptr,
} ;

}



SlicerT::SlicerT(InstrumentTrack * _instrument_track) : 
	Instrument( _instrument_track, &slicert_plugin_descriptor ),
	m_sampleBuffer(),
	noteThreshold( 0.2f, 0.0f, 1.0f, 0.01f, this, tr( "Note Threshold" ) )
{
	// connect( &noteThreshold, SIGNAL( dataChanged() ), this, SLOT( updateParams() ) );
	printf("Correctly loaded SlicerT!\n");
}


void SlicerT::findSlices() {
	int c = 0;
	float lastAvg = 0;
	float currentAvg = 0;
	slicePoints = {};
	for (int i = 0; i<m_sampleBuffer.endFrame();i+=1) {
		// combine left and right and absolute it
		float sampleValue = abs(m_sampleBuffer.data()[i][0]) + abs(m_sampleBuffer.data()[i][1]) / 2;
		currentAvg += sampleValue;

		if (i%128==0) {
			currentAvg /= 128.0f;
			//printf("%i -> %f : %f\n", i, currentAvg, lastAvg);
			if (abs(currentAvg- lastAvg) > noteThreshold.value()) {
				c++;
				slicePoints.push_back(i);
			}
			lastAvg = currentAvg;
			currentAvg = 0;
		}

	}
	// for (int i : slicePoints) {
	// 	//printf("%i\n", i);
	// }
	
	printf("Found %i notes\n", c);
	emit dataChanged();
}

// void SlicerT::updateParams() {

// }

void SlicerT::playNote( NotePlayHandle * _n, sampleFrame * _working_buffer ) {
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	// init NotePlayHandle data
	if( !_n->m_pluginData )
	{
		frameCounter = 0;
		_n->m_pluginData = new SampleBuffer::handleState( false, SRC_LINEAR );

		findSlices();

	}

	// if play returns true (success I guess)
	if( m_sampleBuffer.play( _working_buffer + offset,
				(SampleBuffer::handleState *)_n->m_pluginData,
				frames, _n->frequency(),
				static_cast<SampleBuffer::LoopMode>( 0 ) ) )
	{
		frameCounter += frames + offset;
		// for (int i = 0;i<256;i++) {
		// 	printf(">  %f : %f", _working_buffer[i][0], _working_buffer[i][1]);
		// 	printf("\n");
		// }
		// printf("%lu : %lu", sizeof(_working_buffer), sizeof(_working_buffer[0]));
		// printf("\n");
		// add the buffer, and then process ???? maybe
		applyRelease( _working_buffer, _n );
		instrumentTrack()->processAudioBuffer( _working_buffer,
								frames + offset, _n );

	}



	// testing
	// sampleFrame testFrame;
	// printf("starting print");
	// m_sampleBuffer.play(&testFrame, new SampleBuffer::handleState( false, SRC_LINEAR ), 2, _n->frequency(), static_cast<SampleBuffer::LoopMode>( 0 ));



}

void SlicerT::updateFile(QString file) {
	printf("updated audio file");
	m_sampleBuffer.setAudioFile(file);
	findSlices();
}


void SlicerT::saveSettings(QDomDocument & _doc, QDomElement & _parent) {}
void SlicerT::loadSettings( const QDomElement & _this ) {}

QString SlicerT::nodeName() const
{
	return( slicert_plugin_descriptor.name );
}

gui::PluginView * SlicerT::instantiateView( QWidget * _parent )
{
	return( new gui::SlicerTUI( this, _parent ) );
}



extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	
	return( new SlicerT( static_cast<InstrumentTrack *>( m ) ) );
}


}


} // namespace lmms
