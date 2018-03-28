#ifndef GROOVEEXPERIMENTS_H
#define GROOVEEXPERIMENTS_H

#include <QObject>

#include "AutomatableSlider.h"
#include "Groove.h"
#include "lmms_basics.h"
#include "MidiTime.h"
#include "Note.h"
#include "Pattern.h"

/**
 * A groove thats new
 */
class GrooveExperiments : public Groove
{
	Q_OBJECT
public:
	GrooveExperiments(QObject *parent = NULL);

	virtual ~GrooveExperiments();

	void init();

	int isInTick(MidiTime * curStart, const fpp_t frames, const f_cnt_t offset, Note * n, Pattern * p);

	inline virtual QString nodeName() const
	{
		return "experiment";
	}



	QWidget * instantiateView(QWidget * parent);

public slots:
	// valid values are from 0 - 127
	void update();

private:
	int m_framesPerTick;
} ;

class GrooveExperimentsView : public QWidget
{
	Q_OBJECT
public:
	GrooveExperimentsView(GrooveExperiments * groove, QWidget * parent = NULL);
	~GrooveExperimentsView();

public slots:
	void modelChanged();
	void valueChanged(int);

private:
	GrooveExperiments * m_groove;
	IntModel * m_sliderModel;
	AutomatableSlider * m_slider;

} ;

#endif // GROOVEEXPERIMENTS_H
