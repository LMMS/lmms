

#include "SfzParser.h"
#include "SfzOpcodeState.h"
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
		qDebug() << "[SFZ Parser] Error, could not read file:" << filePath;
		return false;
	}

	QString fileContents = file.readAll();

	// Before parsing the headers and opcodes, we need to hande #include and #define statements
	// This amounts to recursively loading and copy/pasting the contents of the other files where the #include is, and find/replacing the #define words with their values
	fileContents = recursiveHandleIncludeAndDefineStatements(parentDirectory, fileContents);

	qDebug().noquote() << "TESTING: Loaded SFZ:\n" << fileContents; // testing

	// Now that all the includes/defines are handled, loop
	std::vector<QString> parsedSegments;

	for (QString line : fileContents.split("\n"))
	{
		// Remove comments from end of line
		line = line.split("//")[0];
		// Split the line on whitespace to extract header and opcodes keywords
		// Fortunately, the SFZ format specifically states that opcode assignments cannot contain spaces around the =, so there is no risk
		// of accidentally splitting the opcode name from the value.
		line.replace(">", "> "); // Real quick, make sure there is whitespace between header keywords and anything else after them. One .sfz file I found did `<curve>curve_index=11` in it,, with no space between, which makes parsing complicated, so to fix it we just insert an extra space.
		for (QString segment : line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts))
		{
			parsedSegments.push_back(segment);
		}
	}
	
	// Okay so ummm... there's a problem I didn't mention haha :sweat_smile:
	// Unfortunately, sample opcode assignments *can* contain spaces and special characters except for "=". This means that the sample opcode lines could have gotten split up by what we juts did :|
	// But that's okay. We can just loop through all of the segments, check if they have the sample opcode, and if so, check the next few segments too to see if they might be part of the file name (i.e., don't have = and aren't a header with < > aaround it)
	for (size_t i = 0; i < parsedSegments.size(); ++i)
	{
		if (parsedSegments.at(i).startsWith("sample="))
		{
			QStringList samplePathSegments;
			// Add the initial part of the sample file, before any spaces
			if (parsedSegments.at(i).split("=").size() != 2)
			{
				qDebug() << "[SFZ Parser] Warning, sample file path starts with a space? That's kind of weird:" << parsedSegments;
				samplePathSegments.push_back("");
			}
			else
			{
				samplePathSegments.push_back(parsedSegments.at(i).split("=")[1]);
			}
			// Look at the next few segments to see if they might be continuations of the sample file path
			for (size_t j = i + 1; j < parsedSegments.size();)
			{
				QString nextSegment = parsedSegments.at(j);
				if (nextSegment.contains("=")) { break; } // If there's an equals sign, it's an opcode assignment, not part of the sample file
				if (nextSegment.front() == "<" && nextSegment.back() == ">") { break; } // If it has < > around it, it's a header, so not part of the sample file
				samplePathSegments.push_back(nextSegment);
				parsedSegments.erase(parsedSegments.begin() + j); // Get rid of that segment, since it's part of the file path. Don't increment the index, since everything will shift back.
			}
			// Now replace the sample opcode segment with the complete filename
			parsedSegments.at(i) = "sample=" + samplePathSegments.join(" "); // Technically this doesn't account for samples with double spaces or tabs in the filename
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
		// Depending on the current header/zone, opcode assignments are handled differently:
		switch (currentHeader)
		{
			case Header::Global:
			{
				// Do nothing. The global headers were already handled above
				break;
			}
			case Header::Group:
			{
				// If we are in a group, update the opcodes of the current group state
				auto opcodeNameAndValue = segment.split("=");
				if (opcodeNameAndValue.size() != 2) { qDebug() << "[SFZ Parser] Syntax error, could not parse opcode assignment:" << segment; return false; }
				currentGroupState.setOpcodeByStrings(opcodeNameAndValue[0], opcodeNameAndValue[1]);
				break;
			}
			case Header::Region:
			{
				// If we are within a region, update the opcodes of the current region state
				auto opcodeNameAndValue = segment.split("=");
				if (opcodeNameAndValue.size() != 2) { qDebug() << "[SFZ Parser] Syntax error, could not parse opcode assignment:" << segment; return false; }
				currentRegionState.setOpcodeByStrings(opcodeNameAndValue[0], opcodeNameAndValue[1]);
				break;
			}
			case Header::Control:
			{
				qDebug() << "[SFZ Parser] Warning, the <control> header has not been implemented yet. Encountered opcode assignment:" << segment;
				break;
			}
			case Header::Curve:
			{
				qDebug() << "[SFZ Parser] Warning, the <curve> header has not been implemented yet. Encountered opcode assignment:" << segment;
				break;
			}
			default:
			{
				qDebug() << "[SFZ Parser] Error, encountered line within invalid header" << segment;
				return false;
			}
		}
	}
	// Check one last time in case the file ended with a region and didn't get added
	if (currentHeader == Header::Region) { outputRegions.emplace_back(currentRegionState); }

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
		else if (line.startsWith("#include"))
		{
			// Replace any of the defined variables before parsing the include path, since some SFZ files use $variables in them
			for (const auto& [variableName, variableValue] : defineMap)
			{
				line.replace(variableName, variableValue);
			}

			const auto segments = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
			// An include statement should have two parts, the #include and the path
			if (segments.size() != 2)
			{
				qDebug() << "[SFZ Parser] Ill-formed include statment:" << line;
				continue;
			}

			QString relativePath = segments[1];
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