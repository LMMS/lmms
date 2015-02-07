#include "SendButtonIndicator.h"

#include "Engine.h"
#include "FxMixer.h"
#include "Model.h"

QPixmap * SendButtonIndicator::s_qpmOff = NULL;
QPixmap * SendButtonIndicator::s_qpmOn = NULL;

SendButtonIndicator:: SendButtonIndicator( QWidget * _parent, FxLine * _owner,
										   FxMixerView * _mv) :
	QLabel( _parent ),
	m_parent( _owner ),
	m_mv( _mv )
{
	if( ! s_qpmOff )
	{
		s_qpmOff = new QPixmap( QPixmap( "icons:mixer_send_off.png" ).scaled( 29, 20 ) );
	}

	if( ! s_qpmOn )
	{
		s_qpmOn = new QPixmap( QPixmap( "icons:mixer_send_on.png" ).scaled( 29, 20 ) );
	}

	// don't do any initializing yet, because the FxMixerView and FxLine
	// that were passed to this constructor are not done with their constructors
	// yet.
	setPixmap( *s_qpmOff );
}

void SendButtonIndicator::mousePressEvent( QMouseEvent * e )
{
	FxMixer * mix = Engine::fxMixer();
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
	FxMixer * mix = Engine::fxMixer();
	return mix->channelSendModel(
		m_mv->currentFxLine()->channelIndex(), m_parent->channelIndex());
}

void SendButtonIndicator::updateLightStatus()
{
	setPixmap( getSendModel() == NULL ? *s_qpmOff : *s_qpmOn );
}
