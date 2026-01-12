

#include "SfzTrigger.h"

namespace lmms
{

const SfzTrigger SfzTrigger::noteOnEvent(const int frameOffset, const int key, const int velocity)
{
	SfzTrigger trigger = SfzTrigger();
	trigger.m_type = Type::NoteOn;
	trigger.m_key = key;
	trigger.m_velocity = velocity;
	trigger.m_frameOffset = frameOffset;
	return trigger;
}

const SfzTrigger SfzTrigger::noteOffEvent(const int frameOffset, const int key, const int velocity)
{
	SfzTrigger trigger = SfzTrigger();
	trigger.m_type = Type::NoteOff;
	trigger.m_key = key;
	trigger.m_velocity = velocity;
	trigger.m_frameOffset = frameOffset;
	return trigger;
}

const SfzTrigger SfzTrigger::controlChangeEvent(const int frameOffset, const int controlNumber, const int value)
{
	SfzTrigger trigger = SfzTrigger();
	trigger.m_type = Type::ControlChange;
	trigger.m_controlChangeNumber = controlNumber;
	trigger.m_controlChangeValue = value;
	trigger.m_frameOffset = frameOffset; // Frame offset is ignored for CC events, since making them sample-exact would be inconvenient
	return trigger;
}

} // namespace lmms