
#ifndef LMMS_SFZ_TRIGGER_H
#define LMMS_SFZ_TRIGGER_H

#include <optional>

namespace lmms
{


class SfzTrigger
{
public:
	static const SfzTrigger noteOnEvent(const int key, const int vel);
	static const SfzTrigger noteOffEvent(const int key, const int vel);
	static const SfzTrigger controlChangeEvent(const int controlNumber, const int value);

	enum class Type
	{
		NoteOn,
		NoteOff,
		ControlChange,
	};

	const auto& type() const { return m_type; }
	const auto& key() const { return m_key; }
	const auto& velocity() const { return m_velocity; }
	const auto& controlChangeNumber() const { return m_controlChangeNumber; }
	const auto& controlChangeValue() const { return m_controlChangeValue; }

private:
	Type m_type;
	std::optional<int> m_key;
	std::optional<int> m_velocity;
	std::optional<int> m_controlChangeNumber;
	std::optional<int> m_controlChangeValue;
};


} // namespace lmms


#endif // LMMS_SFZ_TRIGGER_H