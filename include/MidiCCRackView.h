#ifndef MIDI_CC_RACK_VIEW_H
#define MIDI_CC_RACK_VIEW_H

#include <QWidget>
#include <QCloseEvent>

#include "SerializingObject.h"
#include "lmms_basics.h"

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

//protected:
//	void closeEvent( QCloseEvent * _ce ) override;

};

#endif
