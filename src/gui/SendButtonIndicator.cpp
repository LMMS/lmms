#include "SendButtonIndicator.h"

#include "embed.h"
#include "Mixer.h"
#include "MixerLine.h"
#include "MixerView.h"

namespace lmms::gui
{

SendButtonIndicator:: SendButtonIndicator( QWidget * _parent, MixerLine * _owner,
										   MixerView * _mv) :
	QLabel( _parent ),
	m_parent( _owner ),
	m_mv( _mv )
{
	static auto s_qpmOff = QPixmap{embed::getIconPixmap("mixer_send_off", 29, 20)};
	static auto s_qpmOn = QPixmap{embed::getIconPixmap("mixer_send_on", 29, 20)};
	m_qpmOff = &s_qpmOff;
	m_qpmOn = &s_qpmOn;

	// don't do any initializing yet, because the MixerView and MixerLine
	// that were passed to this constructor are not done with their constructors
	// yet.
	setPixmap(s_qpmOff);
}

void SendButtonIndicator::mousePressEvent( QMouseEvent * e )
{
	Mixer * mix = Engine::mixer();
	int from = m_mv->currentMixerLine()->channelIndex();
	int to = m_parent->channelIndex();
	FloatModel * sendModel = mix->channelSendModel(from, to);
	if( sendModel == nullptr )
	{
		// not sending. create a mixer send.
		mix->createChannelSend( from, to );
	}
	else
	{
		// sending. delete the mixer send.
		mix->deleteChannelSend( from, to );
	}

	m_mv->updateMixerLine(m_parent->channelIndex());
	updateLightStatus();
}

FloatModel * SendButtonIndicator::getSendModel()
{
	Mixer * mix = Engine::mixer();
	return mix->channelSendModel(
		m_mv->currentMixerLine()->channelIndex(), m_parent->channelIndex());
}

void SendButtonIndicator::updateLightStatus()
{
	setPixmap(!getSendModel() ? *m_qpmOff : *m_qpmOn);
}


} // namespace lmms::gui
