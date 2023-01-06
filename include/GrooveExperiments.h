#ifndef GROOVEEXPERIMENTS_H
#define GROOVEEXPERIMENTS_H

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
 * A groove thats new
 */
class GrooveExperiments : public Groove
{
	Q_OBJECT
public:
	GrooveExperiments(QObject *parent = NULL);

	virtual ~GrooveExperiments();

	void init();

	int isInTick(TimePos * curStart, const fpp_t frames, const f_cnt_t offset, Note * n, MidiClip* c);

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


namespace gui
{

class GrooveExperimentsView : public QWidget
{
	Q_OBJECT
public:
	GrooveExperimentsView(GrooveExperiments * groove, QWidget * parent = NULL);
	~GrooveExperimentsView();

public slots:
	void valueChanged();
	void modelChanged();

private:
	GrooveExperiments * m_groove;
	IntModel * m_sliderModel;
	AutomatableSlider * m_slider;

} ;

} // namespace gui

} // namespace lmms

#endif // GROOVEEXPERIMENTS_H
