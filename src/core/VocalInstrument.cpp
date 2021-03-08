//
// Created by seledreams on 02/03/2021.
//

#include "VocalInstrument.h"
#include "Instrument.h"
VocalInstrument::VocalInstrument(VocalInstrumentTrack *_vocal_track,
								 const Descriptor *_descriptor,
								 const Descriptor::SubPluginFeatures::Key *key) : Instrument(_vocal_track,_descriptor,key)
{

}
VocalInstrument::~VocalInstrument()
{
}
