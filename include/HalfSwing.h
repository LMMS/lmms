#ifndef HALFSWING_H
#define HALFSWING_H

#include <QObject>

#include "AutomatableSlider.h"
#include "Groove.h"
#include "lmms_basics.h"
#include "TimePos.h"
#include "Note.h"
#include "MidiClip.h"

namespace lmms
{

/**
 * A groove thatjust latter half of the HydrogenSwing algo.
 */
class HalfSwing : public Groove
{
	Q_OBJECT
public:
	HalfSwing(QObject *parent = NULL);

	virtual ~HalfSwing();

	void init();

	int isInTick(TimePos * curStart, const fpp_t frames, const f_cnt_t offset, Note * n, MidiClip* c);

	inline virtual QString nodeName() const
	{
		return "half";
	}

	QWidget * instantiateView(QWidget * parent);

public slots:
	// valid values are from 0 - 127
	void update();

private:
	int m_framesPerTick;
} ;


namespace gui
{

class HalfSwingView : public QWidget
{
	Q_OBJECT
public:
	HalfSwingView(HalfSwing * halfSwing, QWidget * parent = NULL);
	~HalfSwingView();

public slots:
	void valueChanged();
	void modelChanged();

private:
	HalfSwing * m_swing;
	IntModel * m_sliderModel;
	AutomatableSlider * m_slider;

} ;

} // namespace gui

} // namespace lmms

#endif // HALFSWING_H
