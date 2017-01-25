#ifndef HYDROGENSWING_H
#define HYDROGENSWING_H

#include <QtCore/QObject>

#include "Groove.h"
#include "Knob.h"
#include "lmms_basics.h"
#include "MidiTime.h"
#include "Note.h"
#include "Pattern.h"

/**
 * A groove that mimics Hydrogen drum machine's swing feature
 */
class HydrogenSwing : public QObject, public Groove
{
	Q_OBJECT
public:
	HydrogenSwing(QObject *parent=0 );

	virtual ~HydrogenSwing();

	void init();
	int amount();

	int isInTick(MidiTime * _cur_start, const fpp_t _frames, const f_cnt_t _offset, Note * _n, Pattern * _p );

	void loadSettings( const QDomElement & _this );
	void saveSettings( QDomDocument & _doc, QDomElement & _element );
	inline virtual QString nodeName() const
	{
		return "hydrogen";
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
	float m_swingFactor;// =  (m_swingAmount / 127.0)

} ;

class HydrogenSwingView : public QWidget
{
	Q_OBJECT
public:
	HydrogenSwingView(HydrogenSwing * _hy_swing, QWidget * parent=0 );
	~HydrogenSwingView();

public slots:
	void valueChanged(float);

private:
	HydrogenSwing * m_hy_swing;
	FloatModel * m_nobModel;
	Knob * m_nob;

} ;

#endif // HYDROGENSWING_H
