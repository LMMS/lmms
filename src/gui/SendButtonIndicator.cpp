#include "SendButtonIndicator.h"

#include "engine.h"
#include "FxMixer.h"
#include "Model.h"

SendButtonIndicator:: SendButtonIndicator( QWidget * _parent, FxLine * _owner,
										   FxMixerView * _mv) :
	QLabel( _parent ),
	m_parent( _owner ),
	m_mv( _mv )
{
	qpmOff = embed::getIconPixmap("mixer_send_off", 23, 16);
	qpmOn = embed::getIconPixmap("mixer_send_on", 23, 16);

	// don't do any initializing yet, because the FxMixerView and FxLine
	// that were passed to this constructor are not done with their constructors
	// yet.

}

void SendButtonIndicator::mousePressEvent( QMouseEvent * e )
{
	FxMixer * mix = engine::fxMixer();
	int from = m_mv->currentFxLine()->channelIndex();
	int to = m_parent->channelIndex();
	FloatModel * sendModel = mix->channelSendModel(from, to);
	if( sendModel == NULL )
	{
		// not sending. create a mixer send.
		mix->createChannelSend( from, to );
	}
	else
	{
		// sending. delete the mixer send.
		mix->deleteChannelSend( from, to );
	}

	m_mv->updateFxLine(m_parent->channelIndex());
	updateLightStatus();
}

FloatModel * SendButtonIndicator::getSendModel()
{
	FxMixer * mix = engine::fxMixer();
	return mix->channelSendModel(
		m_mv->currentFxLine()->channelIndex(), m_parent->channelIndex());
}

void SendButtonIndicator::updateLightStatus()
{
	setPixmap( getSendModel() == NULL ? qpmOff : qpmOn );
}
