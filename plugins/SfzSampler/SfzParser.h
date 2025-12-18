
#ifndef LMMS_SFZ_PARSER_H
#define LMMS_SFZ_PARSER_H

#include "SfzRegion.h"
#include <QString>
#include <vector>

namespace lmms
{

class SfzParser
{
public:
	static bool parseSfzFile(const QString& filePath, std::vector<SfzRegion>& outputRegions);
};



} // namespace lmms


#endif // LMMS_SFZ_PARSER_H