

#include "SfzTrigger.h"

namespace lmms
{

const SfzTrigger SfzTrigger::noteOnEvent(const int key, const int velocity)
{
	SfzTrigger trigger = SfzTrigger();
	trigger.m_type = Type::NoteOn;
	trigger.m_key = key;
	trigger.m_velocity = velocity;
	return trigger;
}

const SfzTrigger SfzTrigger::noteOffEvent(const int key, const int velocity)
{
	SfzTrigger trigger = SfzTrigger();
	trigger.m_type = Type::NoteOff;
	trigger.m_key = key;
	trigger.m_velocity = velocity;
	return trigger;
}

const SfzTrigger SfzTrigger::controlChangeEvent(const int controlNumber, const int value)
{
	SfzTrigger trigger = SfzTrigger();
	trigger.m_type = Type::ControlChange;
	trigger.m_controlChangeNumber = controlNumber;
	trigger.m_controlChangeValue = value;
	return trigger;
}

} // namespace lmms