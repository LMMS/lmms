

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
	void doStop();
	void doPlay();
	void doNext();
	void doPrev();
	void doRecord();
	void doScroll();
	void saveControllers();
	void loadControllers();

private:
	float m_scrollLast;
	
	QComboBox * m_dropDown;
	QVBoxLayout * m_layout;
	AutomatableControlButton  * m_homeButton;
	AutomatableControlButton  * m_stopButton;
	AutomatableControlButton  * m_playButton;
	AutomatableControlButton  * m_recordButton;
	AutomatableControlButton  * m_scrollButton;
	AutomatableControlButton  * m_nextButton;
	AutomatableControlButton  * m_prevButton;
	AutomatableControlButton  * m_saveButton;

};

#endif // STUDIOCONTROLLERVIEW_H
