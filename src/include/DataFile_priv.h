#ifndef DATA_FILE_PRIVATE_HEADER
#define DATA_FILE_PRIVATE_HEADER "DataFile_priv.h"
#else

static Type type( const QString& typeName );
static QString typeName( Type type );

void cleanMetaNodes( QDomElement de );

// helper upgrade routines
void upgrade_0_2_1_20070501();
void upgrade_0_2_1_20070508();
void upgrade_0_3_0_rc2();
void upgrade_0_3_0();
void upgrade_0_4_0_20080104();
void upgrade_0_4_0_20080118();
void upgrade_0_4_0_20080129();
void upgrade_0_4_0_20080409();
void upgrade_0_4_0_20080607();
void upgrade_0_4_0_20080622();
void upgrade_0_4_0_beta1();
void upgrade_0_4_0_rc2();
void upgrade_1_0_99();
void upgrade_1_1_0();
void upgrade_1_1_91();

void upgrade();

void loadData( const QByteArray & _data, const QString & _sourceFile );


struct EXPORT typeDescStruct
{
	Type m_type;
	QString m_name;
} ;
static typeDescStruct s_types[TypeCount];

#endif
