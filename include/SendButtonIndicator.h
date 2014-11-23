#ifndef SENDBUTTONINDICATOR_H
#define SENDBUTTONINDICATOR_H

#include <QDebug>
#include <QtGui/QLabel>
#include <QtGui/QPixmap>

#include "FxLine.h"
#include "FxMixerView.h"

class FxLine;
class FxMixerView;

class SendButtonIndicator : public QLabel 
{
public:
	SendButtonIndicator( QWidget * _parent, FxLine * _owner,
						 FxMixerView * _mv);

	virtual void mousePressEvent( QMouseEvent * e );
	void updateLightStatus();

private:

	FxLine * m_parent;
	FxMixerView * m_mv;
	static QPixmap * s_qpmOn;
	static QPixmap * s_qpmOff;

	FloatModel * getSendModel();
};

#endif // SENDBUTTONINDICATOR_H
