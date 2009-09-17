#ifndef FXLINE_H
#define FXLINE_H

#include <QWidget>
#include <QLabel>

#include "knob.h"
#include "SendButtonIndicator.h"

class FxMixerView;
class SendButtonIndicator;

class FxLine : public QWidget
{
	Q_OBJECT
public:
	FxLine( QWidget * _parent, FxMixerView * _mv, int _channelIndex);

	virtual void paintEvent( QPaintEvent * );
	virtual void mousePressEvent( QMouseEvent * );
	virtual void mouseDoubleClickEvent( QMouseEvent * );

	inline int channelIndex() { return m_channelIndex; }

	knob * m_sendKnob;
	SendButtonIndicator * m_sendBtn;


private:
	FxMixerView * m_mv;


	int m_channelIndex;

} ;


#endif // FXLINE_H
