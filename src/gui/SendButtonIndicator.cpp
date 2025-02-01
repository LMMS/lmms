#include "SendButtonIndicator.h"

#include "embed.h"
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
		// create mixer sends for all other selected channels if they don't have them already
		for (auto mcv : m_mv->selectedChannels())
		{
			if (mcv->channelIndex() != from && mcv->channelIndex() != to
				&& mix->channelSendModel(mcv->channelIndex(), to) == nullptr)
			{
				mix->createChannelSend(mcv->channelIndex(), to);
			}
		}
	}
	else
	{
		// sending. delete the mixer send.
		mix->deleteChannelSend(from, to);
		// delete mixer sends for all other selected channels if they have them
		for (auto mcv : m_mv->selectedChannels())
		{
			if (mcv->channelIndex() != from && mcv->channelIndex() != to
				&& mix->channelSendModel(mcv->channelIndex(), to) != nullptr)
			{
				mix->deleteChannelSend(mcv->channelIndex(), to);
			}
		}
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
