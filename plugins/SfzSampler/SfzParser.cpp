

#include "SfzParser.h"
#include "SfzOpcodeState.h"
#include <QStringList>
#include <QRegularExpression>
#include <QDebug>

namespace lmms
{


bool SfzParser::parseSfzFile(const QString& filePath, std::vector<SfzRegion>& outputRegions, SfzControlsConfig& controlsConfig)
{
	// Clear the vector of regions just in case anything is inside it from before
	outputRegions.clear();

	// Open the .sfz file
	QDir parentDirectory = QFileInfo(filePath).absoluteDir();
	QFile file(filePath);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "[SFZ Parser] Error, could not read file:" << filePath;
		return false;
	}

	QString fileContents = file.readAll();

	// Before parsing the headers and opcodes, we need to hande #include and #define statements
	// This amounts to recursively loading and copy/pasting the contents of the other files where the #include is, and find/replacing the #define words with their values
	fileContents = recursiveHandleIncludeAndDefineStatements(parentDirectory, fileContents);

	qDebug().noquote() << "TESTING: Loaded SFZ:\n" << fileContents; // testing

	// Now that all the includes/defines are handled, loop through the whole contents and split it up into segments so that it can be parsed
	// For example, if you have a sfz file like:
	/*
		<region>
		sample=test.wav
		key=70
	*/
	// Then the parsed segments would be "<region>", "sample=test.wav", and "key=70"
	// In this example they were all on separate lines, but they don't have to be:
	/*
		<region> ampeg_release=0.3 lokey=45 hikey=49
	*/
	// This would still be parsed into segments as "<region>", "ampeg_release=0.3", "lokey=45", and "hikey=49"
	// According to the SFZ format website, there must never be a space on either side of the = for an opcode assignment. That makes things simpler
	// However, there can be spaces in the value assigned to the opcode. For example, if your sample file name has spaces:
	/*
		<region> sample=Grand Piano G4 MP.wav key=49
	*/
	// This would be parsed as "<region>", "sample=Grand Piano G4 MP.wav", and "key=49"
	// Essentially, we need a way to be able to split up the file on whitespace, while leaving intact any opcode assignments which have spaces in their right hand side.
	std::vector<QString> parsedSegments;

