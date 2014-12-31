#include "DBvModel.h"

DBvModel::DBvModel(float val, float min, float max, float step,
								   Model *parent, const QString &displayName,
								   bool defaultConstructed) :
		FloatModel( val, min, max, step, parent, displayName, defaultConstructed )
{
		connect(this, SIGNAL( dataChanged() ), this,SLOT( calcAmp() ) );
}

void DBvModel::calcAmp()
{
		m_amp = dbvToAmp( value() );
}
