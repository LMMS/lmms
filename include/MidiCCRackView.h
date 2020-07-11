#ifndef MIDI_CC_RACK_VIEW_H
#define MIDI_CC_RACK_VIEW_H

#include <QWidget>
#include <QCloseEvent>

#include "SerializingObject.h"
#include "lmms_basics.h"
#include "ComboBox.h"
#include "Knob.h"

const int MIDI_CC_MAX_CONTROLLERS = 127;

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

private:
	ComboBox *m_trackComboBox;
	Knob *m_controllerKnob[MIDI_CC_MAX_CONTROLLERS]; // Holds the knob widgets for each controller

//protected:
//	void closeEvent( QCloseEvent * _ce ) override;

};

#endif
