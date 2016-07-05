//
// Created by Nicholas Wertzberger on 7/4/16.
//

#ifndef LMMS_BBTRACKCONTAINERVIEW_H
#define LMMS_BBTRACKCONTAINERVIEW_H

#include "TrackContainerView.h"
#include "BBTrackContainer.h"

class BBTrackContainerView : public TrackContainerView
{
	Q_OBJECT
public:
	BBTrackContainerView(BBTrackContainer* tc);

	bool fixedTCOs() const
	{
		return true;
	}

	void removeBBView(int bb);

	void saveSettings(QDomDocument& doc, QDomElement& element);
	void loadSettings(const QDomElement& element);

public slots:
	void addSteps();
	void cloneSteps();
	void removeSteps();
	void addAutomationTrack();

protected slots:
	void dropEvent(QDropEvent * de );
	void updatePosition();

private:
	BBTrackContainer * m_bbtc;
	void makeSteps( bool clone );
};

#endif //LMMS_BBTRACKCONTAINERVIEW_H
