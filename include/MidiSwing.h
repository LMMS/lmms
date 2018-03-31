#ifndef MIDISWING_H
#define MIDISWING_H

#include <QObject>

#include "Groove.h"
#include "lmms_basics.h"
#include "MidiTime.h"
#include "Note.h"
#include "Pattern.h"

/*
 * A swing groove that adjusts by whole ticks.
 * Someone might like it, also might be able to save the output to a midi file later.
 */
class MidiSwing : public Groove
{
	Q_OBJECT
public:
	MidiSwing(QObject * parent = NULL);

	~MidiSwing();

	// TODO why declaring this should it not come from super class?
	int isInTick(MidiTime * cur_start, const fpp_t frames, const f_cnt_t offset, Note * n, Pattern * p);
	int isInTick(MidiTime * curStart, Note * n, Pattern * p);

	inline virtual QString nodeName() const
	{
		return "midi";
	}

	QWidget * instantiateView(QWidget * parent);

};

#endif // MIDISWING_H
