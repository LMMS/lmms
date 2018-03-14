#ifndef HALFSWING_H
#define HALFSWING_H

#include <QObject>

#include "AutomatableSlider.h"
#include "Groove.h"
#include "lmms_basics.h"
#include "MidiTime.h"
#include "Note.h"
#include "Pattern.h"

/**
 * A groove thatjust latter half of the HydrogenSwing algo.
 */
class HalfSwing : public QObject, public Groove
{
	Q_OBJECT
public:
	HalfSwing(QObject *parent=0 );

	virtual ~HalfSwing();

	void init();
	int amount();

	int isInTick(MidiTime * _cur_start, const fpp_t _frames, const f_cnt_t _offset, Note * _n, Pattern * _p );

	void loadSettings( const QDomElement & _this );
	void saveSettings( QDomDocument & _doc, QDomElement & _element );
	inline virtual QString nodeName() const
	{
		return "half";
	}

	QWidget * instantiateView( QWidget * _parent );

signals:
	void swingAmountChanged(int _newAmount);


public slots:
	// valid values are from 0 - 127
	void setAmount(int _amount);
	void update();

private:
	int m_frames_per_tick;
	int m_swingAmount;
	float m_swingFactor;// =  (m_swingAmount / 127)

} ;

class HalfSwingView : public QWidget
{
	Q_OBJECT
public:
	HalfSwingView(HalfSwing * _half_swing, QWidget * parent=0 );
	~HalfSwingView();

public slots:
	void modelChanged();
	void valueChanged(int);

private:
	HalfSwing * m_half_swing;
	IntModel * m_sliderModel;
	AutomatableSlider * m_slider;

} ;

#endif // HALFSWING_H
