

#include "GrooveFactory.h"

#include "Groove.h"
#include "MidiSwing.h"
#include "HydrogenSwing.h"

GrooveFactory::GrooveFactory()
{
}

/**
 * Factory method to create grooves classes of the correct type.
 * grooveType should match the string returned by the grooves nodeName() method
 *
 * TODO this is a bit Java-like how does C++ do this kind of thing normally
 */
Groove * GrooveFactory::create(QString _grooveType) {
	if (_grooveType == NULL || _grooveType == "none") {
		return new Groove();
	}
	if (_grooveType == "hydrogen") {
		return new HydrogenSwing();
	}
	if (_grooveType == "midi") {
		return new MidiSwing();
	}
	return new Groove();
}

