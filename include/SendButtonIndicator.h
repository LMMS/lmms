#ifndef SENDBUTTONINDICATOR_H
#define SENDBUTTONINDICATOR_H

#include <QLabel>


namespace lmms
{

class FloatModel;

namespace gui
{

class MixerLine;
class MixerView;


class SendButtonIndicator : public QLabel 
{
public:
	SendButtonIndicator( QWidget * _parent, MixerLine * _owner,
						 MixerView * _mv);

	void mousePressEvent( QMouseEvent * e ) override;
	void updateLightStatus();

private:

	MixerLine * m_parent;
	MixerView * m_mv;
	static QPixmap * s_qpmOn;
	static QPixmap * s_qpmOff;

	FloatModel * getSendModel();
};


} // namespace gui

} // namespace lmms

#endif // SENDBUTTONINDICATOR_H
