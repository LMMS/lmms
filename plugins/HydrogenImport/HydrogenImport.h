#ifndef _HYDROGEN_IMPORT_H
#define _HYDROGEN_IMPORT_H

#include <QString>
#include <QPair>
#include <QVector>

#include "ImportFilter.h"


class HydrogenImport : public ImportFilter
{
public:
	HydrogenImport( const QString & _file, Engine * engine );
        bool readSong();

	virtual ~HydrogenImport();

	virtual PluginView * instantiateView( QWidget * )
	{
		return( NULL );
	}
private:
	virtual bool tryImport( TrackContainer* tc );
};
#endif

