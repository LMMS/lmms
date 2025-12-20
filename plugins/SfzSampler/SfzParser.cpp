

#include "SfzParser.h"
#include "SfzOpcodeState.h"
#include <QDir>
#include <QStringList>
#include <QRegularExpression>
#include <QDebug>

namespace lmms
{


bool SfzParser::parseSfzFile(const QString& filePath, std::vector<SfzRegion>& outputRegions)
{
	// Clear the vector of regions just in case anything is inside it from before
	outputRegions.clear();

	// Open the .sfz file
	QDir parentDirectory = QFileInfo(filePath).absoluteDir();
	QFile file(filePath);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "[SFZ Parser] Could not read file:" << filePath;
		return false;
	}

	std::vector<QString> parsedSegments;

	while (!file.atEnd())
	{
		QString line = file.readLine();
		// Remove comments from end of line
		line = line.split("\\")[0];
		// Split the line on whitespace to extract header and opcodes keywords
		// Fortunately, the SFZ format specifically states that opcode assignments cannot contain spaces, so there is no risk
		// of accidentally splitting the opcode name from the value.
		for (QString segment : line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts))
		{
			parsedSegments.push_back(segment);
		}
	}

	// Now that all the segments are collected, we can go through them all and construct the SfzRegions
	// First, the <global> header(s) must be found. The SFZ format website does not guarantee that <global> will
	// be the first header, so we have to search through all the segments to find the globals before parsing any <region> headers

	// Create a base opcode state list to keep track of which defaults are global
	SfzOpcodeState globalState;

	bool withinGlobal = false;
	for (QString segment : parsedSegments)
	{
		// Track whether we are entering a <global> header region
		if (segment.front() == "<" && segment.back() == ">")
		{
			if (segment == "<global>")
			{
				withinGlobal = true;
			}
			else
			{
				withinGlobal = false;
			}
			continue;
		}

		// If we are in <global>, update the global opcode state
		if (withinGlobal)
		{
			// Opcodes are stored in name=value format, with no spaces, so splitting on the "=" always works
			auto opcodeNameAndValue = segment.split("=");
			if (opcodeNameAndValue.size() != 2) { qDebug() << "[SFZ Parser] Syntax error, could not parse opcode assignment:" << segment; return false; }
			globalState.setOpcodeByStrings(opcodeNameAndValue[0], opcodeNameAndValue[1]);
		}
	}

	// Now that all the global opcode defaults have been parsed, we can start stepping through the file again and
	// parsing all the <region>s into SfzRegion objects

	// Regions can be within <group>s, which can define default opcodes for all the regions inside them
	// Keep track of the current group and region states. These will be initialized to whatever the global/group settings are, once we encounter a group ro region header
	SfzOpcodeState currentGroupState;
	SfzOpcodeState currentRegionState;

	// Track whether we are in global, a group, or a region header
	withinGlobal = false;
	bool withinGroup = false;
	bool withinRegion = false;
	for (QString segment : parsedSegments)
	{
		// Track whether we are entering a new header region
		if (segment.front() == "<" && segment.back() == ">")
		{
			if (segment == "<global>")
			{
				// If we were previously in a <region>, then wrap it up and add it to the output vector
				if (withinRegion) { outputRegions.emplace_back(currentRegionState); }
				withinGlobal = true;
				withinGroup = false;
				withinRegion = false;
				// Don't do anything special; we already handled the globals above
			}
			else if (segment == "<group>")
			{
				if (withinRegion) { outputRegions.emplace_back(currentRegionState); }
				withinGlobal = false;
				withinGroup = true;
				withinRegion = false;
				// Reset the current group settings to the global defaults
				currentGroupState = globalState;
			}
			else if (segment == "<region>")
			{
				if (withinRegion) { outputRegions.emplace_back(currentRegionState); }
				withinGlobal = false;
				withinGroup = false;
				withinRegion = true;
				// Reset the current region settings to the group defaults
				currentRegionState = currentGroupState;
			}
			else
			{
				qDebug() << "[SFZ Parser] Unknown header type:" << segment;
				return false;
			}
			continue;
			// TODO handle more header types
		}

		// If we are in a group, update the opcodes of the current group state
		if (withinGroup)
		{
			auto opcodeNameAndValue = segment.split("=");
			if (opcodeNameAndValue.size() != 2) { qDebug() << "[SFZ Parser] Syntax error, could not parse opcode assignment:" << segment; return false; }
			currentGroupState.setOpcodeByStrings(opcodeNameAndValue[0], opcodeNameAndValue[1]);
		}
		// If we are within a region, update the opcodes of the current region state
		if (withinRegion)
		{
			auto opcodeNameAndValue = segment.split("=");
			if (opcodeNameAndValue.size() != 2) { qDebug() << "[SFZ Parser] Syntax error, could not parse opcode assignment:" << segment; return false; }
			currentRegionState.setOpcodeByStrings(opcodeNameAndValue[0], opcodeNameAndValue[1]);
		}
	}
	// Check one last time in case the file ended with a region and didn't get added
	if (withinRegion) { outputRegions.emplace_back(currentRegionState); }

	// Now that all the opcodes have been parsed into regions and added to the output vector, we are done!
	// The samples themselves still need to be loaded, but that's a job for later
	return true;
}


} // namespace lmms