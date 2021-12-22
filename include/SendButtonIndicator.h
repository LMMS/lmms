#ifndef SENDBUTTONINDICATOR_H
#define SENDBUTTONINDICATOR_H

#include <QDebug>
#include <QLabel>
#include <QPixmap>

#include "FxLine.h"
#include "MixerView.h"

class FxLine;
class MixerView;

class SendButtonIndicator : public QLabel 
{
public:
	SendButtonIndicator( QWidget * _parent, FxLine * _owner,
						 MixerView * _mv);

	void mousePressEvent( QMouseEvent * e ) override;
	void updateLightStatus();

private:

	FxLine * m_parent;
	MixerView * m_mv;
	static QPixmap * s_qpmOn;
	static QPixmap * s_qpmOff;

	FloatModel * getSendModel();
};

#endif // SENDBUTTONINDICATOR_H
