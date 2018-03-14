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
	HydrogenSwing(QObject *parent=0 );

	virtual ~HydrogenSwing();

	void init();

	int isInTick(MidiTime * _cur_start, const fpp_t _frames, const f_cnt_t _offset, Note * _n, Pattern * _p );

	inline virtual QString nodeName() const
	{
		return "hydrogen";
	}



	QWidget * instantiateView( QWidget * _parent );

public slots:
	// valid values are from 0 - 127
	void update();

private:
	int m_frames_per_tick;
} ;

class HydrogenSwingView : public QWidget
{
	Q_OBJECT
public:
	HydrogenSwingView(HydrogenSwing * _hy_swing, QWidget * parent=0 );
	~HydrogenSwingView();

public slots:
	void modelChanged();
	void valueChanged(int);

private:
	HydrogenSwing * m_hy_swing;
	IntModel * m_sliderModel;
	AutomatableSlider * m_slider;

} ;

#endif // HYDROGENSWING_H
