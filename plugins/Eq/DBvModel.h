#ifndef DBVMODEL
#define DBVMODEL

#include "AutomatableModel.h"


class DBvModel : public FloatModel
{
		Q_OBJECT
public:
		DBvModel( float val = 0, float min = 0, float max = 0, float step = 0,
						  Model * parent = NULL,
						  const QString& displayName = QString(),
						  bool defaultConstructed = false );
		inline float getAmp() const
		{
				return m_amp;
		}

private slots:
		void calcAmp();

private:
		float m_amp;
};




#endif // DBVMODEL

