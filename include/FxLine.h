#ifndef FXLINE_H
#define FXLINE_H

#include <QWidget>
#include <QLabel>

#include "knob.h"
#include "lcd_spinbox.h"
#include "SendButtonIndicator.h"

class FxMixerView;
class SendButtonIndicator;

class FxLine : public QWidget
{
	Q_OBJECT
public:
	FxLine( QWidget * _parent, FxMixerView * _mv, int _channelIndex);
	~FxLine();

	virtual void paintEvent( QPaintEvent * );
	virtual void mousePressEvent( QMouseEvent * );
	virtual void mouseDoubleClickEvent( QMouseEvent * );

	inline int channelIndex() { return m_channelIndex; }
	void setChannelIndex(int index);

	knob * m_sendKnob;
	SendButtonIndicator * m_sendBtn;

private:
	FxMixerView * m_mv;
	lcdSpinBox * m_lcd;


	int m_channelIndex;

} ;


#endif // FXLINE_H
