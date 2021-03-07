//
// Created by seledreams on 02/03/2021.
//

#ifndef VOCALTRACK_H
#define VOCALTRACK_H

#include "InstrumentTrack.h"

class VocalPattern;

class LMMS_EXPORT VocalInstrumentTrack : public InstrumentTrack
{
public:
	VocalInstrumentTrack(TrackContainer *tc);

	// create new track-content-object = vocalpattern
	TrackContentObject* createTCO(const TimePos & pos) override;

	// This callback is called by events on patterns to call the rendering function of the vocal instrument;
	void bounceVocalPattern();

	// play everything in given frame-range - creates sample-play handles
	virtual bool play( const TimePos & _start, const fpp_t _frames,
					   const f_cnt_t _frame_base, int _tco_num = -1 ) override;

protected:
	QString nodeName() const override{
		return "vocaltrack";
	}
private:
	bool m_playing;

};
#endif //VOCALTRACK_H
