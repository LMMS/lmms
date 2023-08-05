

#include "GrooveFactory.h"

#include "Groove.h"
#include "GrooveExperiments.h"
#include "MidiSwing.h"
#include "HalfSwing.h"
#include "HydrogenSwing.h"

namespace lmms
{

GrooveFactory::GrooveFactory()
{
}

/**
 * Factory method to create grooves classes of the correct type.
 * grooveType should match the string returned by the grooves nodeName() method
 *
 * TODO this is a bit Java-like how does C++ do this kind of thing normally
 */
Groove* GrooveFactory::create(const QString& grooveType) {
	if (grooveType.isEmpty() || grooveType == "none") {
		return new Groove();
	}
	if (grooveType == "hydrogen") {
		return new HydrogenSwing();
	}
	if (grooveType == "midi") {
		return new MidiSwing();
	}
	if (grooveType == "half") {
		return new HalfSwing();
	}
	if (grooveType == "experiment") {
		return new GrooveExperiments();
	}
	return new Groove();
}

} // namespace lmms
