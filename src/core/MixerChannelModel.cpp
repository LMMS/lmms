#include "MixerChannelModel.h"

#include "Engine.h"
#include "Mixer.h"

namespace lmms {
MixerChannelModel::MixerChannelModel(Model* parent)
	: IntModel(0, 0, 0, parent, tr("Mixer channel"))
{
	setRange(0, Engine::mixer()->numChannels() - 1, 1);

	connect(Engine::mixer(), &Mixer::channelsSwapped, this, &MixerChannelModel::channelsSwapped);
	connect(Engine::mixer(), &Mixer::channelDeleted, this, &MixerChannelModel::channelDeleted);
	connect(Engine::mixer(), &Mixer::channelCreated, this, &MixerChannelModel::channelCreated);

	connect(this, &MixerChannelModel::dataChanged, Engine::mixer(), [this] {
		--Engine::mixer()->mixerChannel(oldValue())->m_useCount;
		++Engine::mixer()->mixerChannel(value())->m_useCount;
	});

	++Engine::mixer()->mixerChannel(0)->m_useCount;
}

void MixerChannelModel::channelsSwapped(int fromIndex, int toIndex)
{
	if (value() == fromIndex) { setValue(toIndex); }
	else if (value() == toIndex) { setValue(fromIndex); }
}

void MixerChannelModel::channelDeleted(int index)
{
	if (value() == index) { setValue(0); }
	else if (value() > index) { setValue(value() - 1); }
	setRange(0, maxValue() - 1);
}

void MixerChannelModel::channelCreated(int index)
{
	setRange(0, maxValue() + 1);
}

} // namespace lmms