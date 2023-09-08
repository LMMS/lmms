#include "WaveForm.h"

#include <stdio.h>
#include <QPushButton>

#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "LcdSpinBox.h"


#ifndef SLICERT_UI_H
#define SLICERT_UI_H

namespace lmms
{

class SlicerT;

namespace gui
{


class SlicerTUI : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	SlicerTUI( SlicerT * instrument,
					QWidget * parent );
	~SlicerTUI() override = default;

protected slots:
	void exportMidi();
	//void sampleSizeChanged( float _new_sample_length );

protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );

private:
	SlicerT * m_slicerTParent;

	Knob m_noteThresholdKnob;
	Knob m_fadeOutKnob;
	LcdSpinBox m_bpmBox;

	QPushButton m_resetButton;
	QPushButton m_timeShiftButton;
	QPushButton m_midiExportButton;

	WaveForm m_wf;


} ;


} // namespace gui

} // namespace lmms

#endif