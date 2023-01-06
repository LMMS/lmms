#ifndef HYDROGENSWING_H
#define HYDROGENSWING_H

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
 * A groove that mimics Hydrogen drum machine's swing feature
 */
class HydrogenSwing : public Groove
{
	Q_OBJECT
public:
	HydrogenSwing(QObject *parent = NULL);

	virtual ~HydrogenSwing();

	void init();

	int isInTick(TimePos * curStart, const fpp_t frames, const f_cnt_t offset, Note * n, MidiClip* c);

	inline virtual QString nodeName() const
	{
		return "hydrogen";
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

class HydrogenSwingView : public QWidget
{
	Q_OBJECT
public:
	HydrogenSwingView(HydrogenSwing * swing, QWidget * parent = NULL);
	~HydrogenSwingView();

public slots:
	void valueChanged();
	void modelChanged();

private:
	HydrogenSwing * m_swing;
	IntModel * m_sliderModel;
	AutomatableSlider * m_slider;

} ;

} // namespace gui

} // namespace lmms

#endif // HYDROGENSWING_H
