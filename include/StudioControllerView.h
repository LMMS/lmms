

#ifndef STUDIOCONTROLLERVIEW_H
#define STUDIOCONTROLLERVIEW_H

#include <QWidget>
#include <QCloseEvent>

#include <QComboBox>
#include <QVBoxLayout>

#include "SerializingObject.h"
#include "AutomatableControlButton.h"

class StudioControllerView : public QWidget
{
	Q_OBJECT
public:
	StudioControllerView();
	virtual ~StudioControllerView();

signals:

public slots:
	void controllerChanged(int index);
	void doHome();
	void doPlay();
	void doStop();
	void doRecord();
	void doNext();
	void doPrev();
	void doScroll();
	void saveControllers();
	void loadControllers();

private:
	float m_scrollLast;

	QVBoxLayout * m_layout;
	QComboBox * m_dropDown;
	QLabel * m_controllerLabel;
	QLabel * m_actionsLabel;
	AutomatableControlButton * m_homeButton;
	AutomatableControlButton * m_playButton;
	AutomatableControlButton * m_stopButton;
	AutomatableControlButton * m_recordButton;
	AutomatableControlButton * m_nextButton;
	AutomatableControlButton * m_prevButton;
	AutomatableControlButton * m_scrollButton;
	AutomatableControlButton * m_saveButton;
};

#endif // STUDIOCONTROLLERVIEW_H
