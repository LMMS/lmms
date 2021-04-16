#ifndef SAMPLE_TRACK_VIEW_H
#define SAMPLE_TRACK_VIEW_H

#include "SampleTrack.h"

#include "TrackView.h"

class Knob;
class SampleTrack;
class SampleTrackWindow;
class TrackLabelButton;


class SampleTrackView : public TrackView
{
	Q_OBJECT
public:
	SampleTrackView( SampleTrack* Track, TrackContainerView* tcv );
	virtual ~SampleTrackView();

	SampleTrackWindow * getSampleTrackWindow()
	{
		return m_window;
	}

	SampleTrack * model()
	{
		return castModel<SampleTrack>();
	}

	const SampleTrack * model() const
	{
		return castModel<SampleTrack>();
	}


	QMenu * createFxMenu( QString title, QString newFxLabel ) override;


public slots:
	void showEffects();
	void updateIndicator();


protected:
	void modelChanged() override;
	QString nodeName() const override
	{
		return "SampleTrackView";
	}

	void dragEnterEvent(QDragEnterEvent *dee) override;
	void dropEvent(QDropEvent *de) override;

private slots:
	void assignFxLine( int channelIndex );
	void createFxLine();


private:
	SampleTrackWindow * m_window;
	Knob * m_volumeKnob;
	Knob * m_panningKnob;
	FadeButton * m_activityIndicator;

	TrackLabelButton * m_tlb;

	FadeButton * getActivityIndicator() override
	{
		return m_activityIndicator;
	}

	friend class SampleTrackWindow;
} ;



#endif