	// We start by splitting on newline
	for (QString line : fileContents.split("\n"))
	{
		// Remove comments from end of line
		line = line.split("//")[0];
		
		line.replace(">", "> "); // Real quick, make sure there is whitespace between header keywords and anything else after them. One .sfz file I found did `<curve>curve_index=11` in it, with no space between, which makes parsing complicated, so to fix it we just insert an extra space.
		
		// We can start by splitting on whitespace, but then we will have to go back and group together any chunks which belong to the same opcode assignment, if for example the sample file included spaces
		// For example, if we had the line:
		//     <region> sample=My Favorite Sample.flac key=99
		// Then it would initially by split into "<region>", "sample=My", "Favorite", "Sample.flac", and "key=99"
		// However, we notice that "Favorite" and "Sample.flac" are not valid opcode assignments since they don't have an "=", and they're not headers, since you don't have those <brackets>
		// So they must belong to the previous opcode assignment, "sample=My"
		// If we connect them together, we get "<region>", "sample=My Favorite Sample.flac", and "key=99", just as we wanted!
		std::vector<QString> lineSegments;
		for (QString segment : line.split(QRegularExpression("\\s"), Qt::SkipEmptyParts)) // Note: Technically by skipping empty parts, double-spaces within names will be lost. Is this okay? I'm not sure.
		{
			if (segment.contains("=") || (segment.front() == "<" && segment.back() == ">"))
			{
				// If this is an opcode assignment or a <header>, go ahead and add it to the list as it is
				lineSegments.push_back(segment);
			}
			else
			{
				// If it's not, then it must belong to the previous segment, so let's concatinate it (with a space, to account for the space taken from the split)
				if (lineSegments.size() > 0)
				{
					lineSegments.back() += " " + segment;
				}
				else
				{
					qDebug() << "[SFZ Parser] Warning: Encountered non-header, non-opcode assignment at start of line:" << line;
				}
			}
		}
		// Add the segments from the current line to the overall list
		parsedSegments.insert(parsedSegments.end(), lineSegments.begin(), lineSegments.end());
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
	// Keep track of the current group and region states. These will be re-initialized to whatever the global/group settings are, once we encounter a new group or region header
	SfzOpcodeState currentGroupState = globalState;
	SfzOpcodeState currentRegionState = globalState;

	// Track whether we are in global, group, region, or other header
	Header currentHeader = Header::None;
	for (QString segment : parsedSegments)
	{
		// Track whether we are entering a new header region
		if (segment.front() == "<" && segment.back() == ">")
		{
			// If we were previously in a <region>, then wrap it up and add it to the output vector
			if (currentHeader == Header::Region) { outputRegions.emplace_back(currentRegionState); }

			if (segment == "<global>")
			{
				currentHeader = Header::Global;
				// Don't do anything special; we already handled the globals above
			}
			else if (segment == "<group>")
			{
				currentHeader = Header::Group;
				// Reset the current group settings to the global defaults
				currentGroupState = globalState;
			}
			else if (segment == "<region>")
			{
				currentHeader = Header::Region;
				// Reset the current region settings to the group defaults
				currentRegionState = currentGroupState;
			}
			else if (segment == "<control>")
			{
				currentHeader = Header::Control;
			}
			else if (segment == "<curve>")
			{
				currentHeader = Header::Curve;
			}
			else
			{
				qDebug() << "[SFZ Parser] Error, unknown header type:" << segment;
				return false;
			}
			continue; // If the header is recognized, move to the next line and start parsing things
			// TODO handle more header types
		}

		// If this line/segment isn't a new header, it must be an opcode assignment
		auto opcodeNameAndValue = segment.split("=");
		if (opcodeNameAndValue.size() != 2) { qDebug() << "[SFZ Parser] Syntax error, could not parse opcode assignment:" << segment; return false; }
		// Depending on the current header/zone, opcode assignments are handled differently:
		switch (currentHeader)
		{
			case Header::Global:
			{
				// Do nothing. The global headers were already handled above
				// TODO this is wrong, some sfz's treat global headers as local, expecting that one after another will overwrite the last
				break;
			}
			case Header::Group:
			{
				// If we are in a group, update the opcodes of the current group state
				currentGroupState.setOpcodeByStrings(opcodeNameAndValue[0], opcodeNameAndValue[1]);
				break;
			}
			case Header::Region:
			{
				// If we are within a region, update the opcodes of the current region state
				currentRegionState.setOpcodeByStrings(opcodeNameAndValue[0], opcodeNameAndValue[1]);
				break;
			}
			case Header::Control:
			{
				controlsConfig.setOpcodeByStrings(opcodeNameAndValue[0], opcodeNameAndValue[1]);
				// Also set the opcode for the global state, that way it will get propagated down to all other regions (this is needed for things like `default_path` to work for all regions)
				// Technically it could be reworked so that the regions fetch any needed info from the controls state, but for now this way is simpler.
				globalState.setOpcodeByStrings(opcodeNameAndValue[0], opcodeNameAndValue[1]);
				break;
			}
			case Header::Curve:
			{
				qDebug() << "[SFZ Parser] Warning: The <curve> header has not been implemented yet. Encountered opcode assignment:" << segment;
				break;
			}
			default:
			{
				qDebug() << "[SFZ Parser] Error: Encountered line within invalid header" << segment;
				return false;
			}
		}
		// For the GUI, it's nice to keep track of which midi CC's are being used, and only display those (not all 128)
		// This is a bit hacky, but just checking if the opcode has "ccN" inside it and parsing that number works fine
		QRegularExpression re("cc\\d+");
		QRegularExpressionMatch match = re.match(opcodeNameAndValue[0]);
		if (match.hasMatch())
		{
			int ccNumber = match.captured(0).split("cc")[1].toInt();
			if (ccNumber >= 0 && ccNumber <= SfzOpcodeState::NumMidiCCs) { controlsConfig.m_activeMidiCCs.at(ccNumber) = true; }
		}
	}
	// Check one last time in case the file ended with a region and didn't get added
	if (currentHeader == Header::Region) { outputRegions.emplace_back(currentRegionState); }


	// Just so that the GUI doesn't have to loop over all the regions to find the switch keys, let's also
	// make a list of them in the controlsConfig object for easy access
	for (const auto& region : outputRegions)
	{
		if (region.m_sw_last != std::nullopt)
		{
			SfzControlsConfig::SwitchKeyInfo info;
			info.sw_label = region.m_sw_label.value_or("");
			info.sw_lokey = region.m_sw_lokey;
			info.sw_hikey = region.m_sw_hikey;
			info.sw_default = region.m_sw_default;
			controlsConfig.m_switchKeyInfo.insert({region.m_sw_last.value(), info});
		}
	}

	// Now that all the opcodes have been parsed into regions and added to the output vector, we are done!
	// The samples themselves still need to be loaded, but that's a job for later
	return true;
}





QString SfzParser::recursiveHandleIncludeAndDefineStatements(const QDir& parentDirectory, QString fileContents, std::map<QString, QString> defineMap)
{
	// Reconstruct the file line by line as we parse the defines and includes
	QStringList reconstructedSegments;

	for (QString line : fileContents.split("\n"))
	{
		if (line.startsWith("#define"))
		{
			// Split on whitespace
			const auto segments = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
			// A define statement should have 3 parts, the #define, the $variable name, and the value
			if (segments.size() != 3)
			{
				qDebug() << "[SFZ Parser] Ill-formed define statment:" << line;
				continue;
			}
			const QString variableName = segments[1];
			const QString variableValue = segments[2];
			defineMap[variableName] = variableValue;
			// A define variable should probably start with a $
			if (variableName.front() != "$") { qDebug() << "[SFZ Parser] Warning: #define variable name does not start with $:" << line; }
		}
		else if (line.startsWith("#include "))
		{
			// Replace any of the defined variables before parsing the include path, since some SFZ files use $variables in them
			for (const auto& [variableName, variableValue] : defineMap)
			{
				line.replace(variableName, variableValue);
			}

			const auto segments = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
			// An include statement should have two parts, the #include and the path
			if (line.split("#include ").size() != 2)
			{
				qDebug() << "[SFZ Parser] Ill-formed include statment:" << line;
				continue;
			}

			QString relativePath = line.split("#include ")[1];
			relativePath.replace("\"", ""); // Remove " " from start and end
			const QString absolutePath = parentDirectory.absoluteFilePath(relativePath);
			
			QFile file(absolutePath);
			if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				qDebug() << "[SFZ Parser] Could not read included file:" << absolutePath << "from include statement:" << line;
				continue;
			}
			
			QString includedFileContents = file.readAll();

			// Resolve any includes and defines in this new file too before pasting it in
			includedFileContents = recursiveHandleIncludeAndDefineStatements(parentDirectory, includedFileContents, defineMap);
			reconstructedSegments.push_back(includedFileContents);
		}
		else
		{
			// Replace any of the defined variables before adding the line to the reconstructed file
			for (const auto& [variableName, variableValue] : defineMap)
			{
				line.replace(variableName, variableValue);
			}
			reconstructedSegments.push_back(line);
		}
	}

	return reconstructedSegments.join("\n");
}



} // namespace lmms