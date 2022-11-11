#ifndef MIDI_CC_RACK_VIEW_H
#define MIDI_CC_RACK_VIEW_H

#include <QWidget>

#include "Midi.h"
#include "SerializingObject.h"

namespace lmms
{

class InstrumentTrack;

namespace gui
{

class Knob;
class GroupBox;

class MidiCCRackView : public QWidget, public SerializingObject
{
	Q_OBJECT
public:
	MidiCCRackView(InstrumentTrack * track);
	~MidiCCRackView() override;

	void saveSettings(QDomDocument & doc, QDomElement & parent) override;
	void loadSettings(const QDomElement &) override;

	inline QString nodeName() const override
	{
		return "MidiCCRackView";
	}

private slots:
	void renameWindow();

private:
	InstrumentTrack *m_track;

	GroupBox *m_midiCCGroupBox; // MIDI CC GroupBox (used to enable disable MIDI CC)

	Knob *m_controllerKnob[MidiControllerCount]; // Holds the knob widgets for each controller

};


} // namespace gui

} // namespace lmms

#endif
