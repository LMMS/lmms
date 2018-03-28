#ifndef HYDROGENSWING_H
#define HYDROGENSWING_H

#include <QObject>

#include "AutomatableSlider.h"
#include "Groove.h"
#include "lmms_basics.h"
#include "MidiTime.h"
#include "Note.h"
#include "Pattern.h"

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

	int isInTick(MidiTime * curStart, const fpp_t frames, const f_cnt_t offset, Note * n, Pattern * p);

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

class HydrogenSwingView : public QWidget
{
	Q_OBJECT
public:
	HydrogenSwingView(HydrogenSwing * swing, QWidget * parent = NULL);
	~HydrogenSwingView();

public slots:
	void modelChanged();
	void valueChanged(int value);

private:
	HydrogenSwing * m_swing;
	IntModel * m_sliderModel;
	AutomatableSlider * m_slider;

} ;

#endif // HYDROGENSWING_H
