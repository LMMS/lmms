#include <stdio.h>
#include "WaveForm.h"
// #include "AutomatableModel.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
// #include "Graph.h"
// #include "MemoryManager.h"



#ifndef SLICER_UI_H
#define SLICER_UI_H

namespace lmms
{

// forward declaration, to be able to use SlicerT as a parameter
class SlicerT;

namespace gui
{

// class Knob;
// class LedCheckBox;
// class PixmapButton;


class SlicerTUI : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	SlicerTUI( SlicerT * _instrument,
					QWidget * _parent );
	~SlicerTUI() override = default;

// protected slots:
	//void sampleSizeChanged( float _new_sample_length );

protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mousePressEvent( QMouseEvent * _me );


private:
	Knob noteThresholdKnob;

	WaveForm wf;


} ;


} // namespace gui

} // namespace lmms

#endif