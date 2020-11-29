#ifndef MIDI_CC_RACK_VIEW_H
#define MIDI_CC_RACK_VIEW_H

#include <QWidget>
#include <QLabel>
#include <QCloseEvent>

#include "SerializingObject.h"
#include "lmms_basics.h"
#include "ComboBox.h"
#include "ComboBoxModel.h"
#include "Knob.h"
#include "TrackContainer.h"
#include "GroupBox.h"
#include "Midi.h"

class InstrumentTrack;

class MidiCCRackView : public QWidget, public SerializingObject
{
	Q_OBJECT
public:
	MidiCCRackView(InstrumentTrack * track);
	virtual ~MidiCCRackView();

	void saveSettings(QDomDocument & doc, QDomElement & parent) override;
	void loadSettings(const QDomElement &) override;

	inline QString nodeName() const override
	{
		return "MidiCCRackView";
	}

public slots:
	void renameWindow();

private:
	InstrumentTrack *m_track;

	GroupBox *m_midiCCGroupBox; // MIDI CC GroupBox (used to enable disable MIDI CC)

	Knob *m_controllerKnob[MidiControllerCount]; // Holds the knob widgets for each controller

};

#endif
