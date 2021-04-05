//
// Created by seledreams on 02/03/2021.
//

#ifndef VOCALTRACK_H
#define VOCALTRACK_H

#include <QTimer>
#include "InstrumentTrack.h"
#include <thread>

class VocalPattern;

class LMMS_EXPORT VocalInstrumentTrack : public InstrumentTrack
{
public:
	VocalInstrumentTrack(TrackContainer *tc);

	// create new track-content-object = vocalpattern
	TrackContentObject* createTCO(const TimePos & pos) override;

	// This callback is called by events on patterns to call the rendering function of the vocal instrument;
	void bounceVocalPattern();
	public slots:
	void bounceVocalPatterns();
	// play everything in given frame-range - creates sample-play handles
	virtual bool play( const TimePos & _start, const fpp_t _frames,
					   const f_cnt_t _frame_base, int _tco_num = -1 );


protected:
	QString nodeName() const override{
		return "vocaltrack";
	}
	bool getTCOsToPlay();
	bool setupTCO(VocalPattern *_vocal_pattern);
	bool addPlayHandles(const TimePos &_offset);
private:
	QTimer refresh_rate;
	TimePos m_start;
	tcoVector m_tcos;
	int m_tco_num;
	std::thread *current_rendering;
	bool m_rendering;
	bool m_playing;

};
#endif //VOCALTRACK_H
