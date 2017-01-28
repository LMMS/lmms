#ifndef GROOVEEXPERIMENTS_H
#define GROOVEEXPERIMENTS_H

#include <QtCore/QObject>

#include "Groove.h"
#include "Knob.h"
#include "lmms_basics.h"
#include "MidiTime.h"
#include "Note.h"
#include "Pattern.h"

/**
 * A groove thats new
 */
class GrooveExperiments : public QObject, public Groove
{
	Q_OBJECT
public:
	GrooveExperiments(QObject *parent=0 );

	virtual ~GrooveExperiments();

	void init();
	int amount();

	int isInTick(MidiTime * _cur_start, const fpp_t _frames, const f_cnt_t _offset, Note * _n, Pattern * _p );

	void loadSettings( const QDomElement & _this );
	void saveSettings( QDomDocument & _doc, QDomElement & _element );
	inline virtual QString nodeName() const
	{
		return "experiment";
	}



	QWidget * instantiateView( QWidget * _parent );

signals:
	void shiftAmountChanged(int _newAmount);


public slots:
	// valid values are from 0 - 127
	void setAmount(int _amount);
	void update();

private:
	int m_frames_per_tick;
	int m_shiftAmount;
	float m_shiftFactor;// =  (m_shiftAmount / 127.0)

} ;

class GrooveExperimentsView : public QWidget
{
	Q_OBJECT
public:
	GrooveExperimentsView(GrooveExperiments * _m_ge, QWidget * parent=0 );
	~GrooveExperimentsView();

public slots:
	void modelChanged();
	void valueChanged(float);

private:
	GrooveExperiments * m_ge;
	FloatModel * m_nobModel;
	Knob * m_nob;

} ;

#endif // GROOVEEXPERIMENTS_H
