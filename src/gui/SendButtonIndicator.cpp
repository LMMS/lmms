#include "SendButtonIndicator.h"

#include "Engine.h"
#include "Mixer.h"
#include "MixerChannelView.h"
#include "MixerView.h"

namespace lmms::gui
{

SendButtonIndicator:: SendButtonIndicator(QWidget* parent, MixerChannelView* owner, MixerView* mv) :
	QLabel(parent),
	m_parent(owner),
	m_mv(mv)
{
	// don't do any initializing yet, because the MixerView and MixerChannelView
	// that were passed to this constructor are not done with their constructors
	// yet.
	setPixmap(m_qpmOff);
}

void SendButtonIndicator::mousePressEvent(QMouseEvent* e)
{
	Mixer* mix = Engine::mixer();
	int from = m_mv->currentMixerChannel()->channelIndex();
	int to = m_parent->channelIndex();
	FloatModel* sendModel = mix->channelSendModel(from, to);
	if (sendModel == nullptr)
	{
		// not sending. create a mixer send.
		mix->createChannelSend(from, to);
	}
	else
	{
		// sending. delete the mixer send.
		mix->deleteChannelSend(from, to);
	}

	m_mv->updateMixerChannel(m_parent->channelIndex());
	updateLightStatus();
}

FloatModel* SendButtonIndicator::getSendModel()
{
	Mixer* mix = Engine::mixer();
	return mix->channelSendModel(m_mv->currentMixerChannel()->channelIndex(), m_parent->channelIndex());
}

void SendButtonIndicator::updateLightStatus()
{
	setPixmap(!getSendModel() ? m_qpmOff : m_qpmOn);
}


} // namespace lmms::gui
