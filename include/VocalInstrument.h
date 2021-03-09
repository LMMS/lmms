//
// Created by seledreams on 02/03/2021.
//

#ifndef VOCALINSTRUMENT_H
#define VOCALINSTRUMENT_H
#include "Instrument.h"
#include "VocalInstrumentTrack.h"
#include "VocalPattern.h"

class VocalInstrument : public Instrument{
public:
	VocalInstrument(VocalInstrumentTrack *_vocal_track, const Descriptor * _descriptor,
					const Descriptor::SubPluginFeatures::Key * key = nullptr);
	virtual ~VocalInstrument();
	virtual bool hasNoteInput() const { return false; }
	virtual void bounceVocalPattern(VocalPattern *_pattern_to_bounce) = 0;
	virtual void bounceVocalPatterns(VocalPattern **_patterns_to_bounce,int _count) = 0;
	virtual bool isRendering() const {return m_rendering;}
protected:
	bool m_rendering;
};
#endif //VOCALINSTRUMENT_H
