#ifndef _HYDROGEN_IMPORT_H
#define _HYDROGEN_IMPORT_H

#include <QString>
#include <QPair>
#include <QVector>

#include "ImportFilter.h"

namespace lmms
{


class HydrogenImport : public ImportFilter
{
public:
	HydrogenImport( const QString & _file );
        bool readSong();

	virtual ~HydrogenImport();

	virtual gui::PluginView* instantiateView( QWidget * )
	{
		return( nullptr );
	}
private:
	virtual bool tryImport( TrackContainer* tc );
};


} // namespace lmms

#endif

