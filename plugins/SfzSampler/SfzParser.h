
#ifndef LMMS_SFZ_PARSER_H
#define LMMS_SFZ_PARSER_H

#include "SfzRegion.h"
#include <QString>
#include <QDir>
#include <vector>

namespace lmms
{

class SfzParser
{
public:
	static bool parseSfzFile(const QString& filePath, std::vector<SfzRegion>& outputRegions);

	//! Helper function to parse any #include or #define statements from a string, recursively so that the included files have their includes and defines handled too
	//! The defineMap parameter should be left blank, since it's only there to internally keep track of what $keywords are defined to be what as the recursion goes down each path
	static QString recursiveHandleIncludeAndDefineStatements(const QDir& parentDirectory, QString fileContents, std::map<QString, QString> defineMap = {});
};



} // namespace lmms


#endif // LMMS_SFZ_PARSER_H