#ifndef MIDI_CC_RACK_VIEW_H
#define MIDI_CC_RACK_VIEW_H

#include <QWidget>
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
	MidiCCRackView();
	virtual ~MidiCCRackView();

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	inline QString nodeName() const override
	{
		return "MidiCCRackView";
	}

public slots:
	void updateTracksComboBox();
	void updateKnobsModels();

private:
	ComboBox *m_trackComboBox;
	ComboBoxModel *m_trackComboBoxModel;
	TrackContainer::TrackList m_tracks; // List with pointers to the ComboBox tracks themselfs

	GroupBox *m_midiCCGroupBox; // MIDI CC GroupBox (used to enable disable MIDI CC)

	Knob *m_controllerKnob[MidiControllerCount]; // Holds the knob widgets for each controller

};

#endif